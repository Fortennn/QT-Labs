#include "tickettablemodel.h"
#include <QColor>

TicketTableModel::TicketTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{}

int TicketTableModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_items.size();
}

int TicketTableModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return ColCount;
}

QVariant TicketTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_items.size())
        return {};

    const Ticket &t = m_items.at(index.row());

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case ColId:        return t.id;
        case ColTitle:     return t.title;
        case ColPriority:  return t.priority;
        case ColStatus:    return t.status;
        case ColCreatedAt: return t.createdAt.toString("yyyy-MM-dd HH:mm");
        }
    }

    if (role == Qt::ForegroundRole) {
        if (index.column() == ColPriority) {
            if (t.priority == "Low")      return QColor(34, 139, 34);
            if (t.priority == "Medium")   return QColor(218, 165, 32);
            if (t.priority == "High")     return QColor(220, 100, 0);
            if (t.priority == "Critical") return QColor(200, 0, 0);
        }
        if (index.column() == ColStatus) {
            if (t.status == "Open")        return QColor(0, 120, 215);
            if (t.status == "In Progress") return QColor(180, 120, 0);
            if (t.status == "Resolved")    return QColor(34, 139, 34);
            if (t.status == "Closed")      return QColor(120, 120, 120);
        }
    }

    if (role == Qt::FontRole && index.column() == ColPriority && t.priority == "Critical") {
        QFont font;
        font.setBold(true);
        return font;
    }

    return {};
}

QVariant TicketTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};
    switch (section) {
    case ColId:        return "ID";
    case ColTitle:     return "Title";
    case ColPriority:  return "Priority";
    case ColStatus:    return "Status";
    case ColCreatedAt: return "Created At";
    }
    return {};
}

void TicketTableModel::appendTicket(const Ticket &ticket)
{
    Ticket t = ticket;
    t.id = m_nextId++;
    if (!t.createdAt.isValid())
        t.createdAt = QDateTime::currentDateTime();

    const int newRow = m_items.size();
    beginInsertRows(QModelIndex(), newRow, newRow);
    m_items.append(t);
    endInsertRows();
}

void TicketTableModel::replaceTicket(int row, const Ticket &ticket)
{
    if (row < 0 || row >= m_items.size())
        return;
    Ticket t = ticket;
    t.id = m_items.at(row).id;
    t.createdAt = m_items.at(row).createdAt;
    m_items[row] = t;
    emit dataChanged(index(row, 0), index(row, ColCount - 1));
}

void TicketTableModel::removeTicket(int row)
{
    if (row < 0 || row >= m_items.size())
        return;
    beginRemoveRows(QModelIndex(), row, row);
    m_items.removeAt(row);
    endRemoveRows();
}

Ticket TicketTableModel::ticketAt(int row) const
{
    if (row < 0 || row >= m_items.size())
        return {};
    return m_items.at(row);
}

QList<Ticket> TicketTableModel::allTickets() const
{
    return m_items;
}
