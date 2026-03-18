#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ticketdialog.h"

#include <QHeaderView>
#include <QMessageBox>
#include <QToolBar>
#include <QLabel>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_model(new TicketTableModel(this))
{
    ui->setupUi(this);

    setupTable();
    populateSampleData();

    QToolBar *tb = addToolBar("Main");
    tb->setMovable(false);
    tb->addAction(ui->actionNew);
    tb->addAction(ui->actionView);
    tb->addAction(ui->actionEdit);
    tb->addAction(ui->actionDelete);
    tb->addSeparator();
    tb->addAction(ui->actionRefresh);

    connect(ui->actionNew,     &QAction::triggered, this, &MainWindow::onNewTicket);
    connect(ui->actionView,    &QAction::triggered, this, &MainWindow::onViewTicket);
    connect(ui->actionEdit,    &QAction::triggered, this, &MainWindow::onEditTicket);
    connect(ui->actionDelete,  &QAction::triggered, this, &MainWindow::onDeleteTicket);
    connect(ui->actionRefresh, &QAction::triggered, this, &MainWindow::onRefresh);
    connect(ui->actionQuit,    &QAction::triggered, this, &QMainWindow::close);
    connect(ui->actionAbout,   &QAction::triggered, this, [this] {
        QMessageBox::about(this, "Про програму", "HelpDesk v 2.0");
    });

    connect(ui->tableView, &QTableView::doubleClicked,
            this, &MainWindow::onTableDoubleClicked);

    connect(ui->tableView->selectionModel(),
            &QItemSelectionModel::selectionChanged,
            this, &MainWindow::updateActions);

    updateActions();
    statusBar()->showMessage("Готово", 3000);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupTable()
{
    ui->tableView->setModel(m_model);
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableView->setAlternatingRowColors(true);
    ui->tableView->verticalHeader()->setDefaultSectionSize(26);
    ui->tableView->verticalHeader()->hide();

    auto *h = ui->tableView->horizontalHeader();
    h->setSectionResizeMode(TicketTableModel::ColId,        QHeaderView::ResizeToContents);
    h->setSectionResizeMode(TicketTableModel::ColTitle,     QHeaderView::Stretch);
    h->setSectionResizeMode(TicketTableModel::ColPriority,  QHeaderView::ResizeToContents);
    h->setSectionResizeMode(TicketTableModel::ColStatus,    QHeaderView::ResizeToContents);
    h->setSectionResizeMode(TicketTableModel::ColCreatedAt, QHeaderView::ResizeToContents);
    h->setHighlightSections(false);
}

void MainWindow::populateSampleData()
{
    const QVector<std::tuple<QString, QString, QString, QString>> samples = {
        { "Не підключається VPN",                        "High",     "Open",        "VPN-клієнт не запускається щоранку після останнього оновлення." },
        { "Принтер офлайн на 2-му поверсі",              "Medium",   "In Progress", "HP LaserJet відображається як офлайн у черзі друку Windows." },
        { "Пошта не синхронізується на телефоні",        "Low",      "Open",        "Outlook Mobile перестав синхронізуватись після оновлення iOS." },
        { "Ліцензія ПЗ закінчилась",                     "Critical", "Open",        "Ліцензії Adobe CC прострочені, дизайнери не можуть працювати." },
        { "Повільний Wi-Fi у конференц-залі А",          "Medium",   "Resolved",    "Точку доступу замінено, швидкість відновлено." },
        { "Налаштування ноутбука нового працівника",     "Low",      "Closed",      "Dell XPS налаштовано та передано співробітнику." },
        { "Спрацювання антивірусу на ПК-042",            "High",     "In Progress", "Загрозу поміщено на карантин, виконується повне сканування." },
        { "Мерехтіння монітора на робочому місці 14",    "Low",      "Open",        "Можливо, пошкоджено кабель DisplayPort." },
    };

    int day = 0;
    for (auto &[title, priority, status, desc] : samples) {
        Ticket t;
        t.title       = title;
        t.priority    = priority;
        t.status      = status;
        t.description = desc;
        t.createdAt   = QDateTime::currentDateTime().addDays(-day++);
        m_model->addTicket(t);
    }
}

int MainWindow::selectedRow() const
{
    const auto rows = ui->tableView->selectionModel()->selectedRows();
    return rows.isEmpty() ? -1 : rows.first().row();
}

void MainWindow::updateActions()
{
    const bool has = selectedRow() >= 0;
    ui->actionView->setEnabled(has);
    ui->actionEdit->setEnabled(has);
    ui->actionDelete->setEnabled(has);

    const int total = m_model->rowCount();
    statusBar()->showMessage(QString("Всього: %1 заявок").arg(total));
}

void MainWindow::onNewTicket()
{
    TicketDialog dlg(TicketDialog::ModeAdd, this);
    if (dlg.exec() != QDialog::Accepted)
        return;

    Ticket t = dlg.ticket();
    if (t.title.isEmpty()) {
        QMessageBox::warning(this, "Перевірка", "Назва не може бути порожньою.");
        return;
    }
    t.createdAt = QDateTime::currentDateTime();
    m_model->addTicket(t);
    updateActions();
}

void MainWindow::onViewTicket()
{
    const int row = selectedRow();
    if (row < 0) return;

    TicketDialog dlg(TicketDialog::ModeView, this);
    dlg.setTicket(m_model->ticketAt(row));
    if (dlg.exec() == QDialog::Accepted) {
        Ticket t = dlg.ticket();
        if (t.title.isEmpty()) {
            QMessageBox::warning(this, "Перевірка", "Назва не може бути порожньою.");
            return;
        }
        m_model->updateTicket(row, t);
        updateActions();
    }
}

void MainWindow::onEditTicket()
{
    const int row = selectedRow();
    if (row < 0) return;

    TicketDialog dlg(TicketDialog::ModeEdit, this);
    dlg.setTicket(m_model->ticketAt(row));
    if (dlg.exec() != QDialog::Accepted)
        return;

    Ticket t = dlg.ticket();
    if (t.title.isEmpty()) {
        QMessageBox::warning(this, "Перевірка", "Назва не може бути порожньою.");
        return;
    }
    m_model->updateTicket(row, t);
    updateActions();
}

void MainWindow::onDeleteTicket()
{
    const int row = selectedRow();
    if (row < 0) return;

    const Ticket t = m_model->ticketAt(row);
    const auto answer = QMessageBox::question(
        this, "Видалення заявки",
        QString("Видалити заявку #%1 \"%2\"?").arg(t.id).arg(t.title),
        QMessageBox::Yes | QMessageBox::No);

    if (answer != QMessageBox::Yes)
        return;

    m_model->removeTicket(row);
    updateActions();
}

void MainWindow::onRefresh()
{
    statusBar()->showMessage("Оновлено", 2000);
    updateActions();
}

void MainWindow::onTableDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid()) return;
    onViewTicket();
}
