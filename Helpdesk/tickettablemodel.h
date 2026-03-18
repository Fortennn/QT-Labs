#pragma once

#include <QAbstractTableModel>
#include <QVector>
#include "ticket.h"

class TicketTableModel : public QAbstractTableModel {
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

    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void addTicket(const Ticket &ticket);
    void updateTicket(int row, const Ticket &ticket);
    void removeTicket(int row);

    Ticket ticketAt(int row) const;
    int nextId() const;

private:
    QVector<Ticket> m_tickets;
    int             m_nextId = 1;

    QVariant priorityColor(const QString &priority, int role) const;
    QVariant statusColor(const QString &status, int role) const;
};
