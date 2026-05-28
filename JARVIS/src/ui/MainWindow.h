#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QDateTime>
#include <QMainWindow>
#include <QString>
#include <QVector>

#include "../ai/LlamaWorkerThread.h"
#include "../ai/ApiChatWorker.h"

QT_BEGIN_NAMESPACE
class QCloseEvent;
class QFrame;
class QGraphicsDropShadowEffect;
class QKeyEvent;
class QLabel;
class QSystemTrayIcon;
class QPushButton;
class QScrollArea;
class QTextEdit;
class QVBoxLayout;
class QWidget;
QT_END_NAMESPACE

namespace Ui { class MainWindow; }

class JarvisHttpServer;
class MessageWidget;
class ParticleBackground;
class SettingsDialog;
class QProcess;

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
    void onGestureDetected(const QString& gestureName);

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

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
    void exportCurrentChatAsMarkdown();

    // Inject a small grey "system" status bubble into the chat (e.g.
    // "✓ Discord opened", or the captured stdout of an `ipconfig` call).
    void appendSystemBubble(const QString& text);

    // ---- System command dispatch ----
    void handleSystemCommand(const QString& shellCmd, bool isPowerShell);

    // Smart launchers / closers (Windows-focused, no-op-ish elsewhere).
    bool tryOpenUrlOrFile(const QString& target);
    bool tryLaunchKnownApp(const QString& alias, const QStringList& extraArgs);
    bool tryCloseProcess(const QString& alias);
    void runHiddenPowerShell(const QString& cmdLine) const;
    // Run a shell command WITH stdout/stderr capture and dump the (truncated)
    // output as a system bubble when it finishes. label is prepended to the
    // bubble text. Returns immediately; QProcess deletes itself on finish.
    void runCapturedShell(const QString& program,
                          const QStringList& args,
                          const QString& label);

    // ---- System tray ----
    void setupTrayIcon();

    // ---- LAN web server (phone control panel) ----
    // Apply the persisted server prefs (enabled / port / pin) at startup
    // and after the user clicks Apply in SettingsDialog. Idempotent: tears
    // the server down before re-listening if anything changed.
    void applyServerPreferences();
    // Slots for JarvisHttpServer signals. The web server runs on the GUI
    // thread so these are direct calls, but we keep them as slots so they
    // can also be queued safely.
    void onWebChatRequested(const QString& message);
    void onWebCommandRequested(const QString& cmd, bool isPowerShell);

    // Apply the currently-persisted backend choice (local llama.cpp vs
    // remote OpenAI-compatible API) by reading QSettings and configuring
    // m_apiBackend accordingly. Called on init + after Settings dialog.
    void applyBackendPreferences();

    // Тип активного бекенда. Визначає, куди йдуть промпти й сигнали.
    enum class Backend { LocalLlama, RemoteApi };
    Backend currentBackend() const { return m_backend; }
    bool    backendIsBusy() const;
    QString backendModelName() const;
    void    backendQueuePrompt(const QString& system, const QString& user);
    void    backendStop();
    void    backendClearHistory();

    // ---- Camera / Gesture Mode (Python-based) ----
    void toggleCameraMode(bool enabled);
    // Try to match `text` against the camera-keyword regex; if matched,
    // toggle the mode accordingly and return true so the caller can stop
    // forwarding the text to the LLM.  Used both as a pre-LLM intercept
    // in onUserInput() and inside handleSystemCommand().
    bool tryHandleCameraKeyword(const QString& text);
    // Refresh the side-panel camera button's label / colour so it reads
    // "Камера: ВКЛ" while the engine is alive and dimmed otherwise.
    void updateCameraButtonVisual();

    bool m_cameraModeActive = false;
    QProcess* m_gestureProcess = nullptr;
    QPushButton* m_cameraButton = nullptr;

    // ---- Wake Simulation ----
    void onWakeActivation();

    // ---- Members ----
    Ui::MainWindow*     ui              = nullptr;
    LlamaWorkerThread*  aiThread        = nullptr;
    ApiChatWorker*      m_apiBackend    = nullptr;
    Backend             m_backend       = Backend::LocalLlama;
    MessageWidget*      currentAiBubble = nullptr;

    // Manually managed chat widgets
    QScrollArea*  scrollArea   = nullptr;
    QVBoxLayout*  chatLayout   = nullptr;
    QTextEdit*    inputField   = nullptr;
    QPushButton*  sendButton   = nullptr;
    QFrame*       inputWrapper = nullptr;
    QSystemTrayIcon* trayIcon  = nullptr;
    bool          m_quitting   = false;

    // LAN HTTP server for the phone control panel. Owned, lazily created
    // by applyServerPreferences() so the user can toggle it from Settings.
    JarvisHttpServer* httpServer    = nullptr;
    // True while we're waiting for the AI to finish a web-initiated chat
    // request. onReplyFinished() forwards the result to httpServer when set.
    bool              m_webChatPending = false;

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
