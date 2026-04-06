#include "databasemanager.h"

#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QDebug>

bool DatabaseManager::open(const QString &filePath)
{
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(filePath);

    if (!m_db.open()) {
        qDebug() << "DatabaseManager: failed to open:" << m_db.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::initializeSchema()
{
    QSqlQuery query(m_db);
    bool ok = query.exec(
        "CREATE TABLE IF NOT EXISTS passwords ("
        "id          INTEGER PRIMARY KEY AUTOINCREMENT,"
        "title       TEXT    NOT NULL,"
        "username    TEXT,"
        "password    TEXT,"
        "website     TEXT,"
        "category    TEXT,"
        "updated_at  TEXT"
        ")"
    );
    if (!ok)
        qDebug() << "DatabaseManager: initializeSchema error:" << query.lastError().text();
    return ok;
}

QSqlDatabase DatabaseManager::database() const
{
    return m_db;
}
