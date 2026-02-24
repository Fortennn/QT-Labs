#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QKeyEvent>
#include <QLabel>
#include "keyboardwidget.h"
#include "textdisplaywidget.h"
#include "textmodel.h"
#include "lessonloader.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private slots:
    void onStartTraining();
    void onRestartTraining();
    void onReturnToMain();
    void onAbout();
    void onExit();
    void onLessonChanged(int index);
    void onRandomLesson();
    void onReloadLessons();

private:
    Ui::MainWindow       *ui;
    KeyboardWidget       *m_keyboard;
    TextDisplayWidget    *m_textDisplay;
    QLabel               *m_lblPrevLine;
    QLabel               *m_lblCurrLine;
    TextModel             m_model;
    QVector<LessonEntry>  m_lessons;

    bool  m_lastWasError = false;

    void setupConnections();
    void goToPage(int index);
    void scanLessons();
    void loadLesson(const QString &filePath);
    void updateTrainingUI();
    void updateCurrentLineLabel();
    void highlightNextKey();
};

#endif // MAINWINDOW_H
