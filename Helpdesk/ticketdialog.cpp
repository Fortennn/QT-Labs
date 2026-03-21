#include "ticketdialog.h"
#include "ui_ticketdialog.h"

TicketDialog::TicketDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::TicketDialog)
{
    ui->setupUi(this);

    ui->comboPriority->addItems({"Low", "Medium", "High", "Critical"});
    ui->comboStatus->addItems({"Open", "In Progress", "Resolved", "Closed"});

    connect(ui->editTitle, &QLineEdit::textChanged, this, &TicketDialog::onFormChanged);
    connect(ui->editDescription, &QPlainTextEdit::textChanged, this, &TicketDialog::onFormChanged);
    connect(ui->comboPriority, &QComboBox::currentTextChanged, this, &TicketDialog::onFormChanged);
    connect(ui->comboStatus, &QComboBox::currentTextChanged, this, &TicketDialog::onFormChanged);

    connect(ui->btnSave, &QPushButton::clicked, this, &TicketDialog::onSaveClicked);
    connect(ui->btnEdit, &QPushButton::clicked, this, &TicketDialog::onEditClicked);
    connect(ui->btnCancel, &QPushButton::clicked, this, &TicketDialog::onCancelClicked);
    connect(ui->btnClose, &QPushButton::clicked, this, &QDialog::accept);
}

TicketDialog::~TicketDialog()
{
    delete ui;
}

void TicketDialog::setMode(Mode mode)
{
    m_mode = mode;
    updateUiForMode();
    updateValidationUi();
    updateButtonsState();
}

TicketDialog::Mode TicketDialog::mode() const
{
    return m_mode;
}

void TicketDialog::loadTicket(const Ticket &ticket)
{
    m_originalTicket = ticket;
    ui->labelIdValue->setText(ticket.id > 0 ? QString::number(ticket.id) : "-");
    ui->editTitle->setText(ticket.title);
    ui->editDescription->setPlainText(ticket.description);
    ui->comboPriority->setCurrentText(ticket.priority);
    ui->comboStatus->setCurrentText(ticket.status);
    ui->labelCreatedValue->setText(
        ticket.createdAt.isValid() ? ticket.createdAt.toString("yyyy-MM-dd HH:mm") : "-");
}

Ticket TicketDialog::collectTicket() const
{
    Ticket t;
    t.id = m_originalTicket.id;
    t.createdAt = m_originalTicket.createdAt;
    t.title = ui->editTitle->text().trimmed();
    t.description = ui->editDescription->toPlainText().trimmed();
    t.priority = ui->comboPriority->currentText();
    t.status = ui->comboStatus->currentText();
    return t;
}

bool TicketDialog::isTitleValid() const
{
    return !ui->editTitle->text().trimmed().isEmpty();
}

bool TicketDialog::isFormValid() const
{
    return isTitleValid();
}

void TicketDialog::updateUiForMode()
{
    const bool editable = (m_mode == Mode::Edit || m_mode == Mode::Create);

    ui->editTitle->setReadOnly(!editable);
    ui->editDescription->setReadOnly(!editable);
    ui->comboPriority->setEnabled(editable);
    ui->comboStatus->setEnabled(editable);

    ui->btnSave->setVisible(editable);
    ui->btnCancel->setVisible(editable);
    ui->btnEdit->setVisible(m_mode == Mode::View);
    ui->btnClose->setVisible(m_mode == Mode::View);

    switch (m_mode) {
    case Mode::Create: setWindowTitle("New Ticket"); break;
    case Mode::Edit:   setWindowTitle("Edit Ticket"); break;
    case Mode::View:   setWindowTitle("View Ticket"); break;
    }
}

void TicketDialog::updateValidationUi()
{
    if (isTitleValid())
        ui->labelTitleError->clear();
    else
        ui->labelTitleError->setText("Title is required.");
}

void TicketDialog::updateButtonsState()
{
    const bool editable = (m_mode == Mode::Edit || m_mode == Mode::Create);
    ui->btnSave->setEnabled(editable && isFormValid());
}

void TicketDialog::onFormChanged()
{
    updateValidationUi();
    updateButtonsState();
}

void TicketDialog::onSaveClicked()
{
    if (!isFormValid())
        return;

    Ticket t = collectTicket();
    if (m_mode == Mode::Create)
        emit createRequested(t);
    else if (m_mode == Mode::Edit)
        emit updateRequested(t);

    accept();
}

void TicketDialog::onEditClicked()
{
    setMode(Mode::Edit);
}

void TicketDialog::onCancelClicked()
{
    if (m_mode == Mode::Edit) {
        loadTicket(m_originalTicket);
        setMode(Mode::View);
    } else {
        reject();
    }
}
