#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QDateTime>
#include <QMainWindow>
#include <QString>
#include <QVector>

#include "../ai/LlamaWorkerThread.h"

QT_BEGIN_NAMESPACE
class QFrame;
class QGraphicsDropShadowEffect;
class QKeyEvent;
class QLabel;
class QLineEdit;
class QPushButton;
class QScrollArea;
class QVBoxLayout;
class QWidget;
QT_END_NAMESPACE

namespace Ui { class MainWindow; }

class MessageWidget;
class ParticleBackground;
class SettingsDialog;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void onUserInput();
    void updateAiStream(const QString& token);
    void onReplyFinished(const QString& fullResponse);
    void addMessage(const QString& text, bool isUser);

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    // ---- UI construction ----
    void applyPremiumStyles();
    void setupDynamicUi();
    void installSendButtonAnimation();
    void installSideButtonAnimations();
    void installInputFocusAnimation();

    // Side panel polish: header card + telemetry card + extra buttons.
    void buildSidebar();

    // Apply visual / behavioral preferences picked in SettingsDialog.
    void applyUiPreferences(const SettingsDialog& dlg);

    // Apply settings persisted from the previous run, *before* showing the
    // window. Reads QSettings directly.
    void applyPersistedPreferences();

    // Called by aiThread signals to flip the Send button between idle (▶)
    // and generating (◼) states. While generating, clicking the button
    // calls aiThread->stopGeneration().
    void setGenerating(bool generating);

    // Update the bottom-of-sidebar user chip with the current display name.
    void refreshUserChip();

    // ---- Chat helpers ----
    bool isNearBottom() const;
    void scrollToBottom();

    // ---- Chat history ----
    void clearChatLayout();          // remove every MessageWidget from chatLayout
    void newChat(bool persistOld);   // start a brand new conversation
    void saveCurrentChat() const;    // serialize m_currentMessages to JSON
    void loadChatById(const QString& id);
    void appendUserMessage(const QString& text);
    void appendAiMessage(const QString& text);
    void openChatHistoryDialog();

    // ---- System command dispatch ----
    void handleSystemCommand(const QString& shellCmd, bool isPowerShell);

    // Smart launchers / closers (Windows-focused, no-op-ish elsewhere).
    bool tryOpenUrlOrFile(const QString& target);
    bool tryLaunchKnownApp(const QString& alias, const QStringList& extraArgs);
    bool tryCloseProcess(const QString& alias);
    void runHiddenPowerShell(const QString& cmdLine) const;

    // ---- Members ----
    Ui::MainWindow*     ui              = nullptr;
    LlamaWorkerThread*  aiThread        = nullptr;
    MessageWidget*      currentAiBubble = nullptr;

    // Manually managed chat widgets
    QScrollArea*  scrollArea   = nullptr;
    QVBoxLayout*  chatLayout   = nullptr;
    QLineEdit*    inputField   = nullptr;
    QPushButton*  sendButton   = nullptr;
    QFrame*       inputWrapper = nullptr;

    // Animated background (kept as a member so settings can re-tint it).
    ParticleBackground* particleBg = nullptr;

    // Sidebar extras (built dynamically; not in MainWindow.ui).
    QLabel*      sidebarModelLabel = nullptr;
    QLabel*      sidebarStatusBig  = nullptr;
    QLabel*      telemetryLabel    = nullptr;
    QLabel*      userChipName      = nullptr;
    QLabel*      userChipAvatar    = nullptr;

    // True while aiThread is producing tokens for the current request.
    bool         m_generating      = false;

    // Last model file path queued for loading. Used by the modelLoaded slot to
    // refresh the "ACTIVE MODEL" sidebar label with the correct file name.
    QString      m_lastModelPath;

    // ---- Chat-history state ----
    struct StoredMessage {
        bool      isUser;
        QString   text;
        QDateTime time;
    };

    QString               m_currentChatId;
    QString               m_currentChatTitle;
    QVector<StoredMessage> m_currentMessages;
    QString               m_currentAiText;   // accumulator while the AI streams
    QPushButton*          btnHistory       = nullptr;

    // Effects we animate
    QGraphicsDropShadowEffect* sendShadow  = nullptr;
    QGraphicsDropShadowEffect* inputShadow = nullptr;
};

#endif // MAINWINDOW_H
