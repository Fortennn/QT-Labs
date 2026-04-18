#include "passwordbatchworker.h"
#include "passwordleakchecker.h"
#include <QMetaObject>

PasswordBatchWorker::PasswordBatchWorker(const QList<PasswordEntry> &entries, QObject *parent)
    : QObject(parent), m_entries(entries), m_currentIndex(0), m_canceled(false), m_checker(nullptr)
{
}

PasswordBatchWorker::~PasswordBatchWorker()
{
    if (m_checker) {
        m_checker->deleteLater();
    }
}

void PasswordBatchWorker::processAll()
{
    m_checker = new PasswordLeakChecker(this);
    connect(m_checker, &PasswordLeakChecker::checkCompleted, this, &PasswordBatchWorker::onCheckCompleted);
    connect(m_checker, &PasswordLeakChecker::checkError, this, &PasswordBatchWorker::onCheckError);

    m_currentIndex = 0;
    m_canceled = false;
    checkNext();
}

void PasswordBatchWorker::cancel()
{
    m_canceled = true;
}

void PasswordBatchWorker::checkNext()
{
    if (m_canceled || m_currentIndex >= m_entries.size()) {
        emit progressChanged(m_currentIndex, m_entries.size());
        emit finished();
        return;
    }

    emit progressChanged(m_currentIndex, m_entries.size());

    const PasswordEntry &entry = m_entries.at(m_currentIndex);
    if (entry.password.isEmpty()) {
        emit entryChecked(entry.id, false, 0, "Empty password");
        m_currentIndex++;
        QMetaObject::invokeMethod(this, "checkNext", Qt::QueuedConnection);
    } else {
        m_checker->checkPassword(entry.password);
    }
}

void PasswordBatchWorker::onCheckCompleted(bool isLeaked, int count)
{
    int id = m_entries.at(m_currentIndex).id;
    emit entryChecked(id, isLeaked, count, "");

    m_currentIndex++;
    QMetaObject::invokeMethod(this, "checkNext", Qt::QueuedConnection);
}

void PasswordBatchWorker::onCheckError(const QString &error)
{
    int id = m_entries.at(m_currentIndex).id;
    emit entryChecked(id, false, 0, error);

    m_currentIndex++;
    QMetaObject::invokeMethod(this, "checkNext", Qt::QueuedConnection);
}
