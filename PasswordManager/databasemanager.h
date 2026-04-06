#pragma once

#include <QString>
#include <QtSql/QSqlDatabase>

class DatabaseManager
{
public:
    bool open(const QString &filePath);
    bool initializeSchema();
    QSqlDatabase database() const;

private:
    QSqlDatabase m_db;
};
