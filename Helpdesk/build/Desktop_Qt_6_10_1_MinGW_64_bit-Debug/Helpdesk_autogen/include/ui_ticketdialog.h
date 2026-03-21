/********************************************************************************
** Form generated from reading UI file 'ticketdialog.ui'
**
** Created by: Qt User Interface Compiler version 6.10.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_TICKETDIALOG_H
#define UI_TICKETDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_TicketDialog
{
public:
    QVBoxLayout *verticalLayout;
    QFormLayout *formLayout;
    QLabel *labelId;
    QLabel *labelIdValue;
    QLabel *labelTitle;
    QVBoxLayout *vboxLayout;
    QLineEdit *editTitle;
    QLabel *labelTitleError;
    QLabel *labelPriority;
    QComboBox *comboPriority;
    QLabel *labelStatus;
    QComboBox *comboStatus;
    QLabel *labelCreated;
    QLabel *labelCreatedValue;
    QLabel *labelDescription;
    QPlainTextEdit *editDescription;
    QHBoxLayout *buttonsLayout;
    QSpacerItem *spacerItem;
    QPushButton *btnEdit;
    QPushButton *btnClose;
    QPushButton *btnSave;
    QPushButton *btnCancel;

    void setupUi(QDialog *TicketDialog)
    {
        if (TicketDialog->objectName().isEmpty())
            TicketDialog->setObjectName("TicketDialog");
        TicketDialog->resize(480, 420);
        verticalLayout = new QVBoxLayout(TicketDialog);
        verticalLayout->setObjectName("verticalLayout");
        formLayout = new QFormLayout();
        formLayout->setObjectName("formLayout");
        labelId = new QLabel(TicketDialog);
        labelId->setObjectName("labelId");

        formLayout->setWidget(0, QFormLayout::ItemRole::LabelRole, labelId);

        labelIdValue = new QLabel(TicketDialog);
        labelIdValue->setObjectName("labelIdValue");

        formLayout->setWidget(0, QFormLayout::ItemRole::FieldRole, labelIdValue);

        labelTitle = new QLabel(TicketDialog);
        labelTitle->setObjectName("labelTitle");

        formLayout->setWidget(1, QFormLayout::ItemRole::LabelRole, labelTitle);

        vboxLayout = new QVBoxLayout();
        vboxLayout->setObjectName("vboxLayout");
        editTitle = new QLineEdit(TicketDialog);
        editTitle->setObjectName("editTitle");

        vboxLayout->addWidget(editTitle);

        labelTitleError = new QLabel(TicketDialog);
        labelTitleError->setObjectName("labelTitleError");

        vboxLayout->addWidget(labelTitleError);


        formLayout->setLayout(1, QFormLayout::ItemRole::FieldRole, vboxLayout);

        labelPriority = new QLabel(TicketDialog);
        labelPriority->setObjectName("labelPriority");

        formLayout->setWidget(2, QFormLayout::ItemRole::LabelRole, labelPriority);

        comboPriority = new QComboBox(TicketDialog);
        comboPriority->setObjectName("comboPriority");

        formLayout->setWidget(2, QFormLayout::ItemRole::FieldRole, comboPriority);

        labelStatus = new QLabel(TicketDialog);
        labelStatus->setObjectName("labelStatus");

        formLayout->setWidget(3, QFormLayout::ItemRole::LabelRole, labelStatus);

        comboStatus = new QComboBox(TicketDialog);
        comboStatus->setObjectName("comboStatus");

        formLayout->setWidget(3, QFormLayout::ItemRole::FieldRole, comboStatus);

        labelCreated = new QLabel(TicketDialog);
        labelCreated->setObjectName("labelCreated");

        formLayout->setWidget(4, QFormLayout::ItemRole::LabelRole, labelCreated);

        labelCreatedValue = new QLabel(TicketDialog);
        labelCreatedValue->setObjectName("labelCreatedValue");

        formLayout->setWidget(4, QFormLayout::ItemRole::FieldRole, labelCreatedValue);

        labelDescription = new QLabel(TicketDialog);
        labelDescription->setObjectName("labelDescription");
        labelDescription->setAlignment(Qt::AlignTop);

        formLayout->setWidget(5, QFormLayout::ItemRole::LabelRole, labelDescription);

        editDescription = new QPlainTextEdit(TicketDialog);
        editDescription->setObjectName("editDescription");
        editDescription->setMinimumSize(QSize(0, 100));

        formLayout->setWidget(5, QFormLayout::ItemRole::FieldRole, editDescription);


        verticalLayout->addLayout(formLayout);

        buttonsLayout = new QHBoxLayout();
        buttonsLayout->setObjectName("buttonsLayout");
        spacerItem = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        buttonsLayout->addItem(spacerItem);

        btnEdit = new QPushButton(TicketDialog);
        btnEdit->setObjectName("btnEdit");

        buttonsLayout->addWidget(btnEdit);

        btnClose = new QPushButton(TicketDialog);
        btnClose->setObjectName("btnClose");

        buttonsLayout->addWidget(btnClose);

        btnSave = new QPushButton(TicketDialog);
        btnSave->setObjectName("btnSave");

        buttonsLayout->addWidget(btnSave);

        btnCancel = new QPushButton(TicketDialog);
        btnCancel->setObjectName("btnCancel");

        buttonsLayout->addWidget(btnCancel);


        verticalLayout->addLayout(buttonsLayout);


        retranslateUi(TicketDialog);

        QMetaObject::connectSlotsByName(TicketDialog);
    } // setupUi

    void retranslateUi(QDialog *TicketDialog)
    {
        TicketDialog->setWindowTitle(QCoreApplication::translate("TicketDialog", "Ticket", nullptr));
        labelId->setText(QCoreApplication::translate("TicketDialog", "ID:", nullptr));
        labelIdValue->setText(QCoreApplication::translate("TicketDialog", "-", nullptr));
        labelTitle->setText(QCoreApplication::translate("TicketDialog", "Title:", nullptr));
        labelTitleError->setText(QString());
        labelTitleError->setStyleSheet(QCoreApplication::translate("TicketDialog", "color: red;", nullptr));
        labelPriority->setText(QCoreApplication::translate("TicketDialog", "Priority:", nullptr));
        labelStatus->setText(QCoreApplication::translate("TicketDialog", "Status:", nullptr));
        labelCreated->setText(QCoreApplication::translate("TicketDialog", "Created:", nullptr));
        labelCreatedValue->setText(QCoreApplication::translate("TicketDialog", "-", nullptr));
        labelDescription->setText(QCoreApplication::translate("TicketDialog", "Description:", nullptr));
        btnEdit->setText(QCoreApplication::translate("TicketDialog", "Edit", nullptr));
        btnClose->setText(QCoreApplication::translate("TicketDialog", "Close", nullptr));
        btnSave->setText(QCoreApplication::translate("TicketDialog", "Save", nullptr));
        btnCancel->setText(QCoreApplication::translate("TicketDialog", "Cancel", nullptr));
    } // retranslateUi

};

namespace Ui {
    class TicketDialog: public Ui_TicketDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_TICKETDIALOG_H
