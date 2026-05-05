#include "SettingsDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRandomGenerator>
#include <QScrollArea>
#include <QSettings>
#include <QSizePolicy>
#include <QSlider>
#include <QSpinBox>
#include <QTabWidget>
#include <QToolButton>
#include <QToolTip>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

#include "../ai/SystemPrompt.h"
#include "../net/JarvisHttpServer.h"
#include "WelcomeDialog.h"

namespace {

// =============================================================================
//  Local helpers
// =============================================================================

// Round "?" tool button. Click → bubble tooltip with rich HTML description.
class HelpButton : public QToolButton {
public:
    HelpButton(const QString& description, QWidget* parent = nullptr)
        : QToolButton(parent), m_text(description)
    {
        setText(QStringLiteral("?"));
        setCursor(Qt::PointingHandCursor);
        setFixedSize(22, 22);
        setFocusPolicy(Qt::TabFocus);
        setAutoRaise(true);
        setToolTipDuration(20000);
        setToolTip(description);
        setStyleSheet(QStringLiteral(R"_(
            QToolButton {
                color: #8a99b1;
                background: rgba(35, 49, 70, 160);
                border: 1px solid rgba(80, 110, 150, 140);
                border-radius: 11px;
                font-size: 11px;
                font-weight: 700;
            }
            QToolButton:hover {
                color: #ffffff;
                background: rgba(47, 129, 247, 200);
                border-color: #58a6ff;
            }
            QToolButton:pressed {
                background: rgba(31, 110, 235, 220);
            })_"));
        connect(this, &QToolButton::clicked, this, [this]() {
            QToolTip::showText(
                mapToGlobal(QPoint(width() / 2, height() + 4)),
                QStringLiteral("<div style='max-width:360px; line-height:1.5;'>%1</div>")
                    .arg(m_text),
                this);
        });
    }
private:
    QString m_text;
};

// Build a "card" container for a tab section. Every Settings field is wrapped
// in one of these to give the dialog a structured, premium feel instead of
// a plain QFormLayout.
QFrame* makeCard(QWidget* parent) {
    auto* card = new QFrame(parent);
    card->setObjectName(QStringLiteral("settingCard"));
    card->setStyleSheet(QStringLiteral(R"_(
        QFrame#settingCard {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 rgba(20, 28, 40, 230),
                stop:1 rgba(11, 16, 25, 230));
            border: 1px solid rgba(60, 78, 102, 140);
            border-radius: 14px;
        }
    )_"));
    return card;
}

// A "label-on-top, control-below" row inside a card. Optionally adds a "?"
// help button to the right of the title. The control is added at the bottom.
struct FieldOptions {
    QString  title;
    QString  subtitle;     // optional grey caption shown below title
    QString  helpText;     // if non-empty, adds a "?" button on the title row
    QWidget* control = nullptr;   // main control (QSpinBox, QSlider, etc.)
    QWidget* trailing = nullptr;  // optional right-aligned widget on the title row
};

QWidget* buildFieldCard(QWidget* parent, const FieldOptions& opt) {
    auto* card = makeCard(parent);
    auto* lay  = new QVBoxLayout(card);
    lay->setContentsMargins(18, 14, 18, 14);
    lay->setSpacing(8);

    // --- Title row (label + optional help "?" + optional trailing) ---
    auto* titleRow = new QHBoxLayout;
    titleRow->setContentsMargins(0, 0, 0, 0);
    titleRow->setSpacing(8);

    auto* title = new QLabel(opt.title, card);
    title->setStyleSheet(QStringLiteral(
        "color: #e6edf3; font-size: 13.5px; font-weight: 700; "
        "letter-spacing: 0.3px; background: transparent;"));
    titleRow->addWidget(title);

    if (!opt.helpText.isEmpty()) {
        titleRow->addWidget(new HelpButton(opt.helpText, card),
                            0, Qt::AlignVCenter);
    }
    titleRow->addStretch();
    if (opt.trailing) titleRow->addWidget(opt.trailing, 0, Qt::AlignVCenter);

    lay->addLayout(titleRow);

    // --- Optional subtitle ---
    if (!opt.subtitle.isEmpty()) {
        auto* sub = new QLabel(opt.subtitle, card);
        sub->setWordWrap(true);
        sub->setStyleSheet(QStringLiteral(
            "color: #8a99b1; font-size: 11.5px; font-weight: 500; "
            "letter-spacing: 0.2px; background: transparent;"));
        lay->addWidget(sub);
    }

    // --- Control ---
    if (opt.control) {
        opt.control->setParent(card);
        lay->addWidget(opt.control);
    }

    return card;
}

// Tiny helper for tabs: a vertical scroll-able area that hosts a column of
// cards with consistent margins.
QWidget* makeTabPage(QWidget*& innerCol /*out*/) {
    auto* page = new QWidget;
    auto* col  = new QVBoxLayout(page);
    col->setContentsMargins(4, 14, 14, 14);
    col->setSpacing(12);
    innerCol = page;
    return page;
}

// Render a circular avatar pixmap into a QLabel of the given side. Falls
// back to a "+" glyph when no avatar is set yet.
void paintAvatarLabel(QLabel* label, int side) {
    label->setFixedSize(side, side);
    label->setAlignment(Qt::AlignCenter);

    QPixmap pix = WelcomeDialog::savedAvatar(side);
    if (!pix.isNull()) {
        // Mask + halo + ring are drawn into the bitmap itself, so the QLabel
        // does not need a square border (which would visibly leak outside
        // the round mask).
        label->setPixmap(WelcomeDialog::roundAvatar(pix, side));
        label->setText(QString());
        label->setStyleSheet(QStringLiteral(
            "background: transparent; border: none;"));
    } else {
        label->setPixmap(QPixmap());
        label->setText(QStringLiteral("+"));
        label->setStyleSheet(QStringLiteral(
            "color: #58a6ff; font-size: %1px; font-weight: 300; "
            "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, "
            "  stop:0 rgba(47,129,247,55), stop:1 rgba(31,110,235,30)); "
            "border: 2px dashed rgba(47, 129, 247, 160); "
            "border-radius: %2px;")
                .arg(side / 3).arg(side / 2));
    }
}

// QSettings keys.
constexpr const char* kK_Model         = "settings/modelPath";
constexpr const char* kK_Temperature   = "settings/temperature";
constexpr const char* kK_Context       = "settings/contextSize";
constexpr const char* kK_TopP          = "settings/topP";
constexpr const char* kK_TopK          = "settings/topK";
constexpr const char* kK_MinP          = "settings/minP";
constexpr const char* kK_RepeatPen     = "settings/repeatPenalty";
constexpr const char* kK_MaxTokens     = "settings/maxTokens";
constexpr const char* kK_SystemPrompt  = "settings/systemPrompt";
constexpr const char* kK_AccentRgb     = "settings/accentRgb";
constexpr const char* kK_Opacity       = "settings/opacityPct";
constexpr const char* kK_Timestamps    = "settings/showTimestamps";
constexpr const char* kK_UserName      = "user/name";
constexpr const char* kK_GpuLayers     = "settings/gpuLayers";
constexpr const char* kK_PromptTpl     = "settings/promptTemplate";
constexpr const char* kK_ServerEnabled = "server/enabled";
constexpr const char* kK_ServerPort    = "server/port";
constexpr const char* kK_ServerPin     = "server/pin";

} // namespace

// =============================================================================
//  SettingsDialog
// =============================================================================

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("JARVIS · Налаштування"));
    setModal(true);
    resize(760, 760);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(22, 22, 22, 18);
    root->setSpacing(14);

    // ---- Title strip ----
    {
        auto* eyebrow = new QLabel(QStringLiteral("JARVIS · ПАНЕЛЬ КЕРУВАННЯ"), this);
        eyebrow->setStyleSheet(QStringLiteral(
            "color: #58a6ff; font-size: 10px; font-weight: 800; letter-spacing: 4px;"));
        auto* title = new QLabel(QStringLiteral("Налаштування"), this);
        title->setStyleSheet(QStringLiteral(
            "color: #ffffff; font-size: 22px; font-weight: 800; letter-spacing: 0.3px;"));
        auto* subtitle = new QLabel(
            QStringLiteral("Тонко налаштуй модель, промпт і вигляд інтерфейсу."),
            this);
        subtitle->setStyleSheet(QStringLiteral(
            "color: #8a99b1; font-size: 12px; letter-spacing: 0.4px;"));
        subtitle->setWordWrap(true);
        auto* head = new QVBoxLayout;
        head->setSpacing(2);
        head->addWidget(eyebrow);
        head->addWidget(title);
        head->addWidget(subtitle);
        root->addLayout(head);
    }

    // ---- Tab widget ----
    auto* tabs = new QTabWidget(this);
    tabs->setDocumentMode(true);
    tabs->setObjectName(QStringLiteral("settingsTabs"));

    auto makeScrollableTab = [&](void (SettingsDialog::*build)(QWidget*),
                                 const QString& label) {
        auto* tab = new QWidget;
        auto* tabRoot = new QVBoxLayout(tab);
        tabRoot->setContentsMargins(0, 8, 0, 0);
        tabRoot->setSpacing(0);

        auto* scroll = new QScrollArea(tab);
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        auto* page = new QWidget(scroll);
        (this->*build)(page);
        scroll->setWidget(page);

        tabRoot->addWidget(scroll);
        tabs->addTab(tab, label);
    };

    makeScrollableTab(&SettingsDialog::buildModelTab,     QStringLiteral("  Модель  "));
    makeScrollableTab(&SettingsDialog::buildSamplingTab,  QStringLiteral("  Семплінг  "));
    makeScrollableTab(&SettingsDialog::buildPersonaTab,   QStringLiteral("  Персона  "));
    makeScrollableTab(&SettingsDialog::buildInterfaceTab, QStringLiteral("  Інтерфейс  "));
    makeScrollableTab(&SettingsDialog::buildServerTab,    QStringLiteral("  Сервер  "));

    root->addWidget(tabs, /*stretch=*/1);

    // ---- Buttons ----
    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Reset, this);
    if (auto* ok = buttons->button(QDialogButtonBox::Ok)) {
        ok->setText(QStringLiteral("Застосувати"));
        ok->setDefault(true);
    }
    if (auto* cancel = buttons->button(QDialogButtonBox::Cancel)) {
        cancel->setText(QStringLiteral("Скасувати"));
    }
    if (auto* reset = buttons->button(QDialogButtonBox::Reset)) {
        reset->setText(QStringLiteral("Скинути"));
        reset->setToolTip(QStringLiteral("Повернути безпечні значення для всіх полів."));
    }
    root->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
        save();
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::clicked, this, [this, buttons](QAbstractButton* b) {
        if (buttons->buttonRole(b) == QDialogButtonBox::ResetRole) resetToDefaults();
    });

    // ---- Stylesheet ----
    // QDialog uses a transparent background so paintEvent() can render the
    // aurora-style scene below the widgets.
    setAttribute(Qt::WA_StyledBackground, false);
    setStyleSheet(QStringLiteral(R"_(
        QDialog { background: transparent; color: #e6edf3; }
        QLabel  { color: #d1d7df; font-size: 13px; background: transparent; }

        QTabWidget::pane {
            border: 1px solid rgba(35, 50, 72, 220);
            border-radius: 14px;
            top: -1px;
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 rgba(8, 17, 28, 175),
                stop:1 rgba(6, 10, 18, 215));
        }
        QTabBar::tab {
            color: #8a99b1;
            background: transparent;
            padding: 9px 20px;
            font-size: 12px;
            font-weight: 700;
            letter-spacing: 1.5px;
            border: 1px solid transparent;
            border-bottom: none;
            border-top-left-radius: 12px;
            border-top-right-radius: 12px;
            margin-right: 4px;
        }
        QTabBar::tab:hover { color: #d1d7df; }
        QTabBar::tab:selected {
            color: #ffffff;
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 rgba(47, 129, 247, 110),
                stop:1 rgba(8, 17, 28, 240));
            border: 1px solid #233248;
            border-bottom: 1px solid #08111c;
        }

        QComboBox, QDoubleSpinBox, QSpinBox, QPlainTextEdit, QLineEdit {
            background-color: #0c1320;
            color: #e6edf3;
            border: 1px solid #233248;
            border-radius: 10px;
            padding: 8px 12px;
            font-size: 13px;
            selection-background-color: #2f81f7;
        }
        QComboBox:focus, QDoubleSpinBox:focus, QSpinBox:focus,
        QPlainTextEdit:focus, QLineEdit:focus {
            border: 1px solid #2f81f7;
        }
        QComboBox::drop-down { border: none; width: 24px; }
        QSpinBox::up-button, QSpinBox::down-button,
        QDoubleSpinBox::up-button, QDoubleSpinBox::down-button {
            background: transparent;
            border: none;
            width: 16px;
        }
        QSpinBox::up-arrow, QDoubleSpinBox::up-arrow {
            image: none;
            border-left: 4px solid transparent;
            border-right: 4px solid transparent;
            border-bottom: 5px solid #58a6ff;
            width: 0; height: 0;
        }
        QSpinBox::down-arrow, QDoubleSpinBox::down-arrow {
            image: none;
            border-left: 4px solid transparent;
            border-right: 4px solid transparent;
            border-top: 5px solid #58a6ff;
            width: 0; height: 0;
        }

        QPushButton {
            background-color: qlineargradient(
                x1:0, y1:0, x2:0, y2:1, stop:0 #182336, stop:1 #0e1626);
            color: #e6edf3;
            border: 1px solid #2a3a52;
            border-radius: 10px;
            padding: 8px 16px;
            font-weight: 600;
        }
        QPushButton:hover  { border-color: #2f81f7; color: #ffffff; }
        QPushButton:default {
            background-color: qlineargradient(
                x1:0, y1:0, x2:0, y2:1, stop:0 #2f81f7, stop:1 #1d6def);
            border: 1px solid rgba(255,255,255,40);
            color: #ffffff;
        }
        QSlider::groove:horizontal {
            background: #182334; height: 6px; border-radius: 3px;
        }
        QSlider::sub-page:horizontal {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #58a6ff, stop:1 #2f81f7);
            border-radius: 3px;
        }
        QSlider::handle:horizontal {
            background: #ffffff; border: 2px solid #2f81f7;
            width: 14px; margin: -5px 0; border-radius: 9px;
        }
        QCheckBox { color: #d1d7df; font-size: 13px; }
        QCheckBox::indicator {
            width: 16px; height: 16px;
            border-radius: 4px;
            border: 1px solid #2a3a52;
            background: #0c1320;
        }
        QCheckBox::indicator:checked {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 #58a6ff, stop:1 #2f81f7);
            border: 1px solid #2f81f7;
        }
        QScrollArea { background: transparent; border: none; }
        QScrollArea > QWidget > QWidget { background: transparent; }
        QTabWidget::tab-bar { alignment: left; }

        QScrollBar:vertical {
            background: transparent; width: 12px; margin: 4px;
        }
        QScrollBar::handle:vertical {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 rgba(120, 140, 170, 130),
                stop:1 rgba(80,  100, 130, 130));
            border-radius: 6px; min-height: 32px;
        }
        QScrollBar::handle:vertical:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #58a6ff, stop:1 #2f81f7);
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }
    )_"));

    populateModelList();
    selectBestDefaultModel();
    load();
}

// =============================================================================
//  Tab builders
// =============================================================================

void SettingsDialog::buildModelTab(QWidget* tab) {
    auto* col = new QVBoxLayout(tab);
    col->setContentsMargins(4, 14, 14, 14);
    col->setSpacing(12);

    // --- Model file card ---
    {
        auto* row = new QWidget(tab);
        auto* lay = new QHBoxLayout(row);
        lay->setContentsMargins(0, 0, 0, 0);
        lay->setSpacing(8);

        m_modelCombo = new QComboBox(row);
        m_modelCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        auto* browse = new QPushButton(QStringLiteral("Огляд…"), row);
        browse->setCursor(Qt::PointingHandCursor);

        lay->addWidget(m_modelCombo, 1);
        lay->addWidget(browse);

        col->addWidget(buildFieldCard(tab, FieldOptions{
            QStringLiteral("Файл моделі"),
            QStringLiteral("GGUF-файл, який JARVIS завантажує в пам'ять. "
                           "У списку — усе з теки ./models поряд із виконуваним "
                           "файлом."),
            QString(),  // no help button
            row,
            nullptr
        }));

        connect(browse, &QPushButton::clicked, this, [this]() {
            const QString file = QFileDialog::getOpenFileName(
                this, QStringLiteral("Обрати GGUF-модель"),
                QString(), QStringLiteral("GGUF Models (*.gguf)"));
            if (file.isEmpty()) return;
            const int existing = m_modelCombo->findData(file);
            if (existing >= 0) {
                m_modelCombo->setCurrentIndex(existing);
                return;
            }
            m_modelCombo->addItem(QFileInfo(file).fileName(), file);
            m_modelCombo->setCurrentIndex(m_modelCombo->count() - 1);
        });
    }

    // --- Context size card ---
    m_contextSpin = new QSpinBox(tab);
    m_contextSpin->setRange(512, 32768);
    m_contextSpin->setSingleStep(256);
    m_contextSpin->setValue(2048);
    col->addWidget(buildFieldCard(tab, FieldOptions{
        QStringLiteral("Розмір контексту"),
        QStringLiteral("Скільки токенів модель тримає в робочій пам'яті: "
                       "промпт + історія + відповідь. Більше = краща пам'ять, "
                       "повільніше і потребує більше RAM."),
        QString(),
        m_contextSpin,
        nullptr
    }));

    // --- GPU layers card ---
    m_gpuLayersSpin = new QSpinBox(tab);
    m_gpuLayersSpin->setRange(0, 999);
    m_gpuLayersSpin->setSingleStep(1);
    m_gpuLayersSpin->setValue(99);
    m_gpuLayersSpin->setSuffix(QStringLiteral("  шарів"));
    col->addWidget(buildFieldCard(tab, FieldOptions{
        QStringLiteral("GPU offload (n_gpu_layers)"),
        QStringLiteral("Скільки шарів моделі вивантажити на відеокарту. "
                       "0 — повністю CPU (повільно). 99 — все, що влізе у "
                       "VRAM (рекомендовано). Якщо JARVIS падає або модель не "
                       "стартує — зменш значення (наприклад, 35) до тих пір, "
                       "поки вистачає VRAM. Потребує білду llama.cpp із CUDA."),
        QString(),
        m_gpuLayersSpin,
        nullptr
    }));

    // --- Prompt template card ---
    m_promptTemplateCombo = new QComboBox(tab);
    m_promptTemplateCombo->addItem(QStringLiteral("Авто (за іменем файлу)"),
                                   int(LlamaWorkerThread::PromptTemplate::Auto));
    m_promptTemplateCombo->addItem(QStringLiteral("ChatML (dolphin / qwen / hermes)"),
                                   int(LlamaWorkerThread::PromptTemplate::ChatML));
    m_promptTemplateCombo->addItem(QStringLiteral("Llama 3 / 3.1 / 3.2"),
                                   int(LlamaWorkerThread::PromptTemplate::Llama3));
    m_promptTemplateCombo->addItem(QStringLiteral("Mistral / Mixtral"),
                                   int(LlamaWorkerThread::PromptTemplate::Mistral));
    m_promptTemplateCombo->addItem(QStringLiteral("Gemma"),
                                   int(LlamaWorkerThread::PromptTemplate::Gemma));
    col->addWidget(buildFieldCard(tab, FieldOptions{
        QStringLiteral("Шаблон промпту"),
        QStringLiteral("У якому форматі обгортати system / user / assistant. "
                       "«Авто» вгадує за іменем файлу. Якщо модель «галюцинує» "
                       "псевдо-теги або відмовляється відповідати — спробуй "
                       "вибрати правильний шаблон вручну."),
        QString(),
        m_promptTemplateCombo,
        nullptr
    }));

    col->addStretch();
}

void SettingsDialog::buildSamplingTab(QWidget* tab) {
    auto* col = new QVBoxLayout(tab);
    col->setContentsMargins(4, 14, 14, 14);
    col->setSpacing(12);

    m_temperatureSpin = new QDoubleSpinBox(tab);
    m_temperatureSpin->setRange(0.0, 2.0);
    m_temperatureSpin->setSingleStep(0.05);
    m_temperatureSpin->setDecimals(2);
    m_temperatureSpin->setValue(0.80);
    col->addWidget(buildFieldCard(tab, {
        QStringLiteral("Temperature"), QString(),
        QStringLiteral(
            "Наскільки <i>випадковими</i> є відповіді. <b>0.0</b> — "
            "детермінований, обирає найбільш імовірний токен (повторюваний). "
            "<b>0.7–0.9</b> — стандарт для чату, баланс між точністю й творчістю. "
            "<b>1.2+</b> — креативно, але часто «маячня». Низькі значення для "
            "коду й фактів, вищі — для прози й мозкового штурму."),
        m_temperatureSpin, nullptr
    }));

    m_topPSpin = new QDoubleSpinBox(tab);
    m_topPSpin->setRange(0.05, 1.0);
    m_topPSpin->setSingleStep(0.05);
    m_topPSpin->setDecimals(2);
    m_topPSpin->setValue(0.95);
    col->addWidget(buildFieldCard(tab, {
        QStringLiteral("Top-P (nucleus)"), QString(),
        QStringLiteral(
            "Залишає лише найменшу множину токенів, чия сумарна ймовірність "
            "не менша за <b>P</b>. <b>1.0</b> — розглядати всі токени. "
            "<b>0.9–0.95</b> — класичний nucleus-sampling, гарний дефолт. "
            "Менші значення дають безпечніші, точніші відповіді; вищі — "
            "пропускають рідкісні слова."),
        m_topPSpin, nullptr
    }));

    m_topKSpin = new QSpinBox(tab);
    m_topKSpin->setRange(1, 500);
    m_topKSpin->setValue(40);
    col->addWidget(buildFieldCard(tab, {
        QStringLiteral("Top-K"), QString(),
        QStringLiteral(
            "Брати лише <b>K</b> найімовірніших наступних токенів. <b>40</b> — "
            "консервативний дефолт. Менше (наприклад, 10) робить модель "
            "передбачуванішою; більше (100+) дає ширший словник. Працює разом "
            "із Top-P — спрацьовує те, що жорсткіше."),
        m_topKSpin, nullptr
    }));

    m_minPSpin = new QDoubleSpinBox(tab);
    m_minPSpin->setRange(0.0, 0.5);
    m_minPSpin->setSingleStep(0.01);
    m_minPSpin->setDecimals(2);
    m_minPSpin->setValue(0.10);
    col->addWidget(buildFieldCard(tab, {
        QStringLiteral("Min-P"), QString(),
        QStringLiteral(
            "Відкидає токен, якщо його ймовірність нижча за "
            "<b>min_p × max(p)</b>. Сучасна альтернатива Top-P, яка адаптується "
            "до впевненості моделі. <b>0.05–0.10</b> добре працює для більшості "
            "чат-сценаріїв. <b>0</b> — вимкнути."),
        m_minPSpin, nullptr
    }));

    m_repeatPenaltySpin = new QDoubleSpinBox(tab);
    m_repeatPenaltySpin->setRange(1.0, 2.0);
    m_repeatPenaltySpin->setSingleStep(0.05);
    m_repeatPenaltySpin->setDecimals(2);
    m_repeatPenaltySpin->setValue(1.10);
    col->addWidget(buildFieldCard(tab, {
        QStringLiteral("Штраф за повтор"), QString(),
        QStringLiteral(
            "Множник для токенів, які вже з'явилися нещодавно. <b>1.0</b> — "
            "вимкнено. <b>1.05–1.20</b> зменшує повторення без шкоди для "
            "граматики. Завищене значення (>1.3) псує зв'язність — модель "
            "починає уникати поширених слів."),
        m_repeatPenaltySpin, nullptr
    }));

    m_maxTokensSpin = new QSpinBox(tab);
    m_maxTokensSpin->setRange(32, 8192);
    m_maxTokensSpin->setSingleStep(32);
    m_maxTokensSpin->setValue(1024);
    col->addWidget(buildFieldCard(tab, {
        QStringLiteral("Макс. довжина відповіді"), QString(),
        QStringLiteral(
            "Жорсткий ліміт токенів на одну відповідь. Генерація також "
            "зупиняється, коли модель емітить токен кінця ходу. Зменши, якщо "
            "відповіді задовгі; збільш для розгорнутих текстів."),
        m_maxTokensSpin, nullptr
    }));

    col->addStretch();
}

void SettingsDialog::buildPersonaTab(QWidget* tab) {
    auto* col = new QVBoxLayout(tab);
    col->setContentsMargins(4, 14, 14, 14);
    col->setSpacing(12);

    // --- Profile card: avatar + name in one card ---
    {
        auto* card = makeCard(tab);
        auto* lay = new QVBoxLayout(card);
        lay->setContentsMargins(18, 16, 18, 16);
        lay->setSpacing(14);

        auto* sectionTitle = new QLabel(QStringLiteral("Профіль"), card);
        sectionTitle->setStyleSheet(QStringLiteral(
            "color: #e6edf3; font-size: 13.5px; font-weight: 700;"));

        auto* sectionSub = new QLabel(
            QStringLiteral("Фото та ім'я використовуються в нижній панелі "
                           "та поряд із твоїми повідомленнями."), card);
        sectionSub->setStyleSheet(QStringLiteral(
            "color: #8a99b1; font-size: 11.5px;"));
        sectionSub->setWordWrap(true);

        // --- Avatar row: preview + buttons ---
        auto* avatarRow = new QHBoxLayout;
        avatarRow->setContentsMargins(0, 4, 0, 0);
        avatarRow->setSpacing(16);

        auto* avatarLabel = new QLabel(card);
        paintAvatarLabel(avatarLabel, 88);

        auto* avatarBtnsCol = new QVBoxLayout;
        avatarBtnsCol->setSpacing(8);

        auto* uploadBtn = new QPushButton(
            QStringLiteral("Завантажити фото…"), card);
        uploadBtn->setCursor(Qt::PointingHandCursor);
        auto* removeBtn = new QPushButton(
            QStringLiteral("Видалити фото"), card);
        removeBtn->setCursor(Qt::PointingHandCursor);

        avatarBtnsCol->addWidget(uploadBtn);
        avatarBtnsCol->addWidget(removeBtn);
        avatarBtnsCol->addStretch();

        avatarRow->addWidget(avatarLabel, 0, Qt::AlignTop);
        avatarRow->addLayout(avatarBtnsCol, 1);

        connect(uploadBtn, &QPushButton::clicked, this, [this, avatarLabel]() {
            const QString src = QFileDialog::getOpenFileName(
                this, QStringLiteral("Обрати фото"),
                QString(),
                QStringLiteral("Зображення (*.png *.jpg *.jpeg *.bmp *.webp)"));
            if (src.isEmpty()) return;
            if (WelcomeDialog::persistAvatarFromFile(src)) {
                paintAvatarLabel(avatarLabel, avatarLabel->width());
            }
        });
        connect(removeBtn, &QPushButton::clicked, this, [avatarLabel]() {
            WelcomeDialog::clearAvatar();
            paintAvatarLabel(avatarLabel, avatarLabel->width());
        });

        // --- Name field ---
        auto* nameTitle = new QLabel(QStringLiteral("Твоє ім'я"), card);
        nameTitle->setStyleSheet(QStringLiteral(
            "color: #e6edf3; font-size: 13px; font-weight: 700;"));

        m_userNameEdit = new QLineEdit(card);
        m_userNameEdit->setPlaceholderText(QStringLiteral("Наприклад: Олександр"));
        m_userNameEdit->setMaxLength(40);

        lay->addWidget(sectionTitle);
        lay->addWidget(sectionSub);
        lay->addLayout(avatarRow);
        lay->addSpacing(4);
        lay->addWidget(nameTitle);
        lay->addWidget(m_userNameEdit);

        col->addWidget(card);
    }

    // --- System prompt card ---
    m_systemPromptEdit = new QPlainTextEdit(tab);
    m_systemPromptEdit->setPlaceholderText(
        QStringLiteral("Залиш порожнім, щоб використати вбудований промпт JARVIS."));
    m_systemPromptEdit->setMinimumHeight(220);
    m_systemPromptEdit->setPlainText(Config::SYSTEM_PROMPT);
    col->addWidget(buildFieldCard(tab, {
        QStringLiteral("Системний промпт"),
        QStringLiteral("Приховані інструкції, які модель читає перед "
                       "кожною розмовою."),
        QStringLiteral(
            "Приховані інструкції, які модель читає <i>перед</i> кожною "
            "розмовою. Визначають персону, тон, доступні інструменти "
            "(теги <code>[CMD: …]</code>) і правила. Порожньо — буде "
            "використано вбудований промпт. <b>Редагуй обережно</b> — "
            "поганий промпт = погана модель."),
        m_systemPromptEdit, nullptr
    }));

    col->addStretch();
}

void SettingsDialog::buildInterfaceTab(QWidget* tab) {
    auto* col = new QVBoxLayout(tab);
    col->setContentsMargins(4, 14, 14, 14);
    col->setSpacing(12);

    m_accentCombo = new QComboBox(tab);
    m_accentCombo->addItem(QStringLiteral("Aurora Blue (за замовчуванням)"), QColor( 47, 129, 247));
    m_accentCombo->addItem(QStringLiteral("Cyan Pulse"),                    QColor( 56, 189, 248));
    m_accentCombo->addItem(QStringLiteral("Iris Violet"),                   QColor(139, 110, 246));
    m_accentCombo->addItem(QStringLiteral("Emerald Core"),                  QColor( 52, 211, 153));
    m_accentCombo->addItem(QStringLiteral("Amber Warning"),                 QColor(245, 158,  11));
    m_accentCombo->addItem(QStringLiteral("Crimson"),                       QColor(244,  63,  94));
    col->addWidget(buildFieldCard(tab, {
        QStringLiteral("Акцентний колір"),
        QStringLiteral("Перефарбовує фон, рамку поля вводу та кнопку "
                       "відправки."),
        QString(),
        m_accentCombo, nullptr
    }));

    m_opacitySlider = new QSlider(Qt::Horizontal, tab);
    m_opacitySlider->setRange(70, 100);
    m_opacitySlider->setValue(100);
    m_opacitySlider->setTickPosition(QSlider::TicksBelow);
    m_opacitySlider->setTickInterval(5);
    col->addWidget(buildFieldCard(tab, {
        QStringLiteral("Прозорість вікна"),
        QStringLiteral("100 % — повністю непрозоре. Менші значення дозволяють "
                       "робочому столу проглядатись крізь вікно."),
        QString(),
        m_opacitySlider, nullptr
    }));

    m_timestampsCheck = new QCheckBox(QStringLiteral("Показувати час біля повідомлень"), tab);
    m_timestampsCheck->setChecked(true);
    col->addWidget(buildFieldCard(tab, {
        QStringLiteral("Часові мітки"),
        QStringLiteral("Маленький час HH:mm над кожним повідомленням."),
        QString(),
        m_timestampsCheck, nullptr
    }));

    col->addStretch();
}

// =============================================================================
//  Reset / Persistence
// =============================================================================

void SettingsDialog::resetToDefaults() {
    if (m_temperatureSpin)   m_temperatureSpin->setValue(0.80);
    if (m_contextSpin)       m_contextSpin->setValue(2048);
    if (m_topPSpin)          m_topPSpin->setValue(0.95);
    if (m_topKSpin)          m_topKSpin->setValue(40);
    if (m_minPSpin)          m_minPSpin->setValue(0.10);
    if (m_repeatPenaltySpin) m_repeatPenaltySpin->setValue(1.10);
    if (m_maxTokensSpin)     m_maxTokensSpin->setValue(1024);
    if (m_gpuLayersSpin)     m_gpuLayersSpin->setValue(99);
    if (m_promptTemplateCombo) m_promptTemplateCombo->setCurrentIndex(0);
    if (m_systemPromptEdit)  m_systemPromptEdit->setPlainText(Config::SYSTEM_PROMPT);
    if (m_userNameEdit)      m_userNameEdit->clear();
    if (m_accentCombo)       m_accentCombo->setCurrentIndex(0);
    if (m_opacitySlider)     m_opacitySlider->setValue(100);
    if (m_timestampsCheck)   m_timestampsCheck->setChecked(true);
    if (m_serverEnableCheck) m_serverEnableCheck->setChecked(false);
    if (m_serverPortSpin)    m_serverPortSpin->setValue(17320);
    if (m_serverPinEdit)     m_serverPinEdit->clear();
    refreshServerUrls();
}

void SettingsDialog::save() const {
    QSettings s;
    s.setValue(kK_Model,        getSelectedModel());
    s.setValue(kK_Temperature,  static_cast<double>(getTemperature()));
    s.setValue(kK_Context,      getContextSize());
    s.setValue(kK_TopP,         m_topPSpin->value());
    s.setValue(kK_TopK,         m_topKSpin->value());
    s.setValue(kK_MinP,         m_minPSpin->value());
    s.setValue(kK_RepeatPen,    m_repeatPenaltySpin->value());
    s.setValue(kK_MaxTokens,    m_maxTokensSpin->value());
    s.setValue(kK_GpuLayers,    m_gpuLayersSpin ? m_gpuLayersSpin->value() : 99);
    s.setValue(kK_PromptTpl,    m_promptTemplateCombo ? m_promptTemplateCombo->currentData().toInt() : 0);
    s.setValue(kK_SystemPrompt, m_systemPromptEdit->toPlainText());
    s.setValue(kK_AccentRgb,    getAccentColor().rgb());
    s.setValue(kK_Opacity,      m_opacitySlider->value());
    s.setValue(kK_Timestamps,   m_timestampsCheck->isChecked());
    s.setValue(kK_UserName,     getUserName());
    if (m_serverEnableCheck)
        s.setValue(kK_ServerEnabled, m_serverEnableCheck->isChecked());
    if (m_serverPortSpin)
        s.setValue(kK_ServerPort,    m_serverPortSpin->value());
    if (m_serverPinEdit)
        s.setValue(kK_ServerPin,     m_serverPinEdit->text().trimmed());
}

void SettingsDialog::load() {
    QSettings s;

    if (s.contains(kK_Model)) {
        const QString p = s.value(kK_Model).toString();
        const int idx = m_modelCombo->findData(p);
        if (idx >= 0) m_modelCombo->setCurrentIndex(idx);
        else if (QFileInfo::exists(p)) {
            m_modelCombo->addItem(QFileInfo(p).fileName(), p);
            m_modelCombo->setCurrentIndex(m_modelCombo->count() - 1);
        }
    }

    if (s.contains(kK_Temperature))
        m_temperatureSpin->setValue(s.value(kK_Temperature).toDouble());
    if (s.contains(kK_Context))
        m_contextSpin->setValue(s.value(kK_Context).toInt());
    if (s.contains(kK_TopP))
        m_topPSpin->setValue(s.value(kK_TopP).toDouble());
    if (s.contains(kK_TopK))
        m_topKSpin->setValue(s.value(kK_TopK).toInt());
    if (s.contains(kK_MinP))
        m_minPSpin->setValue(s.value(kK_MinP).toDouble());
    if (s.contains(kK_RepeatPen))
        m_repeatPenaltySpin->setValue(s.value(kK_RepeatPen).toDouble());
    if (s.contains(kK_MaxTokens))
        m_maxTokensSpin->setValue(s.value(kK_MaxTokens).toInt());
    if (s.contains(kK_GpuLayers) && m_gpuLayersSpin)
        m_gpuLayersSpin->setValue(s.value(kK_GpuLayers).toInt());
    if (s.contains(kK_PromptTpl) && m_promptTemplateCombo) {
        const int v = s.value(kK_PromptTpl).toInt();
        const int idx = m_promptTemplateCombo->findData(v);
        if (idx >= 0) m_promptTemplateCombo->setCurrentIndex(idx);
    }
    if (s.contains(kK_SystemPrompt)) {
        const QString prompt = s.value(kK_SystemPrompt).toString();
        if (!prompt.isEmpty()) m_systemPromptEdit->setPlainText(prompt);
    }
    if (s.contains(kK_AccentRgb)) {
        const QRgb rgb = static_cast<QRgb>(s.value(kK_AccentRgb).toUInt());
        for (int i = 0; i < m_accentCombo->count(); ++i) {
            if (m_accentCombo->itemData(i).value<QColor>().rgb() == rgb) {
                m_accentCombo->setCurrentIndex(i);
                break;
            }
        }
    }
    if (s.contains(kK_Opacity))
        m_opacitySlider->setValue(s.value(kK_Opacity).toInt());
    if (s.contains(kK_Timestamps))
        m_timestampsCheck->setChecked(s.value(kK_Timestamps).toBool());
    if (s.contains(kK_UserName))
        m_userNameEdit->setText(s.value(kK_UserName).toString());

    if (m_serverEnableCheck)
        m_serverEnableCheck->setChecked(
            s.value(kK_ServerEnabled, false).toBool());
    if (m_serverPortSpin)
        m_serverPortSpin->setValue(
            s.value(kK_ServerPort, 17320).toInt());
    if (m_serverPinEdit)
        m_serverPinEdit->setText(s.value(kK_ServerPin).toString());
    refreshServerUrls();
}

// =============================================================================
//  Getters
// =============================================================================

LlamaWorkerThread::GenParams SettingsDialog::getGenParams() const {
    LlamaWorkerThread::GenParams p;
    p.temperature   = static_cast<float>(m_temperatureSpin->value());
    p.contextSize   = m_contextSpin->value();
    p.topP          = static_cast<float>(m_topPSpin->value());
    p.topK          = m_topKSpin->value();
    p.minP          = static_cast<float>(m_minPSpin->value());
    p.repeatPenalty = static_cast<float>(m_repeatPenaltySpin->value());
    p.maxTokens     = m_maxTokensSpin->value();
    p.gpuLayers     = m_gpuLayersSpin ? m_gpuLayersSpin->value() : 99;
    if (m_promptTemplateCombo) {
        p.promptTemplate = static_cast<LlamaWorkerThread::PromptTemplate>(
            m_promptTemplateCombo->currentData().toInt());
    }
    return p;
}

QString SettingsDialog::getSystemPromptOverride() const {
    const QString text = m_systemPromptEdit->toPlainText();
    if (text == Config::SYSTEM_PROMPT) return QString();
    return text;
}

float    SettingsDialog::getTemperature()   const { return static_cast<float>(m_temperatureSpin->value()); }
int      SettingsDialog::getContextSize()   const { return m_contextSpin->value(); }
QString  SettingsDialog::getSelectedModel() const { return m_modelCombo->currentData().toString(); }

QColor   SettingsDialog::getAccentColor()   const { return m_accentCombo->currentData().value<QColor>(); }
double   SettingsDialog::getWindowOpacity() const { return m_opacitySlider->value() / 100.0; }
bool     SettingsDialog::showTimestamps()   const { return m_timestampsCheck->isChecked(); }
QString  SettingsDialog::getUserName()      const { return m_userNameEdit ? m_userNameEdit->text().trimmed() : QString(); }

// =============================================================================
//  Model discovery
// =============================================================================

void SettingsDialog::populateModelList() {
    QStringList roots = {
        QDir::current().absoluteFilePath(QStringLiteral("models")),
        QCoreApplication::applicationDirPath() + QStringLiteral("/models"),
        QCoreApplication::applicationDirPath() + QStringLiteral("/../models"),
    };
    QStringList seen;
    for (const QString& root : roots) {
        QDir d(root);
        if (!d.exists()) continue;
        const QFileInfoList entries = d.entryInfoList(
            QStringList{ QStringLiteral("*.gguf") }, QDir::Files, QDir::Name);
        for (const QFileInfo& fi : entries) {
            const QString abs = fi.absoluteFilePath();
            if (seen.contains(abs)) continue;
            seen << abs;
            m_modelCombo->addItem(fi.fileName(), abs);
        }
    }
    if (m_modelCombo->count() == 0) {
        m_modelCombo->addItem(QStringLiteral("(no GGUF files found in ./models)"), QString());
    }
}

void SettingsDialog::selectBestDefaultModel() {
    if (!m_modelCombo || m_modelCombo->count() == 0) return;
    for (int i = 0; i < m_modelCombo->count(); ++i) {
        const QString name = m_modelCombo->itemText(i).toLower();
        if (name.contains(QStringLiteral("dolphin"))) {
            m_modelCombo->setCurrentIndex(i);
            return;
        }
    }
    m_modelCombo->setCurrentIndex(0);
}

// =============================================================================
//  paintEvent — premium dark aurora background
// =============================================================================

void SettingsDialog::paintEvent(QPaintEvent* /*e*/) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);

    const QRectF r = rect();

    // 1) Base obsidian gradient.
    {
        QLinearGradient base(QPointF(0, 0), QPointF(0, r.height()));
        base.setColorAt(0.0, QColor(0x0d, 0x14, 0x1d));
        base.setColorAt(1.0, QColor(0x05, 0x08, 0x0e));
        p.fillRect(r, base);
    }

    // 2) Top-left accent blob (cool blue glow).
    {
        QRadialGradient blob(QPointF(r.width() * 0.18, r.height() * 0.10),
                             r.width() * 0.55);
        blob.setColorAt(0.00, QColor(47, 129, 247, 70));
        blob.setColorAt(0.55, QColor(47, 129, 247, 18));
        blob.setColorAt(1.00, QColor(47, 129, 247,  0));
        p.fillRect(r, blob);
    }

    // 3) Bottom-right cyan whisper.
    {
        QRadialGradient blob(QPointF(r.width() * 0.85, r.height() * 0.92),
                             r.width() * 0.65);
        blob.setColorAt(0.00, QColor(56, 189, 248, 50));
        blob.setColorAt(0.55, QColor(56, 189, 248, 14));
        blob.setColorAt(1.00, QColor(56, 189, 248,  0));
        p.fillRect(r, blob);
    }

    // 4) Subtle vignette so card edges feel rooted.
    {
        QRadialGradient vignette(QPointF(r.width() * 0.5, r.height() * 0.5),
                                 r.width() * 0.85);
        vignette.setColorAt(0.55, QColor(0, 0, 0,   0));
        vignette.setColorAt(1.00, QColor(0, 0, 0, 130));
        p.fillRect(r, vignette);
    }

    // Hairline top accent — replaces the harsh blue stripe with a soft glow.
    {
        QLinearGradient stroke(QPointF(0, 0), QPointF(r.width(), 0));
        stroke.setColorAt(0.0, QColor(47, 129, 247,   0));
        stroke.setColorAt(0.5, QColor(47, 129, 247, 110));
        stroke.setColorAt(1.0, QColor(47, 129, 247,   0));
        p.fillRect(QRectF(0, 0, r.width(), 1), stroke);
    }
}

// =============================================================================
//  Server tab — LAN HTTP control panel for the phone
// =============================================================================

void SettingsDialog::buildServerTab(QWidget* tab) {
    auto* col = new QVBoxLayout(tab);
    col->setContentsMargins(4, 14, 14, 14);
    col->setSpacing(12);

    // ---- 1. Enable toggle ----
    m_serverEnableCheck = new QCheckBox(
        QStringLiteral("Запускати веб-сервер разом із JARVIS"), tab);
    col->addWidget(buildFieldCard(tab, FieldOptions{
        QStringLiteral("Веб-керування з телефону"),
        QStringLiteral("JARVIS підіймає міні-сервер у твоїй локальній мережі. "
                       "З телефону, що в тій самій Wi-Fi, відкриваєш URL нижче "
                       "і керуєш ПК у браузері."),
        QString(),
        m_serverEnableCheck, nullptr
    }));

    // ---- 2. Port ----
    m_serverPortSpin = new QSpinBox(tab);
    m_serverPortSpin->setRange(1024, 65535);
    m_serverPortSpin->setValue(17320);
    col->addWidget(buildFieldCard(tab, FieldOptions{
        QStringLiteral("Порт"),
        QStringLiteral("За замовчуванням 17320. Зміни лише якщо порт зайнятий "
                       "іншою програмою."),
        QString(),
        m_serverPortSpin, nullptr
    }));

    // ---- 3. PIN ----
    m_serverPinEdit = new QLineEdit(tab);
    m_serverPinEdit->setPlaceholderText(
        QStringLiteral("(порожньо = без авторизації)"));
    m_serverPinEdit->setClearButtonEnabled(true);
    auto* genPinBtn = new QPushButton(QStringLiteral("Згенерувати"), tab);
    genPinBtn->setCursor(Qt::PointingHandCursor);
    connect(genPinBtn, &QPushButton::clicked, this, [this]() {
        // 6-digit PIN — easy to type from a phone yet still ~1M permutations.
        const quint32 v = QRandomGenerator::global()->bounded(1000000u);
        m_serverPinEdit->setText(QString::asprintf("%06u", v));
    });
    auto* pinRow = new QHBoxLayout;
    pinRow->setContentsMargins(0, 0, 0, 0);
    pinRow->setSpacing(8);
    pinRow->addWidget(m_serverPinEdit, 1);
    pinRow->addWidget(genPinBtn, 0);
    auto* pinHolder = new QWidget(tab);
    pinHolder->setLayout(pinRow);
    col->addWidget(buildFieldCard(tab, FieldOptions{
        QStringLiteral("PIN-авторизація"),
        QStringLiteral("Цифровий код, що його телефон надсилатиме в заголовку "
                       "X-JARVIS-PIN. Якщо порожньо — будь-хто в LAN зможе "
                       "керувати ПК. Рекомендую 6 цифр."),
        QString(),
        pinHolder, nullptr
    }));

    // ---- 4. URL preview + Open in browser ----
    m_serverUrlsView = new QPlainTextEdit(tab);
    m_serverUrlsView->setReadOnly(true);
    m_serverUrlsView->setMinimumHeight(72);
    m_serverUrlsView->setFrameShape(QFrame::NoFrame);
    m_serverUrlsView->setStyleSheet(QStringLiteral(
        "QPlainTextEdit {"
        "  background: rgba(8,12,18,0.65);"
        "  color: #58a6ff;"
        "  border: 1px solid rgba(60,78,102,0.55);"
        "  border-radius: 10px;"
        "  padding: 10px 12px;"
        "  font-family: 'Cascadia Mono','Consolas','Menlo',monospace;"
        "  font-size: 12.5px;"
        "}"));
    auto* openBtn = new QPushButton(
        QStringLiteral("Відкрити в браузері"), tab);
    openBtn->setCursor(Qt::PointingHandCursor);
    connect(openBtn, &QPushButton::clicked, this, [this]() {
        const QStringList urls = JarvisHttpServer::lanUrls(
            static_cast<quint16>(m_serverPortSpin
                                 ? m_serverPortSpin->value() : 17320));
        const QString first = urls.isEmpty()
            ? QStringLiteral("http://127.0.0.1:%1").arg(
                m_serverPortSpin ? m_serverPortSpin->value() : 17320)
            : urls.first();
        QDesktopServices::openUrl(QUrl(first));
    });
    auto* urlsHolder = new QWidget(tab);
    auto* urlsLay    = new QVBoxLayout(urlsHolder);
    urlsLay->setContentsMargins(0, 0, 0, 0);
    urlsLay->setSpacing(8);
    urlsLay->addWidget(m_serverUrlsView);
    urlsLay->addWidget(openBtn, 0, Qt::AlignLeft);
    col->addWidget(buildFieldCard(tab, FieldOptions{
        QStringLiteral("Адреси для телефону"),
        QStringLiteral("Відкрий одну з цих адрес у браузері телефона (Wi-Fi "
                       "тієї ж мережі). Список перебудовується при зміні "
                       "порту або мережевих інтерфейсів."),
        QString(),
        urlsHolder, nullptr
    }));

    // Live updates as the user changes port/enable.
    connect(m_serverPortSpin,
            QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int) { refreshServerUrls(); });

    col->addStretch();
}

void SettingsDialog::refreshServerUrls() {
    if (!m_serverUrlsView) return;
    const quint16 p = static_cast<quint16>(
        m_serverPortSpin ? m_serverPortSpin->value() : 17320);
    QStringList urls = JarvisHttpServer::lanUrls(p);
    if (urls.isEmpty()) {
        m_serverUrlsView->setPlainText(QStringLiteral(
            "(не знайдено активних мережевих інтерфейсів IPv4 — "
            "переконайся, що Wi-Fi/Ethernet увімкнено)"));
        return;
    }
    m_serverUrlsView->setPlainText(urls.join('\n'));
}
