#include "MessageWidget.h"

#include <QDateTime>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

namespace {

bool    g_showTimestamps    = true;
QString g_userDisplayName   = QStringLiteral("YOU");

QString currentTimeString() {
    return QDateTime::currentDateTime().toString(QStringLiteral("HH:mm"));
}

} // namespace

void MessageWidget::setShowTimestamps(bool show) { g_showTimestamps = show; }
bool MessageWidget::showTimestamps()             { return g_showTimestamps; }

void MessageWidget::setUserDisplayName(const QString& name) {
    g_userDisplayName = name.isEmpty() ? QStringLiteral("YOU") : name.toUpper();
}
QString MessageWidget::userDisplayName() { return g_userDisplayName; }

MessageWidget::MessageWidget(const QString& text, bool isUser, QWidget* parent)
    : QWidget(parent),
      m_isUser(isUser)
{
    // No translucent / opacity effects on the parent — those interfered with
    // the child bubble's QSS background and graphics shadow effect. The
    // bubble itself is a QFrame that paints its own gradient + border.
    setAttribute(Qt::WA_StyledBackground, false);
    buildUi(text);
}

void MessageWidget::buildUi(const QString& text) {
    auto* outer = new QVBoxLayout(this);
    outer->setSpacing(4);
    // Asymmetric margins create the lane effect (user on right, AI on left).
    outer->setContentsMargins(m_isUser ? 90 : 8, 6, m_isUser ? 8 : 90, 6);

    // ---- Header: author + timestamp ----
    auto* header = new QHBoxLayout;
    header->setContentsMargins(8, 0, 8, 0);
    header->setSpacing(8);

    m_authorLabel = new QLabel(
        m_isUser ? g_userDisplayName : QStringLiteral("JARVIS"), this);
    m_authorLabel->setStyleSheet(m_isUser
        ? QStringLiteral("color: #9bb6e0; font-size: 10.5px; font-weight: 700; "
                         "letter-spacing: 1.5px; text-transform: uppercase; "
                         "background: transparent;")
        : QStringLiteral("color: #58a6ff; font-size: 10.5px; font-weight: 700; "
                         "letter-spacing: 1.5px; text-transform: uppercase; "
                         "background: transparent;"));

    m_timeLabel = new QLabel(currentTimeString(), this);
    m_timeLabel->setStyleSheet(QStringLiteral(
        "color: #5e6c80; font-size: 9.5px; font-weight: 500; "
        "letter-spacing: 1px; background: transparent;"));
    m_timeLabel->setVisible(g_showTimestamps);

    if (m_isUser) {
        header->addStretch();
        header->addWidget(m_timeLabel);
        header->addWidget(m_authorLabel);
    } else {
        header->addWidget(m_authorLabel);
        header->addWidget(m_timeLabel);
        header->addStretch();
    }
    outer->addLayout(header);

    // ---- Bubble: a styled QFrame holding a transparent QLabel ----
    m_bubble = new QFrame(this);
    m_bubble->setObjectName(m_isUser ? QStringLiteral("userBubble")
                                     : QStringLiteral("aiBubble"));
    m_bubble->setAttribute(Qt::WA_StyledBackground, true);
    m_bubble->setStyleSheet(m_isUser
        ? QStringLiteral(R"_(
            QFrame#userBubble {
                background: qlineargradient(
                    x1:0, y1:0, x2:1, y2:1,
                    stop:0 #2f81f7, stop:1 #1d6def);
                border: 1px solid rgba(255,255,255,40);
                border-top-left-radius: 16px;
                border-top-right-radius: 16px;
                border-bottom-left-radius: 16px;
                border-bottom-right-radius: 4px;
            })_")
        : QStringLiteral(R"_(
            QFrame#aiBubble {
                background: qlineargradient(
                    x1:0, y1:0, x2:0, y2:1,
                    stop:0 #16202a,
                    stop:1 #0d131c);
                border: 1px solid rgba(60, 78, 102, 200);
                border-top-left-radius: 16px;
                border-top-right-radius: 16px;
                border-bottom-left-radius: 4px;
                border-bottom-right-radius: 16px;
            })_"));

    auto* bubbleLay = new QVBoxLayout(m_bubble);
    bubbleLay->setContentsMargins(14, 11, 14, 11);
    bubbleLay->setSpacing(0);

    m_textLabel = new QLabel(text, m_bubble);
    m_textLabel->setWordWrap(true);
    m_textLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_textLabel->setStyleSheet(m_isUser
        ? QStringLiteral("color: #f4f7fb; font-size: 13.5px; "
                         "background: transparent;")
        : QStringLiteral("color: #e6edf3; font-size: 13.5px; "
                         "background: transparent;"));
    bubbleLay->addWidget(m_textLabel);

    // Soft drop shadow on the bubble itself.
    auto* shadow = new QGraphicsDropShadowEffect(m_bubble);
    shadow->setBlurRadius(28.0);
    shadow->setOffset(0, 6);
    shadow->setColor(m_isUser ? QColor(31, 110, 235, 110)
                              : QColor(0,    0,   0, 150));
    m_bubble->setGraphicsEffect(shadow);

    // Bubble row — pushes the bubble to the correct side without expanding.
    auto* bubbleRow = new QHBoxLayout;
    bubbleRow->setContentsMargins(0, 0, 0, 0);
    if (m_isUser) {
        bubbleRow->addStretch();
        bubbleRow->addWidget(m_bubble, /*stretch=*/0);
    } else {
        bubbleRow->addWidget(m_bubble, /*stretch=*/0);
        bubbleRow->addStretch();
    }
    outer->addLayout(bubbleRow);
}

void MessageWidget::appendText(const QString& text) {
    if (!m_textLabel) return;
    m_textLabel->setText(m_textLabel->text() + text);
    if (m_timeLabel) m_timeLabel->setText(currentTimeString());
}
