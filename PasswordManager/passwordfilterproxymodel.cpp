#include "passwordfilterproxymodel.h"
#include "passwordtablemodel.h"

PasswordFilterProxyModel::PasswordFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
}

void PasswordFilterProxyModel::setSearchText(const QString &searchText)
{
    m_searchText = searchText;
    invalidateFilter();
}

void PasswordFilterProxyModel::setCategoryFilter(const QString &category)
{
    m_category = category;
    invalidateFilter();
}

bool PasswordFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex titleIndex = sourceModel()->index(sourceRow, PasswordTableModel::ColTitle, sourceParent);
    QModelIndex categoryIndex = sourceModel()->index(sourceRow, PasswordTableModel::ColCategory, sourceParent);

    QString title = sourceModel()->data(titleIndex).toString();
    QString category = sourceModel()->data(categoryIndex).toString();

    bool textMatch = m_searchText.isEmpty() || title.contains(m_searchText, Qt::CaseInsensitive);
    bool categoryMatch = m_category.isEmpty() || m_category == "All" || category == m_category;

    return textMatch && categoryMatch;
}
