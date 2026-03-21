#pragma once

#include <QString>
#include <QDateTime>

struct Ticket
{
    int id = 0;
    QString title;
    QString description;
    QString status;
    QString priority;
    QDateTime createdAt;
};
