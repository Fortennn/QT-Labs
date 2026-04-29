#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>

#include "../ai/LlamaWorkerThread.h"

QT_BEGIN_NAMESPACE
class QScrollArea;
class QVBoxLayout;
class QLineEdit;
class QPushButton;
class QFrame;
class QKeyEvent;
QT_END_NAMESPACE

namespace Ui { class MainWindow; }

class MessageWidget;

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
    void openSettings();

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    void applyPremiumStyles();
    void setupDynamicUi();
    void scrollToBottom();
    bool isNearBottom() const;

    // Detached system shell execution (CMD / PowerShell). Silent, no window.
    void handleSystemCommand(const QString& shellCmd, bool isPowerShell);

    Ui::MainWindow* ui;
    LlamaWorkerThread* aiThread = nullptr;
    MessageWidget* currentAiBubble = nullptr;

    // Manually managed chat widgets
    QScrollArea* scrollArea  = nullptr;
    QVBoxLayout* chatLayout  = nullptr;
    QLineEdit*   inputField  = nullptr;
    QPushButton* sendButton  = nullptr;
    QFrame*      inputWrapper = nullptr;

    // SettingsDialog state — kept in sync with the AI thread.
    float   m_temperature = 0.8f;
    int     m_contextSize = 2048;
    QString m_modelPath;
};

#endif // MAINWINDOW_H
