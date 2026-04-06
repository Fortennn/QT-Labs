#include "passwordrepository.h"

#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QDebug>
#include <QDateTime>

PasswordRepository::PasswordRepository(const QSqlDatabase &db)
    : m_db(db)
{
}

QList<PasswordEntry> PasswordRepository::loadAll() const
{
    QList<PasswordEntry> entries;
    QSqlQuery query(m_db);

    if (!query.exec("SELECT id, title, username, password, website, category, updated_at FROM passwords")) {
        qDebug() << "PasswordRepository::loadAll error:" << query.lastError().text();
        return entries;
    }

    while (query.next()) {
        PasswordEntry e;
        e.id        = query.value(0).toInt();
        e.title     = query.value(1).toString();
        e.username  = query.value(2).toString();
        e.password  = query.value(3).toString();
        e.website   = query.value(4).toString();
        e.category  = query.value(5).toString();
        e.updatedAt = QDateTime::fromString(query.value(6).toString(), Qt::ISODate);
        entries.append(e);
    }
    return entries;
}

bool PasswordRepository::insert(PasswordEntry &entry)
{
    QSqlQuery query(m_db);
    query.prepare(
        "INSERT INTO passwords (title, username, password, website, category, updated_at) "
        "VALUES (:title, :username, :password, :website, :category, :updatedAt)"
    );
    entry.updatedAt = QDateTime::currentDateTime();
    query.bindValue(":title",     entry.title);
    query.bindValue(":username",  entry.username);
    query.bindValue(":password",  entry.password);
    query.bindValue(":website",   entry.website);
    query.bindValue(":category",  entry.category);
    query.bindValue(":updatedAt", entry.updatedAt.toString(Qt::ISODate));

    if (!query.exec()) {
        qDebug() << "PasswordRepository::insert error:" << query.lastError().text();
        return false;
    }
    entry.id = query.lastInsertId().toInt();
    return true;
}

bool PasswordRepository::update(const PasswordEntry &entry)
{
    QSqlQuery query(m_db);
    query.prepare(
        "UPDATE passwords SET "
        "title = :title, username = :username, password = :password, "
        "website = :website, category = :category, updated_at = :updatedAt "
        "WHERE id = :id"
    );
    query.bindValue(":title",     entry.title);
    query.bindValue(":username",  entry.username);
    query.bindValue(":password",  entry.password);
    query.bindValue(":website",   entry.website);
    query.bindValue(":category",  entry.category);
    query.bindValue(":updatedAt", entry.updatedAt.toString(Qt::ISODate));
    query.bindValue(":id",        entry.id);

    if (!query.exec()) {
        qDebug() << "PasswordRepository::update error:" << query.lastError().text();
        return false;
    }
    return true;
}

bool PasswordRepository::remove(int id)
{
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM passwords WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        qDebug() << "PasswordRepository::remove error:" << query.lastError().text();
        return false;
    }
    return true;
}
