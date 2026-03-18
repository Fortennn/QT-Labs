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
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPlainTextEdit>
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
    QLineEdit *lineEditTitle;
    QLabel *labelPriority;
    QComboBox *comboBoxPriority;
    QLabel *labelStatus;
    QComboBox *comboBoxStatus;
    QLabel *labelCreated;
    QLabel *labelCreatedValue;
    QLabel *labelDescription;
    QPlainTextEdit *plainTextEditDescription;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *TicketDialog)
    {
        if (TicketDialog->objectName().isEmpty())
            TicketDialog->setObjectName("TicketDialog");
        TicketDialog->resize(480, 360);
        verticalLayout = new QVBoxLayout(TicketDialog);
        verticalLayout->setSpacing(10);
        verticalLayout->setContentsMargins(16, 16, 16, 16);
        verticalLayout->setObjectName("verticalLayout");
        formLayout = new QFormLayout();
        formLayout->setObjectName("formLayout");
        formLayout->setHorizontalSpacing(12);
        formLayout->setVerticalSpacing(8);
        labelId = new QLabel(TicketDialog);
        labelId->setObjectName("labelId");

        formLayout->setWidget(0, QFormLayout::ItemRole::LabelRole, labelId);

        labelIdValue = new QLabel(TicketDialog);
        labelIdValue->setObjectName("labelIdValue");

        formLayout->setWidget(0, QFormLayout::ItemRole::FieldRole, labelIdValue);

        labelTitle = new QLabel(TicketDialog);
        labelTitle->setObjectName("labelTitle");

        formLayout->setWidget(1, QFormLayout::ItemRole::LabelRole, labelTitle);

        lineEditTitle = new QLineEdit(TicketDialog);
        lineEditTitle->setObjectName("lineEditTitle");

        formLayout->setWidget(1, QFormLayout::ItemRole::FieldRole, lineEditTitle);

        labelPriority = new QLabel(TicketDialog);
        labelPriority->setObjectName("labelPriority");

        formLayout->setWidget(2, QFormLayout::ItemRole::LabelRole, labelPriority);

        comboBoxPriority = new QComboBox(TicketDialog);
        comboBoxPriority->setObjectName("comboBoxPriority");

        formLayout->setWidget(2, QFormLayout::ItemRole::FieldRole, comboBoxPriority);

        labelStatus = new QLabel(TicketDialog);
        labelStatus->setObjectName("labelStatus");

        formLayout->setWidget(3, QFormLayout::ItemRole::LabelRole, labelStatus);

        comboBoxStatus = new QComboBox(TicketDialog);
        comboBoxStatus->setObjectName("comboBoxStatus");

        formLayout->setWidget(3, QFormLayout::ItemRole::FieldRole, comboBoxStatus);

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

        plainTextEditDescription = new QPlainTextEdit(TicketDialog);
        plainTextEditDescription->setObjectName("plainTextEditDescription");

        formLayout->setWidget(5, QFormLayout::ItemRole::FieldRole, plainTextEditDescription);


        verticalLayout->addLayout(formLayout);

        buttonBox = new QDialogButtonBox(TicketDialog);
        buttonBox->setObjectName("buttonBox");
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        verticalLayout->addWidget(buttonBox);


        retranslateUi(TicketDialog);

        QMetaObject::connectSlotsByName(TicketDialog);
    } // setupUi

    void retranslateUi(QDialog *TicketDialog)
    {
        TicketDialog->setWindowTitle(QCoreApplication::translate("TicketDialog", "Ticket", nullptr));
        labelId->setText(QCoreApplication::translate("TicketDialog", "ID:", nullptr));
        labelIdValue->setText(QString());
        labelTitle->setText(QCoreApplication::translate("TicketDialog", "Title:", nullptr));
        labelPriority->setText(QCoreApplication::translate("TicketDialog", "Priority:", nullptr));
        labelStatus->setText(QCoreApplication::translate("TicketDialog", "Status:", nullptr));
        labelCreated->setText(QCoreApplication::translate("TicketDialog", "Created:", nullptr));
        labelCreatedValue->setText(QString());
        labelDescription->setText(QCoreApplication::translate("TicketDialog", "Description:", nullptr));
    } // retranslateUi

};

namespace Ui {
    class TicketDialog: public Ui_TicketDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_TICKETDIALOG_H
