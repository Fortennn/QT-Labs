#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextBrowser>
#include <QLineEdit>
#include <QBoxLayout>
#include "../ai/LlamaWorkerThread.h"

namespace Ui { class MainWindow; }

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onUserInput();
    void updateAiStream(const QString& token);
    void onReplyFinished(const QString& fullResponse);
    void addMessage(const QString& text, bool isUser);

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    void applyPremiumStyles();
    void setupDynamicUi(); // Нова функція

    Ui::MainWindow *ui;
    LlamaWorkerThread* aiThread;
    class MessageWidget* currentAiBubble;

    // Ручне керування віджетами чату
    class QScrollArea* scrollArea;
    class QVBoxLayout* chatLayout;
    class QLineEdit* inputField;
    class QPushButton* sendButton;
};

#endif // MAINWINDOW_H
