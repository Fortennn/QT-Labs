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

private:
    Ui::MainWindow *ui;
    LlamaWorkerThread* aiThread;
};

#endif // MAINWINDOW_H
