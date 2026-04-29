#ifndef LLAMA_WORKER_THREAD_H
#define LLAMA_WORKER_THREAD_H

#include <QList>
#include <QMutex>
#include <QQueue>
#include <QString>
#include <QThread>
#include <QWaitCondition>

struct llama_model;
struct llama_context;

class LlamaWorkerThread : public QThread {
    Q_OBJECT

public:
    // Tunable generation parameters. Mirror the llama.cpp sampler chain
    // configured in processGeneration().
    struct GenParams {
        float temperature   = 0.80f;
        float topP          = 0.95f;
        int   topK          = 40;
        float minP          = 0.10f;
        float repeatPenalty = 1.10f;
        int   maxTokens     = 1024;
        int   contextSize   = 2048;
    };

    explicit LlamaWorkerThread(QObject* parent = nullptr);
    ~LlamaWorkerThread() override;

    bool loadModel(const QString& modelPath);

    // Legacy two-knob setter — still used by older call sites.
    // Equivalent to copying the current params, overwriting these two
    // fields, and forwarding to setGenParams().
    void setParams(float temperature, int contextSize);

    // Update *all* generation knobs at once. Picked up on the next
    // processGeneration() invocation; contextSize on the next loadModel().
    void setGenParams(const GenParams& params);
    GenParams genParams() const;

    // Override the system prompt sent to the model. Empty -> use whatever
    // the caller passes to queuePrompt() / Config::SYSTEM_PROMPT.
    void setSystemPromptOverride(const QString& override);
    QString systemPromptOverride() const;

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
    llama_model*   m_model = nullptr;
    llama_context* m_ctx   = nullptr;

    mutable QMutex m_mutex;
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

    GenParams m_gen;                  // protected by m_mutex
    QString   m_systemPromptOverride; // protected by m_mutex

    bool m_stopRequested = false;
    bool m_quitThread    = false;

    void processGeneration(const Request& req);
};

#endif // LLAMA_WORKER_THREAD_H
