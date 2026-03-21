#pragma once

#include <QAbstractTableModel>
#include <QList>
#include <QColor>
#include <QFont>
#include "ticket.h"

class TicketTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Column {
        ColId = 0,
        ColTitle,
        ColPriority,
        ColStatus,
        ColCreatedAt,
        ColCount
    };

    explicit TicketTableModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void appendTicket(const Ticket &ticket);
    void replaceTicket(int row, const Ticket &ticket);
    void removeTicket(int row);
    Ticket ticketAt(int row) const;
    QList<Ticket> allTickets() const;

private:
    QList<Ticket> m_items;
    int m_nextId = 1;
};
