#ifndef JARVIS_HTTP_SERVER_H
#define JARVIS_HTTP_SERVER_H

#include <QByteArray>
#include <QHash>
#include <QObject>
#include <QString>
#include <QStringList>

QT_BEGIN_NAMESPACE
class QTcpServer;
class QTcpSocket;
QT_END_NAMESPACE

// Tiny HTTP/1.1 server that exposes JARVIS over the LAN so the user can
// control their PC from a phone in the same Wi-Fi network. It is owned by
// MainWindow, lives on the main thread, and uses signals to ask MainWindow
// to do the actual work (run commands, queue chat prompts) — that way all
// UI/AI access stays on the GUI thread.
//
// Endpoints (all JSON unless noted):
//   GET  /                       -> embedded mobile-friendly HTML controller
//   GET  /api/status             -> { online, model, generating, name, hasPin }
//   POST /api/cmd                -> { ok, error? }   body: { cmd, ps:bool }
//   POST /api/chat               -> { reply }        body: { message }
//   GET  /api/buttons            -> [ {icon,label,cmd,ps,confirm}, ... ]
//   POST /api/buttons            -> same array (replaces the saved set)
//
//   OpenAI-compatible (handy when scripting from cURL / SDK / 3rd-party
//   tools that already know how to talk to ChatGPT):
//   GET  /v1/models              -> { object:"list", data:[{ id, ... }] }
//   POST /v1/chat/completions    -> standard OpenAI envelope.
//                                   body: { model?, messages:[{role,content}], ... }
//                                   non-streaming for simplicity.
//
// Auth (optional): if a PIN is configured, every /api/* call must include
// the header `X-JARVIS-PIN: <pin>` (or query string ?pin=...). The PIN
// also gates the embedded HTML's <script> calls.
class LlamaWorkerThread;

class JarvisHttpServer : public QObject {
    Q_OBJECT

public:
    explicit JarvisHttpServer(QObject* parent = nullptr);
    ~JarvisHttpServer() override;

    bool        start(quint16 port);
    void        stop();
    bool        isListening() const;
    quint16     port() const;

    // List of plausible "open me on a phone" URLs derived from the host's
    // non-loopback IPv4 interfaces (Wi-Fi/Ethernet). Empty when offline.
    static QStringList lanUrls(quint16 port);

    // Empty PIN = no auth required. Stored only in memory; persistence is
    // MainWindow's job (QSettings).
    void        setPin(const QString& pin);
    QString     pin() const { return m_pin; }

    // MainWindow surfaces these so we can read worker state for /api/status.
    // We never call .queuePrompt() ourselves — that's done indirectly through
    // signals so MainWindow keeps full control of the chat lifecycle.
    void        setStatusProvider(LlamaWorkerThread* ai,
                                  const QString& userName);
    void        setUserName(const QString& name);

    // Called by MainWindow when an AI reply for a web-initiated chat
    // request is ready. The server matches it to the still-open socket
    // and writes the JSON response. If MainWindow rejects the request
    // (busy, etc.) it should call this with an error string.
    void        completeWebChat(const QString& fullText);
    void        failWebChat(int httpCode, const QString& errorText);

    // Quick-action buttons that the phone UI shows. Stored as a compact
    // JSON array (`[{icon,label,cmd,ps,confirm}, ...]`). Persistence to
    // QSettings lives in MainWindow — we just hold the in-memory copy and
    // emit `buttonsChanged()` when the phone rewrites them.
    void              setButtonsJson(const QByteArray& jsonArray);
    QByteArray        buttonsJson() const { return m_buttonsJson; }
    static QByteArray defaultButtonsJson();

signals:
    // MainWindow connects these — they run on the GUI thread thanks to
    // Qt's auto connection, even though emitted from socket callbacks.
    void        webCommandRequested(const QString& cmd, bool isPowerShell);
    void        webChatRequested(const QString& message);
    void        buttonsChanged(const QByteArray& jsonArray);

private slots:
    void        onNewConnection();
    void        onSocketReadyRead();

private:
    struct Pending {
        QByteArray buffer;
        bool       headersDone = false;
        QByteArray method;
        QByteArray path;
        QHash<QByteArray, QByteArray> headers;
        int        contentLength = 0;
        QByteArray body;
    };

    void        handleRequest(QTcpSocket* sock, const Pending& req);
    bool        checkPin(const Pending& req) const;
    void        writeJson(QTcpSocket* sock, int code,
                          const QByteArray& json,
                          bool keepAlive = false);
    void        writeText(QTcpSocket* sock, int code,
                          const QByteArray& mime,
                          const QByteArray& body,
                          bool keepAlive = false);
    void        writeError(QTcpSocket* sock, int code, const QString& message);
    QByteArray  jarvisHtmlPage() const;

    QTcpServer*                         m_server = nullptr;
    QHash<QTcpSocket*, Pending>         m_pendings;
    LlamaWorkerThread*                  m_ai     = nullptr;
    QString                             m_pin;
    QString                             m_userName;
    QByteArray                          m_buttonsJson;  // canonical JSON array

    // Socket waiting on /api/chat to be completed by MainWindow.
    QTcpSocket*                         m_webChatSocket = nullptr;
    // Same idea for OpenAI-compatible /v1/chat/completions — when this is
    // set we respond with the OpenAI envelope instead of the simple
    // { reply: ... } JSON used by the phone UI.
    QTcpSocket*                         m_v1Socket      = nullptr;
    QString                             m_v1ModelName;     // echoed back in choices
};

#endif // JARVIS_HTTP_SERVER_H
