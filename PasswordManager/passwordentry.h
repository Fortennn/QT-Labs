#pragma once

#include <QString>
#include <QDateTime>

struct PasswordEntry
{
    int id = 0;
    QString title;
    QString username;
    QString password;
    QString website;
    QString category;
    QDateTime updatedAt;
};
