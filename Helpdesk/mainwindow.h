#pragma once

#include <QMainWindow>
#include <QStandardItemModel>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class TicketDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onNew();
    void onView();
    void onEdit();
    void onDelete();
    void onRefresh();
    void onAbout();
    void onSelectionChanged();

private:
    Ui::MainWindow *ui;
    QStandardItemModel *m_model;
    TicketDialog *m_ticketDialog;

    void setupModel();
    void updateActionsState();
};
