#pragma once

#include <QDialog>
#include "ticket.h"

namespace Ui {
class TicketDialog;
}

class TicketDialog : public QDialog {
    Q_OBJECT

public:
    enum Mode { ModeView, ModeAdd, ModeEdit };

    explicit TicketDialog(Mode mode, QWidget *parent = nullptr);
    ~TicketDialog();

    void    setTicket(const Ticket &ticket);
    Ticket  ticket() const;

private slots:
    void onEditClicked();

private:
    void setupReadOnly(bool readOnly);

    Ui::TicketDialog *ui;
    Mode              m_mode;
    int               m_id = 0;
    QDateTime         m_createdAt;
};
