#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMessageBox>
#include <QClipboard>
#include <QApplication>
#include <QHeaderView>
#include <QDebug>
#include <QFileDialog>
#include <QFile>
#include "addentrydialog.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_model(new PasswordTableModel(this))
    , m_proxyModel(new PasswordFilterProxyModel(this))
{
    ui->setupUi(this);

    QAction *actionNewDB = new QAction("New Database...", this);
    QAction *actionDeleteDB = new QAction("Delete Database...", this);
    ui->menuTools->addAction(actionNewDB);
    ui->menuTools->addAction(actionDeleteDB);

    QAction *actionAbout = new QAction("About Password Manager", this);
    QAction *actionAboutQt = new QAction("About Qt", this);
    ui->menuHelp->addAction(actionAbout);
    ui->menuHelp->addAction(actionAboutQt);

    connect(actionNewDB, &QAction::triggered, this, &MainWindow::onNewDBTriggered);
    connect(actionDeleteDB, &QAction::triggered, this, &MainWindow::onDeleteDBTriggered);
    connect(actionAbout, &QAction::triggered, this, &MainWindow::onAboutTriggered);
    connect(actionAboutQt, &QAction::triggered, qApp, &QApplication::aboutQt);

    if (!m_dbManager.open("passwords.db")) {
        QMessageBox::critical(this, "Database Error", "Failed to open the database.");
    } else if (!m_dbManager.initializeSchema()) {
        QMessageBox::critical(this, "Database Error", "Failed to initialize the database schema.");
    } else {
        m_repository = new PasswordRepository(m_dbManager.database());
        m_model->setRepository(m_repository);
        reloadFromDatabase();
    }

    m_proxyModel->setSourceModel(m_model);

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
    ui->tableViewPasswords->setColumnWidth(PasswordTableModel::ColId,        70);
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

    connect(ui->searchEdit,    &QLineEdit::textChanged, this, &MainWindow::onSearchTextChanged);
    connect(ui->clearButton,   &QPushButton::clicked,   this, &MainWindow::onClearSearch);
    connect(ui->comboCategory, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onCategoryFilterChanged);

    connect(m_model, &QAbstractItemModel::rowsInserted,  this, &MainWindow::updateStatusBar);
    connect(m_model, &QAbstractItemModel::rowsRemoved,   this, &MainWindow::updateStatusBar);
    connect(m_proxyModel, &QAbstractItemModel::modelReset,         this, &MainWindow::updateStatusBar);
    connect(m_proxyModel, &QSortFilterProxyModel::rowsInserted,    this, &MainWindow::updateStatusBar);
    connect(m_proxyModel, &QSortFilterProxyModel::rowsRemoved,     this, &MainWindow::updateStatusBar);

    applyModernUI();
    updateStatusBar();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::reloadFromDatabase()
{
    if (!m_repository)
        return;
    m_model->setEntries(m_repository->loadAll());
}

void MainWindow::onNewTriggered()
{
    if (!m_repository) return;

    AddEntryDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    PasswordEntry entry = dialog.getEntry();
    if (entry.title.isEmpty()) {
        QMessageBox::warning(this, "Validation", "Title cannot be empty.");
        return;
    }

    if (!m_repository->insert(entry)) {
        QMessageBox::warning(this, "Database Error", "Failed to insert the entry.");
        return;
    }
    m_model->addEntry(entry);

    int newRow = m_model->rowCount() - 1;
    QModelIndex sourceIndex = m_model->index(newRow, PasswordTableModel::ColTitle);
    QModelIndex proxyIndex  = m_proxyModel->mapFromSource(sourceIndex);

    ui->tableViewPasswords->setCurrentIndex(proxyIndex);
    ui->tableViewPasswords->scrollTo(proxyIndex);

    updateStatusBar();
}

void MainWindow::onEditTriggered()
{
    QModelIndex current = ui->tableViewPasswords->currentIndex();
    if (!current.isValid())
        return;

    QModelIndex sourceIndex = m_proxyModel->mapToSource(current);
    int row = sourceIndex.row();
    PasswordEntry entry = m_model->entryAt(row);

    AddEntryDialog dialog(this);
    dialog.setEditingEntry(entry);
    if (dialog.exec() != QDialog::Accepted)
        return;

    PasswordEntry updatedEntry = dialog.getEntry();

    if (updatedEntry.title.isEmpty()) {
        QMessageBox::warning(this, "Validation", "Title cannot be empty.");
        return;
    }

    if (!m_repository->update(updatedEntry)) {
        QMessageBox::warning(this, "Database Error", "Failed to update the entry.");
        return;
    }
    
    m_model->updateEntry(row, updatedEntry);
    updateStatusBar();
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
        int row = sourceIndex.row();
        PasswordEntry entry = m_model->entryAt(row);

        if (m_repository && !m_repository->remove(entry.id)) {
            QMessageBox::warning(this, "Database Error", "Failed to delete the entry.");
            return;
        }
        m_model->removeEntry(row);
    }

    updateStatusBar();
}

void MainWindow::onSaveTriggered()
{
    ui->statusBar->showMessage("All changes are saved to the database.", 3000);
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
    m_proxyModel->setSearchText(text);
    updateStatusBar();
}

void MainWindow::onClearSearch()
{
    ui->searchEdit->clear();
    ui->comboCategory->setCurrentIndex(0);
}

void MainWindow::onCategoryFilterChanged(int index)
{
    Q_UNUSED(index);
    m_proxyModel->setCategoryFilter(ui->comboCategory->currentText());
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

void MainWindow::applyModernUI()
{
    QString modernQSS = R"(
        QMainWindow {
            background-color: #1E1E2E;
        }
        
        QMenuBar {
            background-color: #181825;
            color: #CDD6F4;
            border-bottom: 1px solid #313244;
        }
        QMenuBar::item:selected {
            background-color: #313244;
            border-radius: 4px;
        }
        QMenu {
            background-color: #181825;
            color: #CDD6F4;
            border: 1px solid #313244;
        }
        QMenu::item:selected {
            background-color: #89B4FA;
            color: #11111B;
        }

        QToolBar {
            background-color: #181825;
            border-bottom: 1px solid #313244;
            spacing: 10px;
            padding: 5px;
            color: #CDD6F4;
        }
        QToolButton {
            border-radius: 6px;
            padding: 4px 8px;
            color: #CDD6F4;
            background: transparent;
        }
        QToolButton:hover {
            background-color: #313244;
        }
        QToolButton:pressed {
            background-color: #45475A;
        }

        QWidget#centralWidget {
            background-color: #1E1E2E;
        }

        QLabel {
            color: #CDD6F4;
            font-size: 14px;
        }

        QLineEdit, QComboBox {
            background-color: #11111B;
            border: 1px solid #313244;
            border-radius: 6px;
            padding: 6px 10px;
            color: #CDD6F4;
            font-size: 14px;
        }
        QLineEdit:focus, QComboBox:focus {
            border: 1px solid #89B4FA;
        }
        QComboBox::drop-down {
            border: none;
            width: 24px;
        }
        QComboBox QAbstractItemView {
            background-color: #181825;
            color: #CDD6F4;
            border: 1px solid #313244;
            selection-background-color: #89B4FA;
            selection-color: #11111B;
        }

        QPushButton {
            background-color: #313244;
            color: #CDD6F4;
            border: none;
            border-radius: 6px;
            padding: 6px 16px;
            font-size: 14px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #45475A;
        }
        QPushButton:pressed {
            background-color: #585B70;
        }

        QTableView {
            background-color: #181825;
            alternate-background-color: #1E1E2E;
            color: #CDD6F4;
            gridline-color: #313244;
            border: 1px solid #313244;
            border-radius: 8px;
            font-size: 12px;
            selection-background-color: #313244;
            selection-color: #CDD6F4;
            outline: none;
        }
        QTableView::item {
            padding: 4px;
        }
        QTableView::item:selected {
            background-color: #313244;
            color: #CDD6F4;
        }
        QTableView::item:focus {
            border: none;
            background-color: #45475A;
            color: #CDD6F4;
        }
        QTableView QLineEdit {
            background-color: #11111B;
            color: #CDD6F4;
            border: 1px solid #89B4FA;
            border-radius: 2px;
            padding: 2px 4px;
            font-size: 12px;
            selection-background-color: #45475A;
            selection-color: #CDD6F4;
        }
        QHeaderView::section {
            background-color: #11111B;
            color: #A6ADC8;
            padding: 8px;
            border: none;
            border-right: 1px solid #313244;
            border-bottom: 1px solid #313244;
            font-weight: bold;
            font-size: 13px;
        }
        QHeaderView::section:last {
            border-right: none;
        }

        QStatusBar {
            background-color: #11111B;
            color: #A6ADC8;
            border-top: 1px solid #313244;
        }
        
        /* Scrollbars */
        QScrollBar:vertical {
            border: none;
            background: #181825;
            width: 12px;
            margin: 0px 0px 0px 0px;
        }
        QScrollBar::handle:vertical {
            background: #45475A;
            min-height: 20px;
            border-radius: 6px;
        }
        QScrollBar::handle:vertical:hover {
            background: #585B70;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            border: none;
            background: none;
        }
        
        QScrollBar:horizontal {
            border: none;
            background: #181825;
            height: 12px;
            margin: 0px 0px 0px 0px;
        }
        QScrollBar::handle:horizontal {
            background: #45475A;
            min-width: 20px;
            border-radius: 6px;
        }
        QScrollBar::handle:horizontal:hover {
            background: #585B70;
        }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            border: none;
            background: none;
        }
    )";
    qApp->setStyleSheet(modernQSS);
}

void MainWindow::onNewDBTriggered()
{
    QString fileName = QFileDialog::getSaveFileName(this, "New Database", "passwords.db", "SQLite Database (*.db)");
    if (!fileName.isEmpty()) {
        m_dbManager.database().close(); 
        
        if (m_dbManager.open(fileName) && m_dbManager.initializeSchema()) {
            delete m_repository;
            m_repository = new PasswordRepository(m_dbManager.database());
            m_model->setRepository(m_repository);
            reloadFromDatabase();
            QMessageBox::information(this, "Success", "Switched to new database:\n" + fileName);
        } else {
            QMessageBox::critical(this, "Error", "Failed to create or open database.");
        }
    }
}

void MainWindow::onDeleteDBTriggered()
{
    QString dbName = m_dbManager.database().databaseName();
    int ret = QMessageBox::warning(this, "Delete Database", 
        "Are you sure you want to completely delete the current database file?\nThis action cannot be undone.",
        QMessageBox::Yes | QMessageBox::No);
        
    if (ret == QMessageBox::Yes) {
        QString connectionName = m_dbManager.database().connectionName();
        m_dbManager.database().close();
        m_dbManager = DatabaseManager();
        QSqlDatabase::removeDatabase(connectionName);
        
        if (QFile::remove(dbName)) {
            QMessageBox::information(this, "Deleted", "Database file deleted successfully.\nPlease create a new database to continue.");
            m_model->setRepository(nullptr); 
            delete m_repository;
            m_repository = nullptr;
        } else {
            QMessageBox::critical(this, "Error", "Failed to delete database file.");
        }
    }
}

void MainWindow::onAboutTriggered()
{
    QMessageBox::about(this, "About Password Manager",
                       "<h2>Password Manager</h2>"
                       "<p>A secure, intuitive, and modern application for managing your credentials locally.</p>"
                       "<p>Built with ❤️ using <b>C++</b> and <b>Qt</b> framework.</p>");
}
