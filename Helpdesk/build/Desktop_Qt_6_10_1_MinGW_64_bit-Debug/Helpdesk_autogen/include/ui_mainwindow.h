/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.10.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTableView>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QAction *actionNew;
    QAction *actionView;
    QAction *actionEdit;
    QAction *actionDelete;
    QAction *actionRefresh;
    QAction *actionOpenCsv;
    QAction *actionExportCsv;
    QAction *actionExit;
    QAction *actionShowToolbar;
    QAction *actionShowFilter;
    QAction *actionCollapseAll;
    QAction *actionAbout;
    QWidget *centralwidget;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *toolbarLayout;
    QPushButton *btnNew;
    QPushButton *btnView;
    QPushButton *btnEdit;
    QPushButton *btnDelete;
    QPushButton *btnRefresh;
    QSpacerItem *spacerItem;
    QHBoxLayout *filterLayout;
    QLabel *labelStatus;
    QComboBox *comboFilterStatus;
    QLabel *labelPriority;
    QComboBox *comboFilterPriority;
    QLineEdit *editSearch;
    QPushButton *btnClear;
    QTableView *tableView;
    QMenuBar *menubar;
    QMenu *menuFile;
    QMenu *menuTicket;
    QMenu *menuView;
    QMenu *menuHelp;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(800, 560);
        actionNew = new QAction(MainWindow);
        actionNew->setObjectName("actionNew");
        actionView = new QAction(MainWindow);
        actionView->setObjectName("actionView");
        actionView->setEnabled(false);
        actionEdit = new QAction(MainWindow);
        actionEdit->setObjectName("actionEdit");
        actionEdit->setEnabled(false);
        actionDelete = new QAction(MainWindow);
        actionDelete->setObjectName("actionDelete");
        actionDelete->setEnabled(false);
        actionRefresh = new QAction(MainWindow);
        actionRefresh->setObjectName("actionRefresh");
        actionOpenCsv = new QAction(MainWindow);
        actionOpenCsv->setObjectName("actionOpenCsv");
        actionExportCsv = new QAction(MainWindow);
        actionExportCsv->setObjectName("actionExportCsv");
        actionExit = new QAction(MainWindow);
        actionExit->setObjectName("actionExit");
        actionShowToolbar = new QAction(MainWindow);
        actionShowToolbar->setObjectName("actionShowToolbar");
        actionShowToolbar->setCheckable(true);
        actionShowToolbar->setChecked(true);
        actionShowFilter = new QAction(MainWindow);
        actionShowFilter->setObjectName("actionShowFilter");
        actionShowFilter->setCheckable(true);
        actionShowFilter->setChecked(true);
        actionCollapseAll = new QAction(MainWindow);
        actionCollapseAll->setObjectName("actionCollapseAll");
        actionAbout = new QAction(MainWindow);
        actionAbout->setObjectName("actionAbout");
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        verticalLayout = new QVBoxLayout(centralwidget);
        verticalLayout->setObjectName("verticalLayout");
        toolbarLayout = new QHBoxLayout();
        toolbarLayout->setObjectName("toolbarLayout");
        btnNew = new QPushButton(centralwidget);
        btnNew->setObjectName("btnNew");

        toolbarLayout->addWidget(btnNew);

        btnView = new QPushButton(centralwidget);
        btnView->setObjectName("btnView");
        btnView->setEnabled(false);

        toolbarLayout->addWidget(btnView);

        btnEdit = new QPushButton(centralwidget);
        btnEdit->setObjectName("btnEdit");
        btnEdit->setEnabled(false);

        toolbarLayout->addWidget(btnEdit);

        btnDelete = new QPushButton(centralwidget);
        btnDelete->setObjectName("btnDelete");
        btnDelete->setEnabled(false);

        toolbarLayout->addWidget(btnDelete);

        btnRefresh = new QPushButton(centralwidget);
        btnRefresh->setObjectName("btnRefresh");

        toolbarLayout->addWidget(btnRefresh);

        spacerItem = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        toolbarLayout->addItem(spacerItem);


        verticalLayout->addLayout(toolbarLayout);

        filterLayout = new QHBoxLayout();
        filterLayout->setObjectName("filterLayout");
        labelStatus = new QLabel(centralwidget);
        labelStatus->setObjectName("labelStatus");

        filterLayout->addWidget(labelStatus);

        comboFilterStatus = new QComboBox(centralwidget);
        comboFilterStatus->setObjectName("comboFilterStatus");
        comboFilterStatus->setMinimumWidth(110);

        filterLayout->addWidget(comboFilterStatus);

        labelPriority = new QLabel(centralwidget);
        labelPriority->setObjectName("labelPriority");

        filterLayout->addWidget(labelPriority);

        comboFilterPriority = new QComboBox(centralwidget);
        comboFilterPriority->setObjectName("comboFilterPriority");
        comboFilterPriority->setMinimumWidth(110);

        filterLayout->addWidget(comboFilterPriority);

        editSearch = new QLineEdit(centralwidget);
        editSearch->setObjectName("editSearch");

        filterLayout->addWidget(editSearch);

        btnClear = new QPushButton(centralwidget);
        btnClear->setObjectName("btnClear");

        filterLayout->addWidget(btnClear);


        verticalLayout->addLayout(filterLayout);

        tableView = new QTableView(centralwidget);
        tableView->setObjectName("tableView");

        verticalLayout->addWidget(tableView);

        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 800, 22));
        menuFile = new QMenu(menubar);
        menuFile->setObjectName("menuFile");
        menuTicket = new QMenu(menubar);
        menuTicket->setObjectName("menuTicket");
        menuView = new QMenu(menubar);
        menuView->setObjectName("menuView");
        menuHelp = new QMenu(menubar);
        menuHelp->setObjectName("menuHelp");
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);

        menubar->addAction(menuFile->menuAction());
        menubar->addAction(menuTicket->menuAction());
        menubar->addAction(menuView->menuAction());
        menubar->addAction(menuHelp->menuAction());
        menuFile->addAction(actionOpenCsv);
        menuFile->addAction(actionExportCsv);
        menuFile->addSeparator();
        menuFile->addAction(actionExit);
        menuTicket->addAction(actionNew);
        menuTicket->addAction(actionView);
        menuTicket->addAction(actionEdit);
        menuTicket->addAction(actionDelete);
        menuTicket->addSeparator();
        menuTicket->addAction(actionRefresh);
        menuView->addAction(actionShowToolbar);
        menuView->addAction(actionShowFilter);
        menuView->addSeparator();
        menuView->addAction(actionCollapseAll);
        menuHelp->addAction(actionAbout);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "Helpdesk", nullptr));
        actionNew->setText(QCoreApplication::translate("MainWindow", "New", nullptr));
#if QT_CONFIG(shortcut)
        actionNew->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+N", nullptr));
#endif // QT_CONFIG(shortcut)
        actionView->setText(QCoreApplication::translate("MainWindow", "View", nullptr));
        actionEdit->setText(QCoreApplication::translate("MainWindow", "Edit", nullptr));
#if QT_CONFIG(shortcut)
        actionEdit->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+E", nullptr));
#endif // QT_CONFIG(shortcut)
        actionDelete->setText(QCoreApplication::translate("MainWindow", "Delete", nullptr));
#if QT_CONFIG(shortcut)
        actionDelete->setShortcut(QCoreApplication::translate("MainWindow", "Del", nullptr));
#endif // QT_CONFIG(shortcut)
        actionRefresh->setText(QCoreApplication::translate("MainWindow", "Refresh", nullptr));
#if QT_CONFIG(shortcut)
        actionRefresh->setShortcut(QCoreApplication::translate("MainWindow", "F5", nullptr));
#endif // QT_CONFIG(shortcut)
        actionOpenCsv->setText(QCoreApplication::translate("MainWindow", "Open CSV...", nullptr));
#if QT_CONFIG(shortcut)
        actionOpenCsv->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+O", nullptr));
#endif // QT_CONFIG(shortcut)
        actionExportCsv->setText(QCoreApplication::translate("MainWindow", "Export to CSV...", nullptr));
#if QT_CONFIG(shortcut)
        actionExportCsv->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+S", nullptr));
#endif // QT_CONFIG(shortcut)
        actionExit->setText(QCoreApplication::translate("MainWindow", "Exit", nullptr));
#if QT_CONFIG(shortcut)
        actionExit->setShortcut(QCoreApplication::translate("MainWindow", "Alt+F4", nullptr));
#endif // QT_CONFIG(shortcut)
        actionShowToolbar->setText(QCoreApplication::translate("MainWindow", "Show Toolbar", nullptr));
        actionShowFilter->setText(QCoreApplication::translate("MainWindow", "Show Filter Bar", nullptr));
        actionCollapseAll->setText(QCoreApplication::translate("MainWindow", "Reset Column Widths", nullptr));
        actionAbout->setText(QCoreApplication::translate("MainWindow", "About", nullptr));
        btnNew->setText(QCoreApplication::translate("MainWindow", "New", nullptr));
        btnView->setText(QCoreApplication::translate("MainWindow", "View", nullptr));
        btnEdit->setText(QCoreApplication::translate("MainWindow", "Edit", nullptr));
        btnDelete->setText(QCoreApplication::translate("MainWindow", "Delete", nullptr));
        btnRefresh->setText(QCoreApplication::translate("MainWindow", "Refresh", nullptr));
        labelStatus->setText(QCoreApplication::translate("MainWindow", "Status:", nullptr));
        labelPriority->setText(QCoreApplication::translate("MainWindow", "Priority:", nullptr));
        editSearch->setPlaceholderText(QCoreApplication::translate("MainWindow", "Search...", nullptr));
        btnClear->setText(QCoreApplication::translate("MainWindow", "Clear", nullptr));
        menuFile->setTitle(QCoreApplication::translate("MainWindow", "File", nullptr));
        menuTicket->setTitle(QCoreApplication::translate("MainWindow", "Ticket", nullptr));
        menuView->setTitle(QCoreApplication::translate("MainWindow", "View", nullptr));
        menuHelp->setTitle(QCoreApplication::translate("MainWindow", "Help", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
