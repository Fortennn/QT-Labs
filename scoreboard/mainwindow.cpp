#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_playerblueplus_clicked()
{
    this->playerblue += 1;
    updateUI();
}


void MainWindow::on_playerblueminus_clicked()
{
    this->playerblue -= 1;
    if (playerblue < 0){
        playerblue = 0;
    }
    updateUI();
}

void MainWindow::on_playerredplus_clicked()
{
    this->playerred += 1;
    updateUI();
}


void MainWindow::on_playerredminus_clicked()
{
    this->playerred -= 1;
    if (playerred < 0){
        playerred = 0;
    }
    updateUI();
}

void MainWindow::updateUI()
{
    this->ui->BlueScore->display(playerblue);
    this->ui->RedScore->display(playerred);
}


void MainWindow::on_Reset_clicked()
{
    this->playerred = 0;
    this->playerblue = 0;
    updateUI();
}

