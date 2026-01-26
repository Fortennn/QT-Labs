#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include "game_state.h"

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_playerblueplus_clicked();

    void on_playerblueminus_clicked();

    void on_playerredplus_clicked();

    void on_playerredminus_clicked();

    void updateUI();


    void on_Reset_clicked();

private:
    Ui::MainWindow *ui;
    Game_state state;
};
#endif // MAINWINDOW_H
