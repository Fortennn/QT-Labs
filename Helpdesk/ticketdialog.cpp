#include "ticketdialog.h"
#include "ui_ticketdialog.h"

#include <QPushButton>

TicketDialog::TicketDialog(Mode mode, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::TicketDialog)
    , m_mode(mode)
    , m_createdAt(QDateTime::currentDateTime())
{
    ui->setupUi(this);

    ui->comboBoxPriority->addItems(Ticket::priorities());
    ui->comboBoxStatus->addItems(Ticket::statuses());

    if (mode == ModeView) {
        setWindowTitle("View Ticket");
        setupReadOnly(true);

        ui->buttonBox->setStandardButtons(QDialogButtonBox::Close);
        QPushButton *editBtn = ui->buttonBox->addButton("Edit", QDialogButtonBox::ActionRole);
        connect(editBtn, &QPushButton::clicked, this, &TicketDialog::onEditClicked);
    } else if (mode == ModeAdd) {
        setWindowTitle("New Ticket");
        setupReadOnly(false);
        ui->labelIdValue->setText("(auto)");
        ui->labelCreatedValue->setText(m_createdAt.toString("yyyy-MM-dd HH:mm"));
    } else {
        setWindowTitle("Edit Ticket");
        setupReadOnly(false);
    }

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

TicketDialog::~TicketDialog()
{
    delete ui;
}

void TicketDialog::setTicket(const Ticket &ticket)
{
    m_id        = ticket.id;
    m_createdAt = ticket.createdAt;

    ui->labelIdValue->setText(QString::number(ticket.id));
    ui->lineEditTitle->setText(ticket.title);
    ui->comboBoxPriority->setCurrentText(ticket.priority);
    ui->comboBoxStatus->setCurrentText(ticket.status);
    ui->labelCreatedValue->setText(ticket.createdAt.toString("yyyy-MM-dd HH:mm"));
    ui->plainTextEditDescription->setPlainText(ticket.description);
}

Ticket TicketDialog::ticket() const
{
    Ticket t;
    t.id          = m_id;
    t.title       = ui->lineEditTitle->text().trimmed();
    t.priority    = ui->comboBoxPriority->currentText();
    t.status      = ui->comboBoxStatus->currentText();
    t.createdAt   = m_createdAt;
    t.description = ui->plainTextEditDescription->toPlainText().trimmed();
    return t;
}

void TicketDialog::onEditClicked()
{
    setupReadOnly(false);
    setWindowTitle("Edit Ticket");
    m_mode = ModeEdit;

    ui->buttonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void TicketDialog::setupReadOnly(bool readOnly)
{
    ui->lineEditTitle->setReadOnly(readOnly);
    ui->comboBoxPriority->setEnabled(!readOnly);
    ui->comboBoxStatus->setEnabled(!readOnly);
    ui->plainTextEditDescription->setReadOnly(readOnly);
}
