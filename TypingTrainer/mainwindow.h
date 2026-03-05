#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QKeyEvent>
#include <QLabel>
#include <QTimer>
#include <QElapsedTimer>
#include <QSettings>
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
    void onTimerTick();
    void onToggleSpeedMode();

private:
    Ui::MainWindow       *ui;
    KeyboardWidget       *m_keyboard;
    TextDisplayWidget    *m_textDisplay;
    QLabel               *m_lblPrevLine;
    QLabel               *m_lblCurrLine;
    TextModel             m_model;
    QVector<LessonEntry>  m_lessons;

    // Timer & time tracking
    QTimer        *m_sessionTimer;
    QElapsedTimer  m_elapsed;
    qint64         m_elapsedMs = 0;

    // Metrics counters
    int  m_totalKeystrokes   = 0;
    int  m_correctKeystrokes = 0;
    bool m_lastWasError      = false;
    bool m_speedModeWPM      = false; // false = CPM, true = WPM

    void setupConnections();
    void goToPage(int index);
    void scanLessons();
    void loadLesson(const QString &filePath);
    void updateTrainingUI();
    void updateCurrentLineLabel();
    void highlightNextKey();
    void updateStatsLabels();
    void resetSessionMetrics();
    void finishSession();

    QString formatTime(qint64 ms) const;
    double  calcSpeed(qint64 ms) const;
    double  calcAccuracy() const;
    QString speedLabel(double speed) const;

    void loadSettings();
    void saveSettings();
};

#endif // MAINWINDOW_H
