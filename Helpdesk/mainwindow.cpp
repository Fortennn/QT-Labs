#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ticketdialog.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QTextStream>
#include <QFile>
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_model(new TicketTableModel(this))
{
    ui->setupUi(this);

    ui->comboFilterStatus->addItems({"All", "Open", "In Progress", "Resolved", "Closed"});
    ui->comboFilterPriority->addItems({"All", "Low", "Medium", "High", "Critical"});

    ui->tableView->setModel(m_model);
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableView->horizontalHeader()->setStretchLastSection(true);
    ui->tableView->verticalHeader()->setVisible(false);

    connect(ui->tableView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &MainWindow::updateActionsState);

    connect(ui->actionNew,     &QAction::triggered, this, &MainWindow::onNewTriggered);
    connect(ui->actionView,    &QAction::triggered, this, &MainWindow::onViewTriggered);
    connect(ui->actionEdit,    &QAction::triggered, this, &MainWindow::onEditTriggered);
    connect(ui->actionDelete,  &QAction::triggered, this, &MainWindow::onDeleteTriggered);
    connect(ui->actionRefresh, &QAction::triggered, this, &MainWindow::onRefreshTriggered);
    connect(ui->actionExportCsv,   &QAction::triggered, this, &MainWindow::onExportCsvTriggered);
    connect(ui->actionOpenCsv,     &QAction::triggered, this, &MainWindow::onOpenCsvTriggered);
    connect(ui->actionExit,        &QAction::triggered, this, &MainWindow::onExitTriggered);
    connect(ui->actionShowToolbar, &QAction::toggled,   this, &MainWindow::onShowToolbarToggled);
    connect(ui->actionShowFilter,  &QAction::toggled,   this, &MainWindow::onShowFilterToggled);
    connect(ui->actionCollapseAll, &QAction::triggered, this, &MainWindow::onResetColumnWidths);
    connect(ui->actionAbout,       &QAction::triggered, this, &MainWindow::onAboutTriggered);

    connect(ui->btnNew,     &QPushButton::clicked, this, &MainWindow::onNewTriggered);
    connect(ui->btnView,    &QPushButton::clicked, this, &MainWindow::onViewTriggered);
    connect(ui->btnEdit,    &QPushButton::clicked, this, &MainWindow::onEditTriggered);
    connect(ui->btnDelete,  &QPushButton::clicked, this, &MainWindow::onDeleteTriggered);
    connect(ui->btnRefresh, &QPushButton::clicked, this, &MainWindow::onRefreshTriggered);

    connect(ui->editSearch,           &QLineEdit::textChanged,      this, &MainWindow::onSearchChanged);
    connect(ui->comboFilterStatus,    &QComboBox::currentTextChanged, this, &MainWindow::onFilterChanged);
    connect(ui->comboFilterPriority,  &QComboBox::currentTextChanged, this, &MainWindow::onFilterChanged);
    connect(ui->btnClear,             &QPushButton::clicked,        this, &MainWindow::onClearClicked);

    connect(ui->tableView, &QTableView::doubleClicked, this, [this]() { onViewTriggered(); });

    updateActionsState();
    ui->statusbar->showMessage("Ready");
}

MainWindow::~MainWindow()
{
    delete ui;
}

int MainWindow::currentRow() const
{
    const auto rows = ui->tableView->selectionModel()->selectedRows();
    if (rows.isEmpty())
        return -1;
    return rows.first().row();
}

void MainWindow::updateActionsState()
{
    const bool hasSelection = ui->tableView->selectionModel()->hasSelection();

    ui->actionNew->setEnabled(true);
    ui->actionView->setEnabled(hasSelection);
    ui->actionEdit->setEnabled(hasSelection);
    ui->actionDelete->setEnabled(hasSelection);

    ui->btnNew->setEnabled(true);
    ui->btnView->setEnabled(hasSelection);
    ui->btnEdit->setEnabled(hasSelection);
    ui->btnDelete->setEnabled(hasSelection);

    const int total    = m_model->rowCount();
    ui->statusbar->showMessage(QString("Ready   Total: %1").arg(total));
}

void MainWindow::onNewTriggered()
{
    auto *dialog = new TicketDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setMode(TicketDialog::Mode::Create);

    connect(dialog, &TicketDialog::createRequested, this, [this](const Ticket &t) {
        m_model->appendTicket(t);
        updateActionsState();
    });

    dialog->show();
}

void MainWindow::openTicketDialog(int row, bool editMode)
{
    if (row < 0)
        return;

    const Ticket ticket = m_model->ticketAt(row);

    auto *dialog = new TicketDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->loadTicket(ticket);
    dialog->setMode(editMode ? TicketDialog::Mode::Edit : TicketDialog::Mode::View);

    connect(dialog, &TicketDialog::updateRequested, this, [this, row](const Ticket &t) {
        m_model->replaceTicket(row, t);
        updateActionsState();
    });

    dialog->show();
}

void MainWindow::onViewTriggered()
{
    openTicketDialog(currentRow(), false);
}

void MainWindow::onEditTriggered()
{
    openTicketDialog(currentRow(), true);
}

void MainWindow::onDeleteTriggered()
{
    const int row = currentRow();
    if (row < 0)
        return;

    const Ticket t = m_model->ticketAt(row);
    const auto result = QMessageBox::question(
        this,
        "Delete Ticket",
        QString("Are you sure you want to delete ticket #%1 \"%2\"?").arg(t.id).arg(t.title),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );

    if (result == QMessageBox::Yes) {
        m_model->removeTicket(row);
        updateActionsState();
    }
}

void MainWindow::onRefreshTriggered()
{
    applyFilter();
    updateActionsState();
}

void MainWindow::onSearchChanged()
{
    applyFilter();
}

void MainWindow::onFilterChanged()
{
    applyFilter();
}

void MainWindow::onClearClicked()
{
    ui->editSearch->clear();
    ui->comboFilterStatus->setCurrentIndex(0);
    ui->comboFilterPriority->setCurrentIndex(0);
    applyFilter();
}

void MainWindow::onExportCsvTriggered()
{
    const QString path = QFileDialog::getSaveFileName(this, "Export to CSV", "tickets.csv", "CSV Files (*.csv)");
    if (path.isEmpty())
        return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream out(&file);
    out << "ID,Title,Priority,Status,Created At\n";
    for (int row = 0; row < m_model->rowCount(); ++row) {
        if (ui->tableView->isRowHidden(row))
            continue;
        const Ticket t = m_model->ticketAt(row);
        QString safeTitle = t.title;
        safeTitle.replace("\"", "\"\"");
        out << t.id << ","
            << "\"" << safeTitle << "\","
            << t.priority << ","
            << t.status << ","
            << t.createdAt.toString("yyyy-MM-dd HH:mm") << "\n";
    }
    ui->statusbar->showMessage(QString("Exported to %1").arg(path), 4000);
}

void MainWindow::onExitTriggered()
{
    close();
}

void MainWindow::onShowToolbarToggled(bool checked)
{
    ui->btnNew->setVisible(checked);
    ui->btnView->setVisible(checked);
    ui->btnEdit->setVisible(checked);
    ui->btnDelete->setVisible(checked);
    ui->btnRefresh->setVisible(checked);
}

void MainWindow::onShowFilterToggled(bool checked)
{
    ui->labelStatus->setVisible(checked);
    ui->comboFilterStatus->setVisible(checked);
    ui->labelPriority->setVisible(checked);
    ui->comboFilterPriority->setVisible(checked);
    ui->editSearch->setVisible(checked);
    ui->btnClear->setVisible(checked);
}

void MainWindow::onResetColumnWidths()
{
    ui->tableView->resizeColumnsToContents();
    ui->tableView->horizontalHeader()->setStretchLastSection(true);
}

void MainWindow::onOpenCsvTriggered()
{
    const QString path = QFileDialog::getOpenFileName(this, "Open CSV", "", "CSV Files (*.csv);;All Files (*)");
    if (path.isEmpty())
        return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", QString("Cannot open file:\n%1").arg(path));
        return;
    }

    QTextStream in(&file);
    const QString header = in.readLine();
    if (header.isNull()) {
        QMessageBox::warning(this, "Error", "File is empty.");
        return;
    }

    int imported = 0;
    int skipped  = 0;

    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (line.isEmpty())
            continue;

        // Parse CSV line: id,"title",priority,status,created_at
        QStringList fields;
        QString current;
        bool inQuotes = false;
        for (int i = 0; i < line.size(); ++i) {
            const QChar c = line[i];
            if (c == '"') {
                if (inQuotes && i + 1 < line.size() && line[i + 1] == '"') {
                    current += '"';
                    ++i;
                } else {
                    inQuotes = !inQuotes;
                }
            } else if (c == ',' && !inQuotes) {
                fields.append(current);
                current.clear();
            } else {
                current += c;
            }
        }
        fields.append(current);

        if (fields.size() < 4) {
            ++skipped;
            continue;
        }

        Ticket t;
        t.title       = fields.value(1).trimmed();
        t.priority    = fields.value(2).trimmed();
        t.status      = fields.value(3).trimmed();
        t.createdAt   = QDateTime::fromString(fields.value(4).trimmed(), "yyyy-MM-dd HH:mm");

        if (t.title.isEmpty()) {
            ++skipped;
            continue;
        }

        m_model->appendTicket(t);
        ++imported;
    }

    applyFilter();
    updateActionsState();
    ui->statusbar->showMessage(
        QString("Imported %1 ticket(s), skipped %2 line(s).").arg(imported).arg(skipped), 5000);
}

void MainWindow::onAboutTriggered()
{
    QMessageBox::about(this, "About Helpdesk",
        "<b>Helpdesk v3.0</b><br><br>"
        "Ticket management application built with Qt Widgets.");
}

void MainWindow::applyFilter()
{
    const QString search   = ui->editSearch->text().trimmed().toLower();
    const QString status   = ui->comboFilterStatus->currentText();
    const QString priority = ui->comboFilterPriority->currentText();

    const QList<Ticket> all = m_model->allTickets();

    int filtered = 0;
    for (int row = 0; row < m_model->rowCount(); ++row) {
        const Ticket &t = m_model->ticketAt(row);

        bool show = true;
        if (!search.isEmpty() && !t.title.toLower().contains(search))
            show = false;
        if (status != "All" && t.status != status)
            show = false;
        if (priority != "All" && t.priority != priority)
            show = false;

        ui->tableView->setRowHidden(row, !show);
        if (show)
            ++filtered;
    }

    ui->statusbar->showMessage(
        QString("Ready   Total: %1   Filtered: %2").arg(m_model->rowCount()).arg(filtered));
}
