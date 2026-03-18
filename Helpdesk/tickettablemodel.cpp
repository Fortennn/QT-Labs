#include "tickettablemodel.h"
#include <QColor>

TicketTableModel::TicketTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{}

int TicketTableModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_tickets.size();
}

int TicketTableModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : ColCount;
}

QVariant TicketTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    const int r = index.row();
    const int c = index.column();

    if (r < 0 || r >= m_tickets.size())
        return {};

    const Ticket &t = m_tickets.at(r);

    if (role == Qt::DisplayRole) {
        switch (c) {
        case ColId:        return t.id;
        case ColTitle:     return t.title;
        case ColPriority:  return t.priority;
        case ColStatus:    return t.status;
        case ColCreatedAt: return t.createdAt.toString("yyyy-MM-dd HH:mm");
        default:           return {};
        }
    }

    if (role == Qt::ForegroundRole) {
        if (c == ColPriority) return priorityColor(t.priority, role);
        if (c == ColStatus)   return statusColor(t.status, role);
    }

    if (role == Qt::TextAlignmentRole && c == ColId)
        return int(Qt::AlignRight | Qt::AlignVCenter);

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
    default:           return {};
    }
}

void TicketTableModel::addTicket(const Ticket &ticket)
{
    const int row = m_tickets.size();
    beginInsertRows(QModelIndex(), row, row);
    Ticket t = ticket;
    t.id = m_nextId++;
    m_tickets.push_back(t);
    endInsertRows();
}

void TicketTableModel::updateTicket(int row, const Ticket &ticket)
{
    if (row < 0 || row >= m_tickets.size())
        return;
    m_tickets[row] = ticket;
    const QModelIndex left  = index(row, 0);
    const QModelIndex right = index(row, ColCount - 1);
    emit dataChanged(left, right, {Qt::DisplayRole, Qt::ForegroundRole});
}

void TicketTableModel::removeTicket(int row)
{
    if (row < 0 || row >= m_tickets.size())
        return;
    beginRemoveRows(QModelIndex(), row, row);
    m_tickets.removeAt(row);
    endRemoveRows();
}

Ticket TicketTableModel::ticketAt(int row) const
{
    if (row < 0 || row >= m_tickets.size())
        return {};
    return m_tickets.at(row);
}

int TicketTableModel::nextId() const
{
    return m_nextId;
}

QVariant TicketTableModel::priorityColor(const QString &priority, int /*role*/) const
{
    if (priority == "Critical") return QColor("#c0392b");
    if (priority == "High")     return QColor("#e67e22");
    if (priority == "Medium")   return QColor("#2980b9");
    return QColor("#27ae60");
}

QVariant TicketTableModel::statusColor(const QString &status, int /*role*/) const
{
    if (status == "Open")        return QColor("#c0392b");
    if (status == "In Progress") return QColor("#d35400");
    if (status == "Resolved")    return QColor("#27ae60");
    return QColor("#7f8c8d");
}
