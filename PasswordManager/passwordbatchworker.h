#pragma once

#include <QObject>
#include <QList>
#include "passwordentry.h"

class PasswordLeakChecker;

class PasswordBatchWorker : public QObject
{
    Q_OBJECT
public:
    explicit PasswordBatchWorker(const QList<PasswordEntry> &entries, QObject *parent = nullptr);
    ~PasswordBatchWorker() override;

public slots:
    void processAll();
    void cancel();

signals:
    void progressChanged(int current, int total);
    void entryChecked(int id, bool isLeaked, int count, const QString &error);
    void finished();

private slots:
    void onCheckCompleted(bool isLeaked, int count);
    void onCheckError(const QString &error);
    void checkNext();

private:
    QList<PasswordEntry> m_entries;
    int m_currentIndex;
    bool m_canceled;
    PasswordLeakChecker *m_checker;
};
