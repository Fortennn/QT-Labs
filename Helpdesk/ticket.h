#pragma once

#include <QString>
#include <QDateTime>

struct Ticket {
    int       id        = 0;
    QString   title;
    QString   priority;
    QString   status;
    QDateTime createdAt;
    QString   description;

    static QStringList priorities() {
        return { "Low", "Medium", "High", "Critical" };
    }

    static QStringList statuses() {
        return { "Open", "In Progress", "Resolved", "Closed" };
    }
};
