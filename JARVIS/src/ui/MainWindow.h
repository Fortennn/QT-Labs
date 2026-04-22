#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextBrowser>
#include <QLineEdit>
#include <QBoxLayout>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onUserInput();
    void updateAiStream(const QString& token);

private:
    void setupUi();
    void loadStyles();

    QTextBrowser* chatBrowser;
    QLineEdit* inputField;
};

#endif // MAINWINDOW_H
