#include "SettingsDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSettings>
#include <QSizePolicy>
#include <QSlider>
#include <QSpinBox>
#include <QTabWidget>
#include <QToolButton>
#include <QToolTip>
#include <QVBoxLayout>
#include <QWidget>

#include "../ai/SystemPrompt.h"

namespace {

// =============================================================================
//  Local helpers (HelpButton + withHelp)
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

// Wrap a control in a row with a "?" button on the right.
QWidget* withHelp(QWidget* control, const QString& description) {
    auto* host = new QWidget(control->parentWidget());
    auto* row  = new QHBoxLayout(host);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(8);
    row->addWidget(control, 1);
    row->addWidget(new HelpButton(description, host), 0, Qt::AlignVCenter);
    return host;
}

// QSettings helpers — scoped under "settings/<key>" so they don't collide
// with WelcomeDialog's "user/name".
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

} // namespace

// =============================================================================
//  SettingsDialog
// =============================================================================

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("JARVIS · Налаштування"));
    setModal(true);
    resize(720, 720);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(20, 20, 20, 16);
    root->setSpacing(14);

    // ---- Title strip ----
    {
        auto* title = new QLabel(QStringLiteral("JARVIS · ПАНЕЛЬ КЕРУВАННЯ"), this);
        title->setStyleSheet(QStringLiteral(
            "color: #ffffff; font-size: 18px; font-weight: 800; letter-spacing: 3px;"));
        auto* subtitle = new QLabel(
            QStringLiteral("Налаштуй модель, промпт і вигляд інтерфейсу. "
                           "Біля кожного поля кнопка «?» — натисни, щоб прочитати опис."), this);
        subtitle->setStyleSheet(QStringLiteral(
            "color: #8a99b1; font-size: 12px; letter-spacing: 0.5px;"));
        subtitle->setWordWrap(true);
        auto* head = new QVBoxLayout;
        head->setSpacing(2);
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
    setStyleSheet(QStringLiteral(R"_(
        QDialog {
            background-color: #06090d;
            color: #e6edf3;
        }
        QLabel { color: #d1d7df; font-size: 13px; }

        QTabWidget::pane {
            border: 1px solid #182334;
            border-radius: 12px;
            top: -1px;
            background: #08111c;
        }
        QTabBar::tab {
            color: #8a99b1;
            background: transparent;
            padding: 9px 18px;
            font-size: 12px;
            font-weight: 700;
            letter-spacing: 1.5px;
            border: 1px solid transparent;
            border-bottom: none;
            border-top-left-radius: 10px;
            border-top-right-radius: 10px;
            margin-right: 4px;
        }
        QTabBar::tab:hover { color: #d1d7df; }
        QTabBar::tab:selected {
            color: #ffffff;
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 rgba(47, 129, 247, 80),
                stop:1 rgba(13, 19, 28, 230));
            border: 1px solid #233248;
            border-bottom: 1px solid #08111c;
        }

        QComboBox, QDoubleSpinBox, QSpinBox, QPlainTextEdit, QLineEdit {
            background-color: #0f1620;
            color: #e6edf3;
            border: 1px solid #233248;
            border-radius: 10px;
            padding: 6px 10px;
            selection-background-color: #2f81f7;
        }
        QComboBox:focus, QDoubleSpinBox:focus, QSpinBox:focus,
        QPlainTextEdit:focus, QLineEdit:focus {
            border: 1px solid #2f81f7;
        }
        QComboBox::drop-down { border: none; width: 22px; }
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
        QCheckBox { color: #d1d7df; }
        QCheckBox::indicator {
            width: 16px; height: 16px;
            border-radius: 4px;
            border: 1px solid #2a3a52;
            background: #0f1620;
        }
        QCheckBox::indicator:checked {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 #58a6ff, stop:1 #2f81f7);
            border: 1px solid #2f81f7;
        }
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
    auto* form = new QFormLayout(tab);
    form->setContentsMargins(8, 12, 18, 12);
    form->setHorizontalSpacing(14);
    form->setVerticalSpacing(12);

    // Model file + Browse
    {
        auto* host = new QWidget(tab);
        auto* row  = new QHBoxLayout(host);
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(8);

        m_modelCombo = new QComboBox(tab);
        m_modelCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        auto* browse = new QPushButton(QStringLiteral("Огляд…"), tab);
        browse->setCursor(Qt::PointingHandCursor);

        row->addWidget(m_modelCombo, 1);
        row->addWidget(browse);

        form->addRow(QStringLiteral("Файл моделі"), withHelp(host, QStringLiteral(
            "Файл <b>GGUF</b>, який JARVIS завантажить у пам'ять. У списку — "
            "усі <code>*.gguf</code> у теці <code>./models</code> поряд із "
            "виконуваним файлом. Натисни <i>Огляд…</i>, щоб обрати файл "
            "із будь-якої теки. Зміна моделі викликає повне перезавантаження — "
            "це триває кілька секунд.")));

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

    m_contextSpin = new QSpinBox(tab);
    m_contextSpin->setRange(512, 32768);
    m_contextSpin->setSingleStep(256);
    m_contextSpin->setValue(2048);
    form->addRow(QStringLiteral("Розмір контексту"), withHelp(m_contextSpin, QStringLiteral(
        "Скільки токенів модель тримає в робочій пам'яті одночасно "
        "(промпт + історія + відповідь). Більший контекст = більше "
        "пам'ятає, але потребує більше RAM і генерує повільніше. "
        "Більшість чат-моделей добре працюють на <b>2048–4096</b>. "
        "Зміна цього значення тягне за собою перезавантаження моделі.")));

    form->addRow(new QWidget); // bottom spacer
}

void SettingsDialog::buildSamplingTab(QWidget* tab) {
    auto* form = new QFormLayout(tab);
    form->setContentsMargins(8, 12, 18, 12);
    form->setHorizontalSpacing(14);
    form->setVerticalSpacing(12);

    m_temperatureSpin = new QDoubleSpinBox(tab);
    m_temperatureSpin->setRange(0.0, 2.0);
    m_temperatureSpin->setSingleStep(0.05);
    m_temperatureSpin->setDecimals(2);
    m_temperatureSpin->setValue(0.80);
    form->addRow(QStringLiteral("Temperature"), withHelp(m_temperatureSpin, QStringLiteral(
        "Наскільки <i>випадковими</i> є відповіді. <b>0.0</b> — "
        "детермінований, обирає найбільш імовірний токен (повторюваний). "
        "<b>0.7–0.9</b> — стандарт для чату, баланс між точністю й творчістю. "
        "<b>1.2+</b> — креативно, але часто «маячня». Низькі значення для "
        "коду й фактів, вищі — для прози й мозкового штурму.")));

    m_topPSpin = new QDoubleSpinBox(tab);
    m_topPSpin->setRange(0.05, 1.0);
    m_topPSpin->setSingleStep(0.05);
    m_topPSpin->setDecimals(2);
    m_topPSpin->setValue(0.95);
    form->addRow(QStringLiteral("Top-P (nucleus)"), withHelp(m_topPSpin, QStringLiteral(
        "Залишає лише найменшу множину токенів, чия сумарна ймовірність "
        "не менша за <b>P</b>. <b>1.0</b> — розглядати всі токени. "
        "<b>0.9–0.95</b> — класичний nucleus-sampling, гарний дефолт. "
        "Менші значення дають безпечніші, точніші відповіді; вищі — "
        "пропускають рідкісні слова.")));

    m_topKSpin = new QSpinBox(tab);
    m_topKSpin->setRange(1, 500);
    m_topKSpin->setValue(40);
    form->addRow(QStringLiteral("Top-K"), withHelp(m_topKSpin, QStringLiteral(
        "Брати лише <b>K</b> найімовірніших наступних токенів. <b>40</b> — "
        "консервативний дефолт. Менше (наприклад, 10) робить модель "
        "передбачуванішою; більше (100+) дає ширший словник. Працює разом "
        "із Top-P — спрацьовує те, що жорсткіше.")));

    m_minPSpin = new QDoubleSpinBox(tab);
    m_minPSpin->setRange(0.0, 0.5);
    m_minPSpin->setSingleStep(0.01);
    m_minPSpin->setDecimals(2);
    m_minPSpin->setValue(0.10);
    form->addRow(QStringLiteral("Min-P"), withHelp(m_minPSpin, QStringLiteral(
        "Відкидає токен, якщо його ймовірність нижча за "
        "<b>min_p × max(p)</b>. Сучасна альтернатива Top-P, яка адаптується "
        "до впевненості моделі. <b>0.05–0.10</b> добре працює для більшості "
        "чат-сценаріїв. <b>0</b> — вимкнути.")));

    m_repeatPenaltySpin = new QDoubleSpinBox(tab);
    m_repeatPenaltySpin->setRange(1.0, 2.0);
    m_repeatPenaltySpin->setSingleStep(0.05);
    m_repeatPenaltySpin->setDecimals(2);
    m_repeatPenaltySpin->setValue(1.10);
    form->addRow(QStringLiteral("Штраф за повтор"), withHelp(m_repeatPenaltySpin, QStringLiteral(
        "Множник для токенів, які вже з'явилися нещодавно. <b>1.0</b> — "
        "вимкнено. <b>1.05–1.20</b> зменшує повторення без шкоди для "
        "граматики. Завищене значення (>1.3) псує зв'язність — модель "
        "починає уникати поширених слів.")));

    m_maxTokensSpin = new QSpinBox(tab);
    m_maxTokensSpin->setRange(32, 8192);
    m_maxTokensSpin->setSingleStep(32);
    m_maxTokensSpin->setValue(1024);
    form->addRow(QStringLiteral("Макс. довжина відповіді"),
        withHelp(m_maxTokensSpin, QStringLiteral(
        "Жорсткий ліміт токенів на одну відповідь. Генерація також "
        "зупиняється, коли модель емітить токен кінця ходу. Зменши, якщо "
        "відповіді задовгі; збільш для розгорнутих текстів.")));

    form->addRow(new QWidget);
}

void SettingsDialog::buildPersonaTab(QWidget* tab) {
    auto* form = new QFormLayout(tab);
    form->setContentsMargins(8, 12, 18, 12);
    form->setHorizontalSpacing(14);
    form->setVerticalSpacing(12);

    m_userNameEdit = new QLineEdit(tab);
    m_userNameEdit->setPlaceholderText(QStringLiteral("Наприклад: Олександр"));
    m_userNameEdit->setMaxLength(40);
    form->addRow(QStringLiteral("Твоє ім'я"), withHelp(m_userNameEdit, QStringLiteral(
        "Як JARVIS звертатиметься до тебе. Це ім'я також виводиться "
        "в нижньому лівому куті панелі та над кожним твоїм повідомленням. "
        "Залиш порожнім, щоб повернути дефолтне «YOU».")));

    m_systemPromptEdit = new QPlainTextEdit(tab);
    m_systemPromptEdit->setPlaceholderText(
        QStringLiteral("Залиш порожнім, щоб використати вбудований промпт JARVIS."));
    m_systemPromptEdit->setMinimumHeight(220);
    m_systemPromptEdit->setPlainText(Config::SYSTEM_PROMPT);
    form->addRow(QStringLiteral("Системний промпт"),
        withHelp(m_systemPromptEdit, QStringLiteral(
        "Приховані інструкції, які модель читає <i>перед</i> кожною "
        "розмовою. Визначають персону, тон, доступні інструменти "
        "(теги <code>[CMD: …]</code>) і правила. Порожньо — буде "
        "використано вбудований промпт. <b>Редагуй обережно</b> — "
        "поганий промпт = погана модель.")));

    form->addRow(new QWidget);
}

void SettingsDialog::buildInterfaceTab(QWidget* tab) {
    auto* form = new QFormLayout(tab);
    form->setContentsMargins(8, 12, 18, 12);
    form->setHorizontalSpacing(14);
    form->setVerticalSpacing(12);

    m_accentCombo = new QComboBox(tab);
    m_accentCombo->addItem(QStringLiteral("Aurora Blue (за замовчуванням)"), QColor( 47, 129, 247));
    m_accentCombo->addItem(QStringLiteral("Cyan Pulse"),                    QColor( 56, 189, 248));
    m_accentCombo->addItem(QStringLiteral("Iris Violet"),                   QColor(139, 110, 246));
    m_accentCombo->addItem(QStringLiteral("Emerald Core"),                  QColor( 52, 211, 153));
    m_accentCombo->addItem(QStringLiteral("Amber Warning"),                 QColor(245, 158,  11));
    m_accentCombo->addItem(QStringLiteral("Crimson"),                       QColor(244,  63,  94));
    form->addRow(QStringLiteral("Акцентний колір"), withHelp(m_accentCombo, QStringLiteral(
        "Перефарбовує анімовані плями фону, рамку поля вводу, градієнт "
        "кнопки відправки й підсвітки бічної панелі. Змінює лише сімейство "
        "відтінку — типографіка, бабли й obsidian-палітра лишаються тими ж.")));

    m_opacitySlider = new QSlider(Qt::Horizontal, tab);
    m_opacitySlider->setRange(70, 100);
    m_opacitySlider->setValue(100);
    m_opacitySlider->setTickPosition(QSlider::TicksBelow);
    m_opacitySlider->setTickInterval(5);
    form->addRow(QStringLiteral("Прозорість вікна"), withHelp(m_opacitySlider, QStringLiteral(
        "Прозорість усього вікна JARVIS. <b>100 %</b> — повністю непрозоре. "
        "Менші значення дозволяють шпалерам / IDE проглядатись крізь — "
        "виглядає круто на тлі темного робочого столу. Нижче 70 % "
        "читабельність уже страждає.")));

    m_timestampsCheck = new QCheckBox(QStringLiteral("Показувати час біля повідомлень"), tab);
    m_timestampsCheck->setChecked(true);
    form->addRow(QString(), withHelp(m_timestampsCheck, QStringLiteral(
        "Перемикає маленький час <code>HH:mm</code> над кожним "
        "повідомленням. Вимкнено — чистіший, кінематографічний вигляд; "
        "увімкнено — корисно для довгих сесій.")));

    form->addRow(new QWidget);
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
    if (m_systemPromptEdit)  m_systemPromptEdit->setPlainText(Config::SYSTEM_PROMPT);
    if (m_userNameEdit)      m_userNameEdit->clear();
    if (m_accentCombo)       m_accentCombo->setCurrentIndex(0);
    if (m_opacitySlider)     m_opacitySlider->setValue(100);
    if (m_timestampsCheck)   m_timestampsCheck->setChecked(true);
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
    s.setValue(kK_SystemPrompt, m_systemPromptEdit->toPlainText());
    s.setValue(kK_AccentRgb,    getAccentColor().rgb());
    s.setValue(kK_Opacity,      m_opacitySlider->value());
    s.setValue(kK_Timestamps,   m_timestampsCheck->isChecked());
    s.setValue(kK_UserName,     getUserName());
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
