#include "ApiChatWorker.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>

ApiChatWorker::ApiChatWorker(QObject* parent)
    : QObject(parent),
      m_net(new QNetworkAccessManager(this))
{}

ApiChatWorker::~ApiChatWorker() {
    if (m_reply) {
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }
}

void ApiChatWorker::setConfig(const Config& cfg) {
    m_cfg = cfg;
    if (m_cfg.endpoint.endsWith(QLatin1Char('/')))
        m_cfg.endpoint.chop(1);
    if (m_cfg.timeoutSec < 10) m_cfg.timeoutSec = 10;
}

void ApiChatWorker::setGenParams(const LlamaWorkerThread::GenParams& params) {
    m_gen = params;
}

void ApiChatWorker::setSystemPromptOverride(const QString& override) {
    m_systemOverride = override;
}

void ApiChatWorker::queueLoadModel(const QString& modelName) {
    // For remote API there is no actual load step. We just update the model
    // name and immediately report success so the rest of the UI can light up.
    if (!modelName.isEmpty()) m_cfg.model = modelName;
    qInfo() << "[JARVIS API] using remote model" << m_cfg.model
            << "endpoint" << m_cfg.endpoint;
    // Defer the signal so consumers connect first.
    QTimer::singleShot(0, this, [this]() { emit modelLoaded(true); });
}

void ApiChatWorker::queuePrompt(const QString& systemPrompt,
                                const QString& userPrompt)
{
    if (m_busy) {
        emit errorOccurred(QStringLiteral(
            "API: попередній запит ще не завершений."));
        return;
    }
    if (m_cfg.endpoint.isEmpty()) {
        emit errorOccurred(QStringLiteral(
            "API: не вказано endpoint. Відкрий «Налаштування» → «Бекенд»."));
        return;
    }
    if (m_cfg.model.isEmpty()) {
        emit errorOccurred(QStringLiteral(
            "API: не вказано model. Перевір «Налаштування» → «Бекенд»."));
        return;
    }
    m_pendingUser = userPrompt;
    m_stopRequested = false;
    m_busy = true;
    sendRequest(systemPrompt, userPrompt);
}

void ApiChatWorker::stopGeneration() {
    m_stopRequested = true;
    if (m_reply) m_reply->abort();
}

void ApiChatWorker::clearHistory() {
    m_history.clear();
}

void ApiChatWorker::resetReply() {
    if (m_reply) {
        m_reply->deleteLater();
        m_reply = nullptr;
    }
    m_sseBuffer.clear();
    m_currentAccum.clear();
}

QByteArray ApiChatWorker::buildPayload(const QString& systemPrompt,
                                       const QString& userPrompt) const
{
    QJsonArray messages;
    QString systemMsg = m_systemOverride.isEmpty() ? systemPrompt
                                                   : m_systemOverride;
    if (!systemMsg.isEmpty()) {
        QJsonObject sys;
        sys.insert(QStringLiteral("role"),    QStringLiteral("system"));
        sys.insert(QStringLiteral("content"), systemMsg);
        messages.append(sys);
    }
    // Останні 10 ходів історії — ідентично LlamaWorkerThread.
    const int historyStart = qMax(0, m_history.size() - 10);
    for (int i = historyStart; i < m_history.size(); ++i) {
        QJsonObject u;
        u.insert(QStringLiteral("role"),    QStringLiteral("user"));
        u.insert(QStringLiteral("content"), m_history[i].user);
        messages.append(u);

        QJsonObject a;
        a.insert(QStringLiteral("role"),    QStringLiteral("assistant"));
        a.insert(QStringLiteral("content"), m_history[i].assistant);
        messages.append(a);
    }
    QJsonObject user;
    user.insert(QStringLiteral("role"),    QStringLiteral("user"));
    user.insert(QStringLiteral("content"), userPrompt);
    messages.append(user);

    QJsonObject root;
    root.insert(QStringLiteral("model"),       m_cfg.model);
    root.insert(QStringLiteral("messages"),    messages);
    root.insert(QStringLiteral("temperature"), m_gen.temperature);
    root.insert(QStringLiteral("top_p"),       m_gen.topP);
    root.insert(QStringLiteral("max_tokens"),  m_gen.maxTokens);
    if (m_cfg.streaming) {
        root.insert(QStringLiteral("stream"), true);
    }
    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

void ApiChatWorker::sendRequest(const QString& systemPrompt,
                                const QString& userPrompt)
{
    resetReply();

    const QString url = m_cfg.endpoint + QStringLiteral("/chat/completions");
    QNetworkRequest req{ QUrl(url) };
    req.setHeader(QNetworkRequest::ContentTypeHeader,
                  QStringLiteral("application/json"));
    if (!m_cfg.apiKey.isEmpty()) {
        req.setRawHeader("Authorization",
                         ("Bearer " + m_cfg.apiKey).toUtf8());
    }
    // Common convention for SSE.
    if (m_cfg.streaming) {
        req.setRawHeader("Accept", "text/event-stream");
    }
    // Disable Qt's automatic decompression so we see raw SSE bytes.
    req.setAttribute(QNetworkRequest::Http2AllowedAttribute, true);

    const QByteArray body = buildPayload(systemPrompt, userPrompt);
    qInfo() << "[JARVIS API] POST" << url
            << " bytes=" << body.size()
            << " stream=" << m_cfg.streaming;

    m_reply = m_net->post(req, body);
    connect(m_reply, &QNetworkReply::readyRead,
            this, &ApiChatWorker::onReadyRead);
    connect(m_reply, &QNetworkReply::finished,
            this, &ApiChatWorker::onFinished);

    // Hard timeout — Qt's default is per-operation but doesn't cover full
    // streaming requests. Wrap with a singleShot so a stuck connection
    // can't park forever.
    QTimer::singleShot(m_cfg.timeoutSec * 1000, m_reply, [this]() {
        if (m_reply) {
            qWarning() << "[JARVIS API] timeout — aborting request";
            m_reply->abort();
        }
    });
}

void ApiChatWorker::onReadyRead() {
    if (!m_reply) return;
    const QByteArray chunk = m_reply->readAll();
    if (chunk.isEmpty()) return;

    if (!m_cfg.streaming) {
        // Non-streaming: buffer until finished().
        m_sseBuffer.append(chunk);
        return;
    }

    m_sseBuffer.append(chunk);

    // SSE frames are separated by "\n\n" (or "\r\n\r\n"). Parse all
    // complete frames currently in the buffer.
    while (true) {
        int sep = m_sseBuffer.indexOf("\n\n");
        if (sep < 0) sep = m_sseBuffer.indexOf("\r\n\r\n");
        if (sep < 0) break;
        const QByteArray frame = m_sseBuffer.left(sep);
        m_sseBuffer.remove(0, sep + 2);
        // Trim a potential leading \r\n from a previous frame's CRLFCRLF.
        if (m_sseBuffer.startsWith("\r\n")) m_sseBuffer.remove(0, 2);

        // Each frame is a series of `field: value` lines. We only care about
        // `data:` payloads.
        const QList<QByteArray> lines = frame.split('\n');
        for (const QByteArray& raw : lines) {
            QByteArray line = raw;
            if (line.endsWith('\r')) line.chop(1);
            if (!line.startsWith("data:")) continue;
            QByteArray payload = line.mid(5).trimmed();
            if (payload == "[DONE]") {
                // The server-side end-of-stream marker; nothing to emit.
                continue;
            }
            const auto doc = QJsonDocument::fromJson(payload);
            if (!doc.isObject()) continue;
            const QJsonObject root = doc.object();
            const QJsonArray choices = root.value(QStringLiteral("choices"))
                                           .toArray();
            for (const QJsonValue& v : choices) {
                const QJsonObject ch = v.toObject();
                const QJsonObject delta = ch.value(QStringLiteral("delta"))
                                            .toObject();
                const QString piece =
                    delta.value(QStringLiteral("content")).toString();
                if (piece.isEmpty()) continue;
                m_currentAccum += piece;
                if (!m_stopRequested) emit tokenGenerated(piece);
            }
        }
    }
}

void ApiChatWorker::onFinished() {
    if (!m_reply) {
        m_busy = false;
        return;
    }

    const int status = m_reply->attribute(
        QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QNetworkReply::NetworkError err = m_reply->error();

    if (err == QNetworkReply::OperationCanceledError && m_stopRequested) {
        // User-initiated abort. Emit whatever we have.
        const QString full = m_currentAccum;
        m_history.append({m_pendingUser, full.trimmed()});
        resetReply();
        m_busy = false;
        emit replyFinished(full);
        return;
    }

    if (err != QNetworkReply::NoError && status < 200) {
        const QString msg = QStringLiteral("API запит провалився: %1")
                                .arg(m_reply->errorString());
        qWarning() << "[JARVIS API]" << msg;
        resetReply();
        m_busy = false;
        emit errorOccurred(msg);
        return;
    }

    if (!m_cfg.streaming) {
        // Non-streaming branch: parse the buffered payload as a single object.
        const auto doc = QJsonDocument::fromJson(m_sseBuffer);
        if (status >= 400 || !doc.isObject()) {
            QString msg;
            if (doc.isObject()) {
                const auto err2 = doc.object().value(QStringLiteral("error"));
                msg = err2.isObject()
                          ? err2.toObject().value(QStringLiteral("message"))
                                .toString()
                          : QString::fromUtf8(m_sseBuffer);
            } else {
                msg = QString::fromUtf8(m_sseBuffer);
            }
            resetReply();
            m_busy = false;
            emit errorOccurred(QStringLiteral("API %1: %2")
                                   .arg(status).arg(msg));
            return;
        }
        const QJsonObject root = doc.object();
        const QJsonArray choices = root.value(QStringLiteral("choices"))
                                       .toArray();
        QString full;
        for (const QJsonValue& v : choices) {
            const QJsonObject msg = v.toObject().value(QStringLiteral("message"))
                                                .toObject();
            full += msg.value(QStringLiteral("content")).toString();
        }
        // Емітимо весь текст одним пакетом, як ніби «псевдо-стрім».
        if (!full.isEmpty()) emit tokenGenerated(full);
        m_history.append({m_pendingUser, full.trimmed()});
        resetReply();
        m_busy = false;
        emit replyFinished(full);
        return;
    }

    // Streaming branch: also handle HTTP error responses that came as a
    // single JSON object instead of an SSE stream (common with 401/429/500).
    if (status >= 400) {
        const auto doc = QJsonDocument::fromJson(m_sseBuffer);
        QString msg;
        if (doc.isObject()) {
            const auto err2 = doc.object().value(QStringLiteral("error"));
            if (err2.isObject()) {
                msg = err2.toObject().value(QStringLiteral("message"))
                          .toString();
            } else {
                msg = QString::fromUtf8(m_sseBuffer);
            }
        } else {
            msg = QString::fromUtf8(m_sseBuffer);
        }
        resetReply();
        m_busy = false;
        emit errorOccurred(QStringLiteral("API %1: %2").arg(status).arg(msg));
        return;
    }

    const QString full = m_currentAccum;
    m_history.append({m_pendingUser, full.trimmed()});
    resetReply();
    m_busy = false;
    emit replyFinished(full);
}
