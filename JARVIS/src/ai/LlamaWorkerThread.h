#ifndef LLAMA_WORKER_THREAD_H
#define LLAMA_WORKER_THREAD_H

#include <QThread>
#include <QString>
#include <QMutex>
#include <QWaitCondition>
#include <QQueue>
#include <QList>

struct llama_model;
struct llama_context;

class LlamaWorkerThread : public QThread {
    Q_OBJECT

public:
    explicit LlamaWorkerThread(QObject *parent = nullptr);
    ~LlamaWorkerThread() override;

    bool loadModel(const QString& modelPath);
    void queueLoadModel(const QString& modelPath);
    void queuePrompt(const QString& systemPrompt, const QString& userPrompt);
    void stopGeneration();
    void clearHistory();

signals:
    void modelLoaded(bool success);
    void tokenGenerated(const QString& token);
    void replyFinished(const QString& fullResponse);
    void errorOccurred(const QString& errorMsg);

protected:
    void run() override;

private:
    llama_model* m_model = nullptr;
    llama_context* m_ctx = nullptr;

    QMutex m_mutex;
    QWaitCondition m_cond;
    
    struct ChatTurn {
        QString user;
        QString assistant;
    };
    QList<ChatTurn> m_history;

    struct Request {
        QString systemPrompt;
        QString userPrompt;
    };
    QQueue<Request> m_requests;
    QString m_modelPathToLoad;

    bool m_stopRequested = false;
    bool m_quitThread = false;
    
    void processGeneration(const Request& req);
};

#endif // LLAMA_WORKER_THREAD_H
