#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class PasswordLeakChecker : public QObject
{
    Q_OBJECT
public:
    explicit PasswordLeakChecker(QObject *parent = nullptr);
    void checkPassword(const QString &password);

signals:
    void checkCompleted(bool isLeaked, int count);
    void checkError(const QString &errorMessage);

private slots:
    void onReplyFinished(QNetworkReply *reply, const QString &hashSuffix);

private:
    QNetworkAccessManager m_network;
};
