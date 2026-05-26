#ifndef API_CHAT_WORKER_H
#define API_CHAT_WORKER_H

#include <QList>
#include <QObject>
#include <QString>

#include "LlamaWorkerThread.h"

QT_BEGIN_NAMESPACE
class QNetworkAccessManager;
class QNetworkReply;
QT_END_NAMESPACE

// Remote-API "model" backend that talks to an OpenAI-compatible HTTP server
// (works with OpenAI, OpenRouter, Groq, Together, LM Studio, Ollama (via
// /v1), llama-server, etc.). Mirrors the public surface of LlamaWorkerThread
// (same signal names + signatures) so MainWindow can dispatch to either
// backend without caring which is live.
//
// Streaming uses Server-Sent Events (`data: { ... }` chunks). If the server
// doesn't stream, we fall back to a non-streaming POST and emit the full
// reply as a single `replyFinished()`.
class ApiChatWorker : public QObject {
    Q_OBJECT

public:
    struct Config {
        // Full base URL ending with `/v1` (no trailing slash). For example:
        //   https://api.openai.com/v1
        //   http://localhost:11434/v1     (Ollama)
        //   http://localhost:1234/v1      (LM Studio)
        //   https://openrouter.ai/api/v1
        QString endpoint = QStringLiteral("https://api.openai.com/v1");
        QString apiKey;          // Authorization: Bearer ...
        QString model = QStringLiteral("gpt-4o-mini");
        bool    streaming = true;
        int     timeoutSec = 120;
    };

    explicit ApiChatWorker(QObject* parent = nullptr);
    ~ApiChatWorker() override;

    void setConfig(const Config& cfg);
    Config config() const { return m_cfg; }

    // Mirror of LlamaWorkerThread::setGenParams — only temperature, topP,
    // maxTokens are forwarded to the remote API. Everything else is ignored.
    void setGenParams(const LlamaWorkerThread::GenParams& params);

    void setSystemPromptOverride(const QString& override);
    QString systemPromptOverride() const { return m_systemOverride; }

    // Same names as LlamaWorkerThread::queueLoadModel / queuePrompt so
    // MainWindow can dispatch to either backend.
    void queueLoadModel(const QString& modelName);
    void queuePrompt(const QString& systemPrompt, const QString& userPrompt);
    void stopGeneration();
    void clearHistory();

    bool    isBusy() const { return m_busy; }
    QString loadedModelName() const { return m_cfg.model; }

signals:
    void modelLoaded(bool success);
    void tokenGenerated(const QString& token);
    void replyFinished(const QString& fullResponse);
    void errorOccurred(const QString& errorMsg);

private slots:
    void onReadyRead();
    void onFinished();

private:
    void sendRequest(const QString& systemPrompt, const QString& userPrompt);
    void resetReply();
    QByteArray buildPayload(const QString& systemPrompt,
                            const QString& userPrompt) const;

    QNetworkAccessManager* m_net   = nullptr;
    QNetworkReply*         m_reply = nullptr;

    Config  m_cfg;
    LlamaWorkerThread::GenParams m_gen;
    QString m_systemOverride;
    QByteArray m_sseBuffer;            // SSE assembly buffer
    QString    m_currentAccum;         // accumulated response text
    bool       m_stopRequested = false;
    bool       m_busy = false;

    struct ChatTurn {
        QString user;
        QString assistant;
    };
    QList<ChatTurn> m_history;
    QString         m_pendingUser;     // user prompt currently in flight
};

#endif // API_CHAT_WORKER_H
