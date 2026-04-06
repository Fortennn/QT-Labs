#include "passwordtablemodel.h"

PasswordTableModel::PasswordTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

void PasswordTableModel::setRepository(PasswordRepository *repository)
{
    m_repository = repository;
}

int PasswordTableModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_entries.size();
}

int PasswordTableModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return ColCount;
}

QVariant PasswordTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_entries.size())
        return {};

    const PasswordEntry &entry = m_entries[index.row()];

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (index.column()) {
        case ColId:       return entry.id;
        case ColTitle:    return entry.title;
        case ColUsername: return entry.username;
        case ColPassword:
            if (role == Qt::DisplayRole) return QString("••••••••");
            return entry.password;
        case ColWebsite:  return entry.website;
        case ColCategory: return entry.category;
        case ColUpdatedAt:
            if (entry.updatedAt.isValid())
                return entry.updatedAt.toString("yyyy-MM-dd HH:mm");
            return QString();
        }
    }

    if (role == Qt::TextAlignmentRole) {
        if (index.column() == ColId)
            return int(Qt::AlignCenter);
    }

    return {};
}

QVariant PasswordTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};

    switch (section) {
    case ColId:        return "ID";
    case ColTitle:     return "Title";
    case ColUsername:  return "Username";
    case ColPassword:  return "Password";
    case ColWebsite:   return "Website";
    case ColCategory:  return "Category";
    case ColUpdatedAt: return "Updated At";
    }
    return {};
}

Qt::ItemFlags PasswordTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    if (index.column() == ColId || index.column() == ColUpdatedAt)
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled;

    return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable;
}

bool PasswordTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    PasswordEntry &entry = m_entries[index.row()];

    switch (index.column()) {
    case ColTitle:    entry.title    = value.toString(); break;
    case ColUsername: entry.username = value.toString(); break;
    case ColPassword: entry.password = value.toString(); break;
    case ColWebsite:  entry.website  = value.toString(); break;
    case ColCategory: entry.category = value.toString(); break;
    default: return false;
    }

    entry.updatedAt = QDateTime::currentDateTime();

    if (m_repository) {
        if (!m_repository->update(entry)) {
            return false;
        }
    }

    emit dataChanged(index, index, {role});
    emit dataChanged(
        this->index(index.row(), ColUpdatedAt),
        this->index(index.row(), ColUpdatedAt),
        {Qt::DisplayRole}
    );

    return true;
}

void PasswordTableModel::setEntries(const QList<PasswordEntry> &entries)
{
    beginResetModel();
    m_entries = entries;
    endResetModel();
}

void PasswordTableModel::updateEntry(int row, const PasswordEntry &entry)
{
    if (row < 0 || row >= m_entries.size())
        return;
    m_entries[row] = entry;
    emit dataChanged(index(row, 0), index(row, ColCount - 1));
}

void PasswordTableModel::addEntry(const PasswordEntry &entry)
{
    beginInsertRows(QModelIndex(), m_entries.size(), m_entries.size());
    m_entries.append(entry);
    endInsertRows();
}

void PasswordTableModel::removeEntry(int row)
{
    if (row < 0 || row >= m_entries.size())
        return;
    beginRemoveRows(QModelIndex(), row, row);
    m_entries.removeAt(row);
    endRemoveRows();
}

PasswordEntry PasswordTableModel::entryAt(int row) const
{
    return m_entries.value(row);
}

int PasswordTableModel::nextId() const
{
    return m_nextId;
}
