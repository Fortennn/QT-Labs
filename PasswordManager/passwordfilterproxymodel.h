#pragma once

#include <QSortFilterProxyModel>
#include <QString>

class PasswordFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit PasswordFilterProxyModel(QObject *parent = nullptr);

    void setSearchText(const QString &searchText);
    void setCategoryFilter(const QString &category);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    QString m_searchText;
    QString m_category;
};
