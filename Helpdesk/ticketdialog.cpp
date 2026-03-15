#include "ticketdialog.h"
#include "ui_ticketdialog.h"

TicketDialog::TicketDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::TicketDialog)
    , m_mode(ModeView)
    , m_previousMode(ModeView)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    connect(ui->btnEdit, &QPushButton::clicked, this, &TicketDialog::onEditClicked);
    connect(ui->btnSave, &QPushButton::clicked, this, &TicketDialog::onSaveClicked);
    connect(ui->btnCancel, &QPushButton::clicked, this, &TicketDialog::onCancelClicked);
    connect(ui->btnClose, &QPushButton::clicked, this, &QDialog::hide);
}

TicketDialog::~TicketDialog()
{
    delete ui;
}

void TicketDialog::setMode(Mode mode)
{
    m_previousMode = m_mode;
    m_mode = mode;
    applyMode();
}

void TicketDialog::applyMode()
{
    switch (m_mode) {
    case ModeView:
        setFieldsReadOnly(true);
        ui->btnEdit->setVisible(true);
        ui->btnClose->setVisible(true);
        ui->btnSave->setVisible(false);
        ui->btnCancel->setVisible(false);
        ui->groupInfo->setVisible(true);
        break;

    case ModeEdit:
        setFieldsReadOnly(false);
        ui->btnEdit->setVisible(false);
        ui->btnClose->setVisible(false);
        ui->btnSave->setVisible(true);
        ui->btnCancel->setVisible(true);
        ui->groupInfo->setVisible(true);
        break;

    case ModeNew:
        setFieldsReadOnly(false);
        ui->lineTitle->clear();
        ui->plainDescription->clear();
        ui->comboPriority->setCurrentIndex(0);
        ui->comboStatus->setCurrentIndex(0);
        ui->labelIdValue->setText(tr("—"));
        ui->labelCreatedValue->setText(tr("—"));
        ui->btnEdit->setVisible(false);
        ui->btnClose->setVisible(false);
        ui->btnSave->setVisible(true);
        ui->btnCancel->setVisible(true);
        ui->groupInfo->setVisible(false);
        break;
    }
}

void TicketDialog::setFieldsReadOnly(bool readOnly)
{
    ui->lineTitle->setReadOnly(readOnly);
    ui->plainDescription->setReadOnly(readOnly);
    ui->comboPriority->setEnabled(!readOnly);
    ui->comboStatus->setEnabled(!readOnly);
}

void TicketDialog::onEditClicked()
{
    setMode(ModeEdit);
    setWindowTitle(tr("Edit Ticket"));
}

void TicketDialog::onSaveClicked()
{
    hide();
}

void TicketDialog::onCancelClicked()
{
    if (m_previousMode == ModeView) {
        setMode(ModeView);
        setWindowTitle(tr("View Ticket"));
    } else {
        hide();
    }
}
