#pragma once

#include <QList>
#include <QtSql/QSqlDatabase>
#include "passwordentry.h"

class PasswordRepository
{
public:
    explicit PasswordRepository(const QSqlDatabase &db);

    QList<PasswordEntry> loadAll() const;
    bool insert(PasswordEntry &entry);
    bool update(const PasswordEntry &entry);
    bool remove(int id);

private:
    QSqlDatabase m_db;
};
