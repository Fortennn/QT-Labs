#pragma once

#include <QMainWindow>
#include "tickettablemodel.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onNewTicket();
    void onViewTicket();
    void onEditTicket();
    void onDeleteTicket();
    void onRefresh();
    void updateActions();
    void onTableDoubleClicked(const QModelIndex &index);

private:
    void setupTable();
    void populateSampleData();
    int  selectedRow() const;

    Ui::MainWindow    *ui;
    TicketTableModel  *m_model;
};
