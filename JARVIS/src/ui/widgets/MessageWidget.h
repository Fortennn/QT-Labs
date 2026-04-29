#ifndef MESSAGE_WIDGET_H
#define MESSAGE_WIDGET_H

#include <QString>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QFrame;
class QLabel;
QT_END_NAMESPACE

class MessageWidget : public QWidget {
    Q_OBJECT

public:
    explicit MessageWidget(const QString& text, bool isUser, QWidget* parent = nullptr);

    void appendText(const QString& text);

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

    bool      m_isUser;
    QFrame*   m_bubble       = nullptr;
    QLabel*   m_authorLabel  = nullptr;
    QLabel*   m_textLabel    = nullptr;
    QLabel*   m_timeLabel    = nullptr;
};

#endif // MESSAGE_WIDGET_H
