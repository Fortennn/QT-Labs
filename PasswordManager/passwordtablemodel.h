#pragma once

#include <QAbstractTableModel>
#include <QList>
#include "passwordentry.h"
#include "passwordrepository.h"

class PasswordTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Column {
        ColId = 0,
        ColTitle,
        ColUsername,
        ColPassword,
        ColWebsite,
        ColCategory,
        ColUpdatedAt,
        ColCount
    };

    explicit PasswordTableModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    void setEntries(const QList<PasswordEntry> &entries);
    void addEntry(const PasswordEntry &entry);
    void setRepository(PasswordRepository *repository);
    void updateEntry(int row, const PasswordEntry &entry);
    void removeEntry(int row);
    PasswordEntry entryAt(int row) const;
    int nextId() const;

private:
    QList<PasswordEntry> m_entries;
    int m_nextId = 1;
    PasswordRepository *m_repository = nullptr;
};
