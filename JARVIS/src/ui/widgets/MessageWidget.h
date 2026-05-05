#ifndef MESSAGE_WIDGET_H
#define MESSAGE_WIDGET_H

#include <QString>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QFrame;
class QLabel;
class QTimer;
QT_END_NAMESPACE

class MessageWidget : public QWidget {
    Q_OBJECT

public:
    enum class Kind {
        User,
        Ai,
        System,   // small grey "system status" bubble (command result etc.)
    };

    // Backwards-compatible legacy ctor: bool isUser -> Kind::User / Kind::Ai.
    explicit MessageWidget(const QString& text, bool isUser, QWidget* parent = nullptr);
    explicit MessageWidget(const QString& text, Kind kind, QWidget* parent = nullptr);

    // Append raw markdown-flavoured text (used while the model streams).
    // The bubble re-renders as plain text + blinking caret to keep the
    // streaming feel cheap; finalize() does the actual markdown -> HTML
    // pass once the full reply is in.
    void appendText(const QString& text);

    // Strip streaming caret, run light markdown->HTML and switch the
    // QLabel into RichText mode. Safe to call multiple times.
    void finalize();

    // Globally toggle the small "HH:mm" timestamp under each bubble.
    static void setShowTimestamps(bool show);
    static bool showTimestamps();

    // Called when the user changes their display name via WelcomeDialog or
    // SettingsDialog. Active bubbles will pick up the new name lazily — this
    // method just stores the global so newly-built widgets read it.
    static void setUserDisplayName(const QString& name);
    static QString userDisplayName();

private:
    void buildUi(const QString& text);

    Kind      m_kind;
    bool      m_isUser;
    bool      m_finalized   = false;
    QString   m_rawText;
    QFrame*   m_bubble       = nullptr;
    QLabel*   m_authorLabel  = nullptr;
    QLabel*   m_textLabel    = nullptr;
    QLabel*   m_timeLabel    = nullptr;

    // Streaming throttle: appendText() doesn't repaint immediately — it
    // schedules m_flushTimer to coalesce token bursts into ~40 ms updates,
    // which slashes QLabel re-flow cost on long responses.
    QTimer*   m_flushTimer   = nullptr;
    bool      m_dirty        = false;
};

#endif // MESSAGE_WIDGET_H
