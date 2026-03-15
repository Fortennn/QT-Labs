#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ticketdialog.h"
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_model(nullptr)
    , m_ticketDialog(nullptr)
{
    ui->setupUi(this);
    setupModel();
    updateActionsState();

    connect(ui->actionNew, &QAction::triggered, this, &MainWindow::onNew);
    connect(ui->actionView, &QAction::triggered, this, &MainWindow::onView);
    connect(ui->actionEdit, &QAction::triggered, this, &MainWindow::onEdit);
    connect(ui->actionDelete, &QAction::triggered, this, &MainWindow::onDelete);
    connect(ui->actionRefresh, &QAction::triggered, this, &MainWindow::onRefresh);
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::onAbout);
    connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);

    connect(ui->ticketsTableView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &MainWindow::onSelectionChanged);

    ui->statusBar->showMessage(tr("Ready"));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupModel()
{
    m_model = new QStandardItemModel(0, 5, this);
    m_model->setHorizontalHeaderLabels({tr("ID"), tr("Title"), tr("Priority"), tr("Status"), tr("Created At")});
    ui->ticketsTableView->setModel(m_model);
    ui->ticketsTableView->horizontalHeader()->setStretchLastSection(true);
}

void MainWindow::updateActionsState()
{
    bool hasSelection = ui->ticketsTableView->selectionModel()->hasSelection();
    ui->actionView->setEnabled(hasSelection);
    ui->actionEdit->setEnabled(hasSelection);
    ui->actionDelete->setEnabled(hasSelection);
}

void MainWindow::onSelectionChanged()
{
    updateActionsState();
}

void MainWindow::onNew()
{
    if (!m_ticketDialog) {
        m_ticketDialog = new TicketDialog(this);
    }
    m_ticketDialog->setMode(TicketDialog::ModeNew);
    m_ticketDialog->setWindowTitle(tr("New Ticket"));
    m_ticketDialog->show();
    m_ticketDialog->raise();
    m_ticketDialog->activateWindow();
}

void MainWindow::onView()
{
    if (!ui->ticketsTableView->selectionModel()->hasSelection())
        return;
    if (!m_ticketDialog) {
        m_ticketDialog = new TicketDialog(this);
    }
    m_ticketDialog->setMode(TicketDialog::ModeView);
    m_ticketDialog->setWindowTitle(tr("View Ticket"));
    m_ticketDialog->show();
    m_ticketDialog->raise();
    m_ticketDialog->activateWindow();
}

void MainWindow::onEdit()
{
    if (!ui->ticketsTableView->selectionModel()->hasSelection())
        return;
    if (!m_ticketDialog) {
        m_ticketDialog = new TicketDialog(this);
    }
    m_ticketDialog->setMode(TicketDialog::ModeEdit);
    m_ticketDialog->setWindowTitle(tr("Edit Ticket"));
    m_ticketDialog->show();
    m_ticketDialog->raise();
    m_ticketDialog->activateWindow();
}

void MainWindow::onDelete()
{
    if (!ui->ticketsTableView->selectionModel()->hasSelection())
        return;
    auto reply = QMessageBox::question(this, tr("Delete Ticket"),
                                       tr("Delete the selected ticket?"),
                                       QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        int row = ui->ticketsTableView->currentIndex().row();
        m_model->removeRow(row);
        updateActionsState();
    }
}

void MainWindow::onRefresh()
{
    ui->statusBar->showMessage(tr("Refreshed"));
}

void MainWindow::onAbout()
{
    QMessageBox::about(this, tr("About Helpdesk"), tr("Helpdesk v 1.0"));
}
