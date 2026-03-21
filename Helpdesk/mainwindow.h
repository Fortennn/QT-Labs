#pragma once

#include <QMainWindow>
#include "tickettablemodel.h"

namespace Ui { class MainWindow; }

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onNewTriggered();
    void onViewTriggered();
    void onEditTriggered();
    void onDeleteTriggered();
    void onRefreshTriggered();
    void onExportCsvTriggered();
    void onOpenCsvTriggered();
    void onExitTriggered();
    void onShowToolbarToggled(bool checked);
    void onShowFilterToggled(bool checked);
    void onResetColumnWidths();
    void onAboutTriggered();
    void onSearchChanged();
    void onFilterChanged();
    void onClearClicked();
    void updateActionsState();

private:
    int currentRow() const;
    void applyFilter();
    void openTicketDialog(int row, bool editMode);

    Ui::MainWindow *ui;
    TicketTableModel *m_model;
};
