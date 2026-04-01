#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMessageBox>
#include <QClipboard>
#include <QApplication>
#include <QHeaderView>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_model(new PasswordTableModel(this))
    , m_proxyModel(new QSortFilterProxyModel(this))
{
    ui->setupUi(this);


    m_proxyModel->setSourceModel(m_model);
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxyModel->setFilterKeyColumn(-1);

    ui->tableViewPasswords->setModel(m_proxyModel);
    ui->tableViewPasswords->setEditTriggers(
        QAbstractItemView::DoubleClicked |
        QAbstractItemView::EditKeyPressed |
        QAbstractItemView::SelectedClicked
    );
    ui->tableViewPasswords->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->tableViewPasswords->horizontalHeader()->setStretchLastSection(false);
    ui->tableViewPasswords->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    ui->tableViewPasswords->horizontalHeader()->setSectionResizeMode(
        PasswordTableModel::ColTitle, QHeaderView::Stretch
    );
    ui->tableViewPasswords->setColumnWidth(PasswordTableModel::ColId,        40);
    ui->tableViewPasswords->setColumnWidth(PasswordTableModel::ColUsername,  140);
    ui->tableViewPasswords->setColumnWidth(PasswordTableModel::ColPassword,  100);
    ui->tableViewPasswords->setColumnWidth(PasswordTableModel::ColWebsite,   180);
    ui->tableViewPasswords->setColumnWidth(PasswordTableModel::ColCategory,  100);
    ui->tableViewPasswords->setColumnWidth(PasswordTableModel::ColUpdatedAt, 140);
    ui->tableViewPasswords->verticalHeader()->setVisible(false);

    connect(ui->actionNew,          &QAction::triggered, this, &MainWindow::onNewTriggered);
    connect(ui->actionEdit,         &QAction::triggered, this, &MainWindow::onEditTriggered);
    connect(ui->actionDelete,       &QAction::triggered, this, &MainWindow::onDeleteTriggered);
    connect(ui->actionSave,         &QAction::triggered, this, &MainWindow::onSaveTriggered);
    connect(ui->actionCopyUsername, &QAction::triggered, this, &MainWindow::onCopyUsernameTriggered);
    connect(ui->actionCopyPassword, &QAction::triggered, this, &MainWindow::onCopyPasswordTriggered);
    connect(ui->actionExit,         &QAction::triggered, this, &QMainWindow::close);

    ui->mainToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    connect(ui->searchEdit,   &QLineEdit::textChanged, this, &MainWindow::onSearchTextChanged);
    connect(ui->clearButton,  &QPushButton::clicked,   this, &MainWindow::onClearSearch);
    connect(ui->comboCategory, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onCategoryFilterChanged);

    connect(m_model, &QAbstractItemModel::rowsInserted,   this, &MainWindow::updateStatusBar);
    connect(m_model, &QAbstractItemModel::rowsRemoved,    this, &MainWindow::updateStatusBar);
    connect(m_proxyModel, &QAbstractItemModel::modelReset, this, &MainWindow::updateStatusBar);
    connect(m_proxyModel, &QSortFilterProxyModel::rowsInserted, this, &MainWindow::updateStatusBar);
    connect(m_proxyModel, &QSortFilterProxyModel::rowsRemoved,  this, &MainWindow::updateStatusBar);

    updateStatusBar();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onNewTriggered()
{
    PasswordEntry entry;
    entry.title = "New Entry";
    m_model->addEntry(entry);

    int newRow = m_model->rowCount() - 1;
    QModelIndex sourceIndex = m_model->index(newRow, PasswordTableModel::ColTitle);
    QModelIndex proxyIndex = m_proxyModel->mapFromSource(sourceIndex);

    ui->tableViewPasswords->setCurrentIndex(proxyIndex);
    ui->tableViewPasswords->scrollTo(proxyIndex);
    ui->tableViewPasswords->edit(proxyIndex);

    updateStatusBar();
}

void MainWindow::onEditTriggered()
{
    QModelIndex current = ui->tableViewPasswords->currentIndex();
    if (!current.isValid())
        return;
    ui->tableViewPasswords->edit(current);
}

void MainWindow::onDeleteTriggered()
{
    QModelIndex current = ui->tableViewPasswords->currentIndex();
    if (!current.isValid())
        return;

    const auto answer = QMessageBox::question(
        this,
        "Delete Entry",
        "Delete the selected entry?",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );

    if (answer == QMessageBox::Yes) {
        QModelIndex sourceIndex = m_proxyModel->mapToSource(current);
        m_model->removeEntry(sourceIndex.row());
    }

    updateStatusBar();
}

void MainWindow::onSaveTriggered()
{
    ui->statusBar->showMessage("Changes saved.", 3000);
}

void MainWindow::onCopyUsernameTriggered()
{
    QModelIndex current = ui->tableViewPasswords->currentIndex();
    if (!current.isValid())
        return;

    QModelIndex sourceIndex = m_proxyModel->mapToSource(current);
    QString username = m_model->entryAt(sourceIndex.row()).username;
    QApplication::clipboard()->setText(username);
    ui->statusBar->showMessage("Username copied to clipboard.", 2000);
}

void MainWindow::onCopyPasswordTriggered()
{
    QModelIndex current = ui->tableViewPasswords->currentIndex();
    if (!current.isValid())
        return;

    QModelIndex sourceIndex = m_proxyModel->mapToSource(current);
    QString password = m_model->entryAt(sourceIndex.row()).password;
    QApplication::clipboard()->setText(password);
    ui->statusBar->showMessage("Password copied to clipboard.", 2000);
}

void MainWindow::onSearchTextChanged(const QString &text)
{
    m_proxyModel->setFilterFixedString(text);
    updateStatusBar();
}

void MainWindow::onClearSearch()
{
    ui->searchEdit->clear();
    ui->comboCategory->setCurrentIndex(0);
}

void MainWindow::onCategoryFilterChanged(int index)
{
    if (index == 0) {
        m_proxyModel->setFilterKeyColumn(-1);
        m_proxyModel->setFilterFixedString(ui->searchEdit->text());
    } else {
        QString category = ui->comboCategory->currentText();
        m_proxyModel->setFilterKeyColumn(PasswordTableModel::ColCategory);
        m_proxyModel->setFilterFixedString(category);
    }
    updateStatusBar();
}

void MainWindow::updateStatusBar()
{
    int total    = m_model->rowCount();
    int filtered = m_proxyModel->rowCount();
    ui->statusBar->showMessage(
        QString("Total: %1   Filtered: %2").arg(total).arg(filtered)
    );
}
