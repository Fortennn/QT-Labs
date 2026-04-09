#include "passwordleakchecker.h"
#include <QNetworkRequest>
#include <QUrl>
#include <QCryptographicHash>
#include <QTimer>
#include <QDebug>

PasswordLeakChecker::PasswordLeakChecker(QObject *parent)
    : QObject(parent)
{
}

void PasswordLeakChecker::checkPassword(const QString &password)
{
    if (password.isEmpty()) {
        emit checkCompleted(false, 0);
        return;
    }

    QByteArray sha1 = QCryptographicHash::hash(
        password.toUtf8(),
        QCryptographicHash::Sha1
    ).toHex().toUpper();

    QString prefix = sha1.left(5);
    QString suffix = sha1.mid(5);

    QNetworkRequest request(QUrl("https://api.pwnedpasswords.com/range/" + prefix));
    request.setRawHeader("Add-Padding", "true");
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    request.setTransferTimeout(10000); // 10 seconds
#endif

    QNetworkReply *reply = m_network.get(request);
    
    // Fallback timer for Qt 5
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QTimer *timer = new QTimer(reply);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, reply, [reply]() {
        if (!reply->isFinished()) {
            reply->abort();
        }
    });
    timer->start(10000);
#endif

    connect(reply, &QNetworkReply::finished, this, [this, reply, suffix]() {
        onReplyFinished(reply, suffix);
    });
}

void PasswordLeakChecker::onReplyFinished(QNetworkReply *reply, const QString &hashSuffix)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit checkError(reply->errorString());
        return;
    }

    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (statusCode != 200) {
        emit checkError(QString("Server returned status %1").arg(statusCode));
        return;
    }

    QByteArray response = reply->readAll();
    QList<QByteArray> lines = response.split('\n');

    bool found = false;
    int count = 0;

    for (const QByteArray &line : lines) {
        QByteArray trimmed = line.trimmed();
        if (trimmed.isEmpty()) continue;

        QList<QByteArray> parts = trimmed.split(':');
        if (parts.size() >= 2) {
            if (parts[0] == hashSuffix.toUtf8()) {
                found = true;
                count = parts[1].toInt();
                break;
            }
        }
    }

    emit checkCompleted(found, count);
}
