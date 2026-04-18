#pragma once

#include <QMainWindow>
#include "passwordfilterproxymodel.h"
#include "passwordtablemodel.h"
#include "databasemanager.h"
#include "passwordrepository.h"
#include "passwordleakchecker.h"
#include <QThread>
#include <QProgressBar>
#include "passwordbatchworker.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onNewTriggered();
    void onEditTriggered();
    void onDeleteTriggered();
    void onSaveTriggered();
    void onCopyUsernameTriggered();
    void onCopyPasswordTriggered();
    void onSearchTextChanged(const QString &text);
    void onClearSearch();
    void onCategoryFilterChanged(int index);
    void updateStatusBar();

    void onNewDBTriggered();
    void onDeleteDBTriggered();
    void onAboutTriggered();

    void onCheckPasswordTriggered();
    void onLeakCheckCompleted(bool isLeaked, int count);
    void onLeakCheckError(const QString &errorMessage);

    void onCheckAllTriggered();
    void onCancelCheckTriggered();
    void onBatchProgressChanged(int current, int total);
    void onBatchEntryChecked(int id, bool isLeaked, int count, const QString &err);
    void onBatchFinished();

private:
    void reloadFromDatabase();
    void applyModernUI();

    Ui::MainWindow *ui;
    PasswordTableModel       *m_model;
    PasswordFilterProxyModel *m_proxyModel;
    DatabaseManager           m_dbManager;
    PasswordRepository    *m_repository = nullptr;
    PasswordLeakChecker   *m_leakChecker;

    QAction               *m_actionCheckAll;
    QAction               *m_actionCancelCheck;
    QProgressBar          *m_progressBar;
    QThread               *m_batchThread = nullptr;
    PasswordBatchWorker   *m_batchWorker = nullptr;
};
