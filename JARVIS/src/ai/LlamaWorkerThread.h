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

    // Generation / context parameters. Apply on the next model (re)load
    // and the next generation cycle.
    void setParams(float temperature, int contextSize);

signals:
    void modelLoaded(bool success);
    // Only clean, user-visible text is emitted here. Anything inside
    // square brackets (e.g. [CMD: ...] / [PS: ...]) is filtered out.
    void tokenGenerated(const QString& token);
    // The full, *unfiltered* assistant response — including bracketed
    // tags — so the UI layer can parse and execute system commands.
    void replyFinished(const QString& fullResponse);
    void errorOccurred(const QString& errorMsg);

protected:
    void run() override;

private:
    llama_model*   m_model = nullptr;
    llama_context* m_ctx   = nullptr;

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
    bool m_quitThread    = false;

    // Tunables (guarded by m_mutex)
    float m_temperature = 0.8f;
    int   m_contextSize = 2048;

    void releaseModel();
    void processGeneration(const Request& req);
};

#endif // LLAMA_WORKER_THREAD_H
