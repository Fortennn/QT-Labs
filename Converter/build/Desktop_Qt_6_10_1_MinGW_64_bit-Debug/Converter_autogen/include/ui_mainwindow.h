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
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QGridLayout *gridLayout_2;
    QGridLayout *gridLayout;
    QTabWidget *tabWidget;
    QWidget *length;
    QGridLayout *gridLayout_4;
    QComboBox *combo_length2;
    QGridLayout *gridLayout_3;
    QVBoxLayout *verticalLayout;
    QLineEdit *length1;
    QComboBox *combo_length1;
    QLineEdit *length2;
    QWidget *mass;
    QGridLayout *gridLayout_6;
    QGridLayout *gridLayout_5;
    QVBoxLayout *verticalLayout_2;
    QLineEdit *mass1;
    QComboBox *combo_mass1;
    QLineEdit *mass2;
    QComboBox *combo_mass2;
    QWidget *temperature;
    QGridLayout *gridLayout_8;
    QGridLayout *gridLayout_7;
    QVBoxLayout *verticalLayout_3;
    QLineEdit *temp1;
    QLineEdit *temp2;
    QComboBox *combo_temp1;
    QComboBox *combo_temp2;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(562, 259);
        MainWindow->setStyleSheet(QString::fromUtf8("QWidget {\n"
"    background-color: #0a0f0a;\n"
"    color: #e0ffe0;\n"
"    font-family: \"Segoe UI\", \"SF Pro Display\", -apple-system, sans-serif;\n"
"    font-size: 14px;\n"
"}\n"
"\n"
"QTabWidget {\n"
"    border: none;\n"
"    background-color: transparent;\n"
"}\n"
"\n"
"QTabWidget::pane {\n"
"    border: 1px solid #1a3d1a;\n"
"    border-radius: 10px;\n"
"    background-color: #0d130d;\n"
"    padding: 8px;\n"
"}\n"
"\n"
"QTabBar::tab {\n"
"    background: #0f1a0f;\n"
"    color: #80c080;\n"
"    padding: 10px 20px;\n"
"    margin-right: 4px;\n"
"    border-top-left-radius: 8px;\n"
"    border-top-right-radius: 8px;\n"
"    border: 1px solid #1a3d1a;\n"
"    border-bottom: none;\n"
"}\n"
"\n"
"QTabBar::tab:selected {\n"
"    background: #1a3d1a;\n"
"    color: #00ff00;\n"
"    border-bottom: 2px solid #00ff00;\n"
"}\n"
"\n"
"QTabBar::tab:hover:!selected {\n"
"    background: #152015;\n"
"    color: #a0d0a0;\n"
"}\n"
"\n"
"QLineEdit {\n"
"    background-color: #0f1a0f;\n"
"    border: 1px solid #1a3d1a"
                        ";\n"
"    border-radius: 8px;\n"
"    padding: 8px 12px;\n"
"    color: #e0ffe0;\n"
"    selection-background-color: #00ff00;\n"
"    selection-color: #000000;\n"
"}\n"
"\n"
"QLineEdit:focus {\n"
"    border: 1px solid #00ff00;\n"
"    background-color: #152015;\n"
"}\n"
"\n"
"QLineEdit:hover {\n"
"    border: 1px solid #2a5d2a;\n"
"}\n"
"\n"
"QComboBox {\n"
"    background-color: #0f1a0f;\n"
"    border: 1px solid #1a3d1a;\n"
"    border-radius: 8px;\n"
"    padding: 8px 12px;\n"
"    color: #e0ffe0;\n"
"    min-width: 100px;\n"
"}\n"
"\n"
"QComboBox:hover {\n"
"    border: 1px solid #2a5d2a;\n"
"    background-color: #152015;\n"
"}\n"
"\n"
"QComboBox:focus {\n"
"    border: 1px solid #00ff00;\n"
"}\n"
"\n"
"QComboBox::drop-down {\n"
"    border: none;\n"
"    width: 30px;\n"
"}\n"
"\n"
"QComboBox::down-arrow {\n"
"    image: none;\n"
"    border-left: 5px solid transparent;\n"
"    border-right: 5px solid transparent;\n"
"    border-top: 5px solid #80c080;\n"
"    margin-right: 8px;\n"
"}\n"
"\n"
"QComboBox "
                        "QAbstractItemView {\n"
"    background-color: #0f1a0f;\n"
"    border: 1px solid #1a3d1a;\n"
"    border-radius: 8px;\n"
"    selection-background-color: #00ff00;\n"
"    selection-color: #000000;\n"
"    padding: 4px;\n"
"}\n"
"\n"
"QPushButton {\n"
"    background-color: #1a3d1a;\n"
"    border: 1px solid #2a5d2a;\n"
"    border-radius: 8px;\n"
"    padding: 8px 16px;\n"
"    color: #e0ffe0;\n"
"}\n"
"\n"
"QPushButton:hover {\n"
"    background-color: #2a5d2a;\n"
"    border: 1px solid #00ff00;\n"
"}\n"
"\n"
"QPushButton:pressed {\n"
"    background-color: #0f1a0f;\n"
"}\n"
"\n"
"QScrollBar:vertical {\n"
"    background: #0d130d;\n"
"    width: 12px;\n"
"    border-radius: 6px;\n"
"}\n"
"\n"
"QScrollBar::handle:vertical {\n"
"    background: #1a3d1a;\n"
"    border-radius: 6px;\n"
"    min-height: 20px;\n"
"}\n"
"\n"
"QScrollBar::handle:vertical:hover {\n"
"    background: #2a5d2a;\n"
"}\n"
"\n"
"QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {\n"
"    height: 0px;\n"
"}\n"
"\n"
"QScrollBar:hor"
                        "izontal {\n"
"    background: #0d130d;\n"
"    height: 12px;\n"
"    border-radius: 6px;\n"
"}\n"
"\n"
"QScrollBar::handle:horizontal {\n"
"    background: #1a3d1a;\n"
"    border-radius: 6px;\n"
"    min-width: 20px;\n"
"}\n"
"\n"
"QScrollBar::handle:horizontal:hover {\n"
"    background: #2a5d2a;\n"
"}"));
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        gridLayout_2 = new QGridLayout(centralwidget);
        gridLayout_2->setObjectName("gridLayout_2");
        gridLayout = new QGridLayout();
        gridLayout->setObjectName("gridLayout");
        tabWidget = new QTabWidget(centralwidget);
        tabWidget->setObjectName("tabWidget");
        tabWidget->setMinimumSize(QSize(120, 160));
        length = new QWidget();
        length->setObjectName("length");
        gridLayout_4 = new QGridLayout(length);
        gridLayout_4->setObjectName("gridLayout_4");
        combo_length2 = new QComboBox(length);
        combo_length2->addItem(QString());
        combo_length2->addItem(QString());
        combo_length2->addItem(QString());
        combo_length2->addItem(QString());
        combo_length2->addItem(QString());
        combo_length2->setObjectName("combo_length2");
        QSizePolicy sizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(combo_length2->sizePolicy().hasHeightForWidth());
        combo_length2->setSizePolicy(sizePolicy);
        combo_length2->setMinimumSize(QSize(126, 0));

        gridLayout_4->addWidget(combo_length2, 1, 1, 1, 1);

        gridLayout_3 = new QGridLayout();
        gridLayout_3->setObjectName("gridLayout_3");
        verticalLayout = new QVBoxLayout();
        verticalLayout->setObjectName("verticalLayout");
        length1 = new QLineEdit(length);
        length1->setObjectName("length1");
        QSizePolicy sizePolicy1(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(100);
        sizePolicy1.setHeightForWidth(length1->sizePolicy().hasHeightForWidth());
        length1->setSizePolicy(sizePolicy1);
        length1->setMinimumSize(QSize(0, 35));

        verticalLayout->addWidget(length1);


        gridLayout_3->addLayout(verticalLayout, 1, 0, 1, 1);


        gridLayout_4->addLayout(gridLayout_3, 0, 0, 1, 1);

        combo_length1 = new QComboBox(length);
        combo_length1->addItem(QString());
        combo_length1->addItem(QString());
        combo_length1->addItem(QString());
        combo_length1->addItem(QString());
        combo_length1->addItem(QString());
        combo_length1->setObjectName("combo_length1");
        sizePolicy.setHeightForWidth(combo_length1->sizePolicy().hasHeightForWidth());
        combo_length1->setSizePolicy(sizePolicy);
        combo_length1->setMinimumSize(QSize(126, 0));

        gridLayout_4->addWidget(combo_length1, 0, 1, 1, 1);

        length2 = new QLineEdit(length);
        length2->setObjectName("length2");
        QSizePolicy sizePolicy2(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(length2->sizePolicy().hasHeightForWidth());
        length2->setSizePolicy(sizePolicy2);
        length2->setMinimumSize(QSize(50, 35));

        gridLayout_4->addWidget(length2, 1, 0, 1, 1);

        tabWidget->addTab(length, QString());
        mass = new QWidget();
        mass->setObjectName("mass");
        gridLayout_6 = new QGridLayout(mass);
        gridLayout_6->setObjectName("gridLayout_6");
        gridLayout_5 = new QGridLayout();
        gridLayout_5->setObjectName("gridLayout_5");
        verticalLayout_2 = new QVBoxLayout();
        verticalLayout_2->setObjectName("verticalLayout_2");
        mass1 = new QLineEdit(mass);
        mass1->setObjectName("mass1");
        mass1->setMinimumSize(QSize(0, 35));

        verticalLayout_2->addWidget(mass1);


        gridLayout_5->addLayout(verticalLayout_2, 0, 0, 1, 1);


        gridLayout_6->addLayout(gridLayout_5, 0, 0, 1, 1);

        combo_mass1 = new QComboBox(mass);
        combo_mass1->addItem(QString());
        combo_mass1->addItem(QString());
        combo_mass1->addItem(QString());
        combo_mass1->setObjectName("combo_mass1");
        combo_mass1->setMinimumSize(QSize(126, 0));

        gridLayout_6->addWidget(combo_mass1, 0, 1, 1, 1);

        mass2 = new QLineEdit(mass);
        mass2->setObjectName("mass2");
        mass2->setMinimumSize(QSize(0, 35));

        gridLayout_6->addWidget(mass2, 1, 0, 1, 1);

        combo_mass2 = new QComboBox(mass);
        combo_mass2->addItem(QString());
        combo_mass2->addItem(QString());
        combo_mass2->addItem(QString());
        combo_mass2->setObjectName("combo_mass2");
        combo_mass2->setMinimumSize(QSize(126, 0));

        gridLayout_6->addWidget(combo_mass2, 1, 1, 1, 1);

        tabWidget->addTab(mass, QString());
        temperature = new QWidget();
        temperature->setObjectName("temperature");
        gridLayout_8 = new QGridLayout(temperature);
        gridLayout_8->setObjectName("gridLayout_8");
        gridLayout_7 = new QGridLayout();
        gridLayout_7->setObjectName("gridLayout_7");
        verticalLayout_3 = new QVBoxLayout();
        verticalLayout_3->setObjectName("verticalLayout_3");
        temp1 = new QLineEdit(temperature);
        temp1->setObjectName("temp1");
        temp1->setMinimumSize(QSize(0, 35));

        verticalLayout_3->addWidget(temp1);


        gridLayout_7->addLayout(verticalLayout_3, 0, 0, 1, 1);


        gridLayout_8->addLayout(gridLayout_7, 0, 0, 1, 1);

        temp2 = new QLineEdit(temperature);
        temp2->setObjectName("temp2");
        temp2->setMinimumSize(QSize(0, 35));

        gridLayout_8->addWidget(temp2, 1, 0, 1, 1);

        combo_temp1 = new QComboBox(temperature);
        combo_temp1->addItem(QString());
        combo_temp1->addItem(QString());
        combo_temp1->addItem(QString());
        combo_temp1->setObjectName("combo_temp1");
        combo_temp1->setMinimumSize(QSize(126, 0));

        gridLayout_8->addWidget(combo_temp1, 0, 1, 1, 1);

        combo_temp2 = new QComboBox(temperature);
        combo_temp2->addItem(QString());
        combo_temp2->addItem(QString());
        combo_temp2->addItem(QString());
        combo_temp2->setObjectName("combo_temp2");
        combo_temp2->setMinimumSize(QSize(126, 0));

        gridLayout_8->addWidget(combo_temp2, 1, 1, 1, 1);

        tabWidget->addTab(temperature, QString());

        gridLayout->addWidget(tabWidget, 0, 0, 1, 1);


        gridLayout_2->addLayout(gridLayout, 0, 0, 1, 1);

        MainWindow->setCentralWidget(centralwidget);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);

        tabWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MainWindow", nullptr));
        combo_length2->setItemText(0, QCoreApplication::translate("MainWindow", "meters (m)", nullptr));
        combo_length2->setItemText(1, QCoreApplication::translate("MainWindow", "kilometers (km)", nullptr));
        combo_length2->setItemText(2, QCoreApplication::translate("MainWindow", "inches (in)", nullptr));
        combo_length2->setItemText(3, QCoreApplication::translate("MainWindow", "feet (ft)", nullptr));
        combo_length2->setItemText(4, QCoreApplication::translate("MainWindow", "miles (mi)", nullptr));

        combo_length1->setItemText(0, QCoreApplication::translate("MainWindow", "meters (m)", nullptr));
        combo_length1->setItemText(1, QCoreApplication::translate("MainWindow", "kilometers (km)", nullptr));
        combo_length1->setItemText(2, QCoreApplication::translate("MainWindow", "inches (in)", nullptr));
        combo_length1->setItemText(3, QCoreApplication::translate("MainWindow", "foot (ft)", nullptr));
        combo_length1->setItemText(4, QCoreApplication::translate("MainWindow", "miles (mi)", nullptr));

        tabWidget->setTabText(tabWidget->indexOf(length), QCoreApplication::translate("MainWindow", "Length", nullptr));
        combo_mass1->setItemText(0, QCoreApplication::translate("MainWindow", "kilograms (kg)", nullptr));
        combo_mass1->setItemText(1, QCoreApplication::translate("MainWindow", "pounds (lb)", nullptr));
        combo_mass1->setItemText(2, QCoreApplication::translate("MainWindow", "ounces (oz)", nullptr));

        combo_mass2->setItemText(0, QCoreApplication::translate("MainWindow", "kilograms (kg)", nullptr));
        combo_mass2->setItemText(1, QCoreApplication::translate("MainWindow", "pounds (lb)", nullptr));
        combo_mass2->setItemText(2, QCoreApplication::translate("MainWindow", "ounces (oz)", nullptr));

        tabWidget->setTabText(tabWidget->indexOf(mass), QCoreApplication::translate("MainWindow", "Mass", nullptr));
        combo_temp1->setItemText(0, QCoreApplication::translate("MainWindow", "Celsius (\302\260C)", nullptr));
        combo_temp1->setItemText(1, QCoreApplication::translate("MainWindow", "Fahrenheit (\302\260F)", nullptr));
        combo_temp1->setItemText(2, QCoreApplication::translate("MainWindow", "Kelvin (K)", nullptr));

        combo_temp2->setItemText(0, QCoreApplication::translate("MainWindow", "Celsius (\302\260C)", nullptr));
        combo_temp2->setItemText(1, QCoreApplication::translate("MainWindow", "Fahrenheit (\302\260F)", nullptr));
        combo_temp2->setItemText(2, QCoreApplication::translate("MainWindow", "Kelvin (K)", nullptr));

        tabWidget->setTabText(tabWidget->indexOf(temperature), QCoreApplication::translate("MainWindow", "Temperature", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
