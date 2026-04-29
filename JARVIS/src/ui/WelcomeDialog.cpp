#include "WelcomeDialog.h"

#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

WelcomeDialog::WelcomeDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Привіт від JARVIS"));
    setModal(true);
    setFixedSize(520, 380);
    setWindowFlag(Qt::FramelessWindowHint, true);
    setAttribute(Qt::WA_TranslucentBackground, true);

    // Outer "card" so we can render rounded corners on a frameless window.
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(28, 28, 28, 28);

    auto* card = new QFrame(this);
    card->setObjectName(QStringLiteral("welcomeCard"));
    card->setStyleSheet(QStringLiteral(R"_(
        QFrame#welcomeCard {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 #0e1726, stop:1 #060a12);
            border: 1px solid rgba(80, 130, 200, 90);
            border-radius: 22px;
        }
    )_"));

    auto* shadow = new QGraphicsDropShadowEffect(card);
    shadow->setBlurRadius(60);
    shadow->setOffset(0, 16);
    shadow->setColor(QColor(0, 0, 0, 200));
    card->setGraphicsEffect(shadow);

    auto* col = new QVBoxLayout(card);
    col->setContentsMargins(36, 32, 36, 28);
    col->setSpacing(14);

    auto* eyebrow = new QLabel(QStringLiteral("JARVIS · ВІТАЮ"), card);
    eyebrow->setStyleSheet(QStringLiteral(
        "color: #58a6ff; font-size: 10.5px; font-weight: 800; letter-spacing: 4px;"));

    auto* title = new QLabel(QStringLiteral("Як до тебе звертатись?"), card);
    title->setStyleSheet(QStringLiteral(
        "color: #ffffff; font-size: 24px; font-weight: 800; letter-spacing: 0.5px;"));
    title->setWordWrap(true);

    auto* subtitle = new QLabel(
        QStringLiteral("Це ім'я з'являтиметься поряд з твоїми повідомленнями "
                       "та в нижньому кутку панелі. Завжди можна змінити в "
                       "Налаштуваннях."), card);
    subtitle->setStyleSheet(QStringLiteral(
        "color: #8a99b1; font-size: 12.5px; line-height: 1.45;"));
    subtitle->setWordWrap(true);

    m_nameEdit = new QLineEdit(card);
    m_nameEdit->setPlaceholderText(QStringLiteral("Наприклад: Олександр"));
    m_nameEdit->setMaxLength(40);
    m_nameEdit->setStyleSheet(QStringLiteral(R"_(
        QLineEdit {
            background: rgba(13, 19, 28, 220);
            color: #e6edf3;
            border: 1px solid #233248;
            border-radius: 12px;
            padding: 12px 16px;
            font-size: 15px;
            font-weight: 600;
            selection-background-color: #2f81f7;
        }
        QLineEdit:focus { border: 1px solid #2f81f7; }
    )_"));

    // Continue + Skip buttons.
    auto* btnRow = new QHBoxLayout;
    btnRow->setSpacing(10);
    btnRow->addStretch();

    auto* skipBtn = new QPushButton(QStringLiteral("Пропустити"), card);
    skipBtn->setCursor(Qt::PointingHandCursor);
    skipBtn->setStyleSheet(QStringLiteral(R"_(
        QPushButton {
            background: transparent;
            color: #8a99b1;
            border: 1px solid #233248;
            border-radius: 10px;
            padding: 10px 18px;
            font-size: 13px;
            font-weight: 600;
        }
        QPushButton:hover { color: #e6edf3; border-color: #58a6ff; }
    )_"));

    auto* okBtn = new QPushButton(QStringLiteral("Продовжити  →"), card);
    okBtn->setCursor(Qt::PointingHandCursor);
    okBtn->setDefault(true);
    okBtn->setStyleSheet(QStringLiteral(R"_(
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #2f81f7, stop:1 #1d6def);
            color: #ffffff;
            border: 1px solid rgba(255,255,255,40);
            border-radius: 10px;
            padding: 10px 22px;
            font-size: 13.5px;
            font-weight: 800;
            letter-spacing: 0.3px;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #4593f9, stop:1 #2f81f7);
        }
    )_"));

    btnRow->addWidget(skipBtn);
    btnRow->addWidget(okBtn);

    col->addWidget(eyebrow);
    col->addWidget(title);
    col->addWidget(subtitle);
    col->addSpacing(6);
    col->addWidget(m_nameEdit);
    col->addStretch();
    col->addLayout(btnRow);

    root->addWidget(card);

    connect(okBtn,   &QPushButton::clicked, this, &QDialog::accept);
    connect(skipBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_nameEdit, &QLineEdit::returnPressed, this, &QDialog::accept);

    m_nameEdit->setFocus();
}

QString WelcomeDialog::chosenName() const {
    return m_nameEdit ? m_nameEdit->text().trimmed() : QString();
}

QString WelcomeDialog::savedName() {
    QSettings s;
    return s.value(QStringLiteral("user/name")).toString().trimmed();
}

void WelcomeDialog::persist(const QString& name) {
    QSettings s;
    s.setValue(QStringLiteral("user/name"), name.trimmed());
}
