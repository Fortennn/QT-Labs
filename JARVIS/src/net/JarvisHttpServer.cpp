#include "JarvisHttpServer.h"

#include "../ai/LlamaWorkerThread.h"

#include <QDateTime>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkInterface>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUrl>
#include <QUrlQuery>

namespace {

QByteArray statusText(int code) {
    switch (code) {
    case 200: return "OK";
    case 204: return "No Content";
    case 400: return "Bad Request";
    case 401: return "Unauthorized";
    case 403: return "Forbidden";
    case 404: return "Not Found";
    case 405: return "Method Not Allowed";
    case 409: return "Conflict";
    case 503: return "Service Unavailable";
    case 500:
    default:  return "Internal Server Error";
    }
}

QByteArray buildResponse(int code,
                         const QByteArray& mime,
                         const QByteArray& body,
                         bool keepAlive)
{
    QByteArray r;
    r.reserve(body.size() + 256);
    r.append("HTTP/1.1 ").append(QByteArray::number(code))
        .append(' ').append(statusText(code)).append("\r\n");
    r.append("Content-Type: ").append(mime).append("\r\n");
    r.append("Content-Length: ").append(QByteArray::number(body.size())).append("\r\n");
    r.append("Cache-Control: no-store\r\n");
    r.append("Access-Control-Allow-Origin: *\r\n");
    r.append("Access-Control-Allow-Headers: Content-Type, X-JARVIS-PIN\r\n");
    r.append("Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n");
    r.append("Connection: ").append(keepAlive ? "keep-alive" : "close").append("\r\n");
    r.append("\r\n");
    r.append(body);
    return r;
}

} // namespace

// =============================================================================
//  Construction / lifecycle
// =============================================================================

JarvisHttpServer::JarvisHttpServer(QObject* parent)
    : QObject(parent)
{}

JarvisHttpServer::~JarvisHttpServer() {
    stop();
}

bool JarvisHttpServer::start(quint16 listenPort) {
    stop();
    m_server = new QTcpServer(this);
    connect(m_server, &QTcpServer::newConnection,
            this, &JarvisHttpServer::onNewConnection);
    if (!m_server->listen(QHostAddress::AnyIPv4, listenPort)) {
        delete m_server;
        m_server = nullptr;
        return false;
    }
    return true;
}

void JarvisHttpServer::stop() {
    if (m_webChatSocket) {
        failWebChat(503, QStringLiteral("Server stopping"));
    }
    for (QTcpSocket* s : m_pendings.keys()) {
        if (s) s->disconnectFromHost();
    }
    m_pendings.clear();
    if (m_server) {
        m_server->close();
        m_server->deleteLater();
        m_server = nullptr;
    }
}

bool JarvisHttpServer::isListening() const {
    return m_server && m_server->isListening();
}

quint16 JarvisHttpServer::port() const {
    return m_server ? m_server->serverPort() : 0;
}

QStringList JarvisHttpServer::lanUrls(quint16 listenPort) {
    QStringList out;
    const auto ifaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface& iface : ifaces) {
        if (!(iface.flags() & QNetworkInterface::IsRunning)) continue;
        if (iface.flags() & QNetworkInterface::IsLoopBack) continue;
        for (const QNetworkAddressEntry& entry : iface.addressEntries()) {
            const QHostAddress ip = entry.ip();
            if (ip.protocol() != QAbstractSocket::IPv4Protocol) continue;
            if (ip.isLoopback()) continue;
            const QString s = ip.toString();
            if (s.startsWith(QLatin1String("169.254."))) continue;  // link-local
            out << QStringLiteral("http://%1:%2").arg(s).arg(listenPort);
        }
    }
    return out;
}

void JarvisHttpServer::setPin(const QString& pin) {
    m_pin = pin.trimmed();
}

void JarvisHttpServer::setStatusProvider(LlamaWorkerThread* ai,
                                         const QString& userName)
{
    m_ai = ai;
    m_userName = userName;
}

void JarvisHttpServer::setUserName(const QString& name) {
    m_userName = name;
}

void JarvisHttpServer::setButtonsJson(const QByteArray& jsonArray) {
    // Validate: must be a JSON array; otherwise fall back to defaults.
    const auto doc = QJsonDocument::fromJson(jsonArray);
    if (doc.isArray()) {
        m_buttonsJson = QJsonDocument(doc.array())
                            .toJson(QJsonDocument::Compact);
    } else {
        m_buttonsJson = defaultButtonsJson();
    }
}

QByteArray JarvisHttpServer::defaultButtonsJson() {
    // Stays platform-neutral — only aliases (handled by `handleSystemCommand`)
    // and URLs. Absolutely no `C:/...` paths in defaults.
    QJsonArray a;
    auto add = [&](const QString& icon, const QString& label,
                   const QString& cmd, bool ps = false,
                   const QString& confirm = QString())
    {
        QJsonObject o;
        o.insert(QStringLiteral("icon"),    icon);
        o.insert(QStringLiteral("label"),   label);
        o.insert(QStringLiteral("cmd"),     cmd);
        o.insert(QStringLiteral("ps"),      ps);
        if (!confirm.isEmpty())
            o.insert(QStringLiteral("confirm"), confirm);
        a.append(o);
    };
    add(QStringLiteral("🌐"), QStringLiteral("Chrome"),       QStringLiteral("open chrome"));
    add(QStringLiteral("🎧"), QStringLiteral("SoundCloud"),   QStringLiteral("https://soundcloud.com"));
    add(QStringLiteral("💬"), QStringLiteral("Discord"),      QStringLiteral("open discord"));
    add(QStringLiteral("🎮"), QStringLiteral("Steam"),        QStringLiteral("open steam"));
    add(QStringLiteral("📝"), QStringLiteral("Notepad"),      QStringLiteral("open notepad"));
    add(QStringLiteral("🔢"), QStringLiteral("Calculator"),   QStringLiteral("open calc"));
    add(QStringLiteral("📁"), QStringLiteral("Explorer"),     QStringLiteral("open explorer"));
    add(QStringLiteral("📊"), QStringLiteral("Task Manager"), QStringLiteral("open taskmgr"));
    add(QStringLiteral("⏻"),  QStringLiteral("Shutdown"),
        QStringLiteral("Stop-Computer -Force"), true,
        QStringLiteral("Вимкнути ПК?"));
    add(QStringLiteral("🔄"), QStringLiteral("Reboot"),
        QStringLiteral("Restart-Computer -Force"), true,
        QStringLiteral("Перезавантажити ПК?"));
    add(QStringLiteral("🔒"), QStringLiteral("Lock"),
        QStringLiteral("rundll32.exe user32.dll,LockWorkStation"), true);
    add(QStringLiteral("🔇"), QStringLiteral("Mute"),
        QStringLiteral("(New-Object -ComObject WScript.Shell).SendKeys([char]173)"),
        true);
    return QJsonDocument(a).toJson(QJsonDocument::Compact);
}

// =============================================================================
//  Connection handling
// =============================================================================

void JarvisHttpServer::onNewConnection() {
    while (m_server && m_server->hasPendingConnections()) {
        QTcpSocket* sock = m_server->nextPendingConnection();
        if (!sock) continue;
        m_pendings.insert(sock, Pending{});
        connect(sock, &QTcpSocket::readyRead,
                this, &JarvisHttpServer::onSocketReadyRead);
        connect(sock, &QTcpSocket::disconnected, this, [this, sock]() {
            // If this was the chat socket waiting for a reply, drop the
            // pending state so the next request can fly.
            if (m_webChatSocket == sock) m_webChatSocket = nullptr;
            m_pendings.remove(sock);
            sock->deleteLater();
        });
    }
}

void JarvisHttpServer::onSocketReadyRead() {
    auto* sock = qobject_cast<QTcpSocket*>(sender());
    if (!sock) return;
    auto it = m_pendings.find(sock);
    if (it == m_pendings.end()) return;
    Pending& req = it.value();
    req.buffer.append(sock->readAll());

    // Parse headers once.
    if (!req.headersDone) {
        const int sep = req.buffer.indexOf("\r\n\r\n");
        if (sep < 0) return;  // headers still incomplete
        const QByteArray rawHeaders = req.buffer.left(sep);
        req.body = req.buffer.mid(sep + 4);

        const auto lines = rawHeaders.split('\n');
        if (lines.isEmpty()) {
            writeError(sock, 400, QStringLiteral("Malformed request"));
            sock->disconnectFromHost();
            return;
        }
        // Request-line: METHOD PATH HTTP/1.1
        const QByteArray firstLine = lines.first().trimmed();
        const auto parts = firstLine.split(' ');
        if (parts.size() < 2) {
            writeError(sock, 400, QStringLiteral("Malformed request line"));
            sock->disconnectFromHost();
            return;
        }
        req.method = parts[0].toUpper();
        req.path   = parts[1];
        for (int i = 1; i < lines.size(); ++i) {
            const QByteArray line = lines[i].trimmed();
            if (line.isEmpty()) continue;
            const int colon = line.indexOf(':');
            if (colon < 0) continue;
            QByteArray k = line.left(colon).trimmed().toLower();
            QByteArray v = line.mid(colon + 1).trimmed();
            req.headers.insert(k, v);
        }
        req.contentLength = req.headers.value("content-length", "0").toInt();
        req.headersDone = true;
    }

    // Wait until full body arrives.
    if (req.body.size() < req.contentLength) return;

    handleRequest(sock, req);
    // Pending entry will be cleaned up either on disconnect or right now if
    // we already wrote a final response. Most handlers close the socket;
    // the chat handler holds it open until completeWebChat() fires.
}

// =============================================================================
//  Authentication
// =============================================================================

bool JarvisHttpServer::checkPin(const Pending& req) const {
    if (m_pin.isEmpty()) return true;
    const QByteArray header = req.headers.value("x-jarvis-pin");
    if (header == m_pin.toUtf8()) return true;
    // ?pin= query fallback so the embedded HTML can pass it on first load.
    QUrl u = QUrl::fromEncoded("http://_" + req.path);
    const QString q = QUrlQuery(u).queryItemValue(QStringLiteral("pin"));
    return (q == m_pin);
}

// =============================================================================
//  Routing
// =============================================================================

void JarvisHttpServer::handleRequest(QTcpSocket* sock, const Pending& req) {
    // Strip query string for routing.
    QByteArray path = req.path;
    int q = path.indexOf('?');
    if (q >= 0) path = path.left(q);

    if (req.method == "OPTIONS") {
        writeText(sock, 204, "text/plain", "");
        sock->disconnectFromHost();
        return;
    }

    if (req.method == "GET" && (path == "/" || path == "/index.html")) {
        writeText(sock, 200, "text/html; charset=utf-8", jarvisHtmlPage());
        sock->disconnectFromHost();
        return;
    }

    // Every /api/* is gated by PIN if set.
    if (path.startsWith("/api/") && !checkPin(req)) {
        writeError(sock, 401, QStringLiteral("PIN required"));
        sock->disconnectFromHost();
        return;
    }

    if (req.method == "GET" && path == "/api/status") {
        QJsonObject o;
        o.insert(QStringLiteral("online"), true);
        o.insert(QStringLiteral("model"),
                 m_ai ? m_ai->loadedModelName() : QString());
        o.insert(QStringLiteral("generating"),
                 m_ai && m_ai->isBusy());
        o.insert(QStringLiteral("user"), m_userName);
        o.insert(QStringLiteral("hasPin"), !m_pin.isEmpty());
        o.insert(QStringLiteral("ts"),
                 QDateTime::currentDateTime().toString(Qt::ISODate));
        writeJson(sock, 200,
                  QJsonDocument(o).toJson(QJsonDocument::Compact));
        sock->disconnectFromHost();
        return;
    }

    if (req.method == "GET" && path == "/api/buttons") {
        const QByteArray body = m_buttonsJson.isEmpty()
                                    ? defaultButtonsJson()
                                    : m_buttonsJson;
        writeJson(sock, 200, body);
        sock->disconnectFromHost();
        return;
    }

    if (req.method == "POST" && path == "/api/buttons") {
        const auto doc = QJsonDocument::fromJson(req.body);
        if (!doc.isArray()) {
            writeError(sock, 400, QStringLiteral("Expected JSON array"));
            sock->disconnectFromHost();
            return;
        }
        // Sanity-check shape and re-serialize compactly.
        QJsonArray clean;
        for (const QJsonValue& v : doc.array()) {
            if (!v.isObject()) continue;
            const QJsonObject o = v.toObject();
            QJsonObject n;
            n.insert(QStringLiteral("icon"),
                     o.value(QStringLiteral("icon")).toString());
            n.insert(QStringLiteral("label"),
                     o.value(QStringLiteral("label")).toString().trimmed());
            n.insert(QStringLiteral("cmd"),
                     o.value(QStringLiteral("cmd")).toString().trimmed());
            n.insert(QStringLiteral("ps"),
                     o.value(QStringLiteral("ps")).toBool(false));
            const QString confirmText =
                o.value(QStringLiteral("confirm")).toString();
            if (!confirmText.isEmpty())
                n.insert(QStringLiteral("confirm"), confirmText);
            if (n.value(QStringLiteral("label")).toString().isEmpty()) continue;
            if (n.value(QStringLiteral("cmd")).toString().isEmpty())   continue;
            clean.append(n);
        }
        m_buttonsJson = QJsonDocument(clean).toJson(QJsonDocument::Compact);
        emit buttonsChanged(m_buttonsJson);
        writeJson(sock, 200, m_buttonsJson);
        sock->disconnectFromHost();
        return;
    }

    if (req.method == "POST" && path == "/api/cmd") {
        const auto doc = QJsonDocument::fromJson(req.body);
        if (!doc.isObject()) {
            writeError(sock, 400, QStringLiteral("Expected JSON object"));
            sock->disconnectFromHost();
            return;
        }
        const QJsonObject o = doc.object();
        const QString cmd = o.value(QStringLiteral("cmd")).toString().trimmed();
        const bool   ps   = o.value(QStringLiteral("ps")).toBool(false);
        if (cmd.isEmpty()) {
            writeError(sock, 400, QStringLiteral("Empty cmd"));
            sock->disconnectFromHost();
            return;
        }
        emit webCommandRequested(cmd, ps);
        QJsonObject reply;
        reply.insert(QStringLiteral("ok"), true);
        reply.insert(QStringLiteral("queued"), cmd);
        writeJson(sock, 200,
                  QJsonDocument(reply).toJson(QJsonDocument::Compact));
        sock->disconnectFromHost();
        return;
    }

    if (req.method == "POST" && path == "/api/chat") {
        if (m_webChatSocket) {
            writeError(sock, 409, QStringLiteral("Another web chat is in flight"));
            sock->disconnectFromHost();
            return;
        }
        if (m_ai && m_ai->isBusy()) {
            writeError(sock, 503,
                QStringLiteral("JARVIS is currently generating on the desktop"));
            sock->disconnectFromHost();
            return;
        }
        const auto doc = QJsonDocument::fromJson(req.body);
        if (!doc.isObject()) {
            writeError(sock, 400, QStringLiteral("Expected JSON object"));
            sock->disconnectFromHost();
            return;
        }
        const QString message =
            doc.object().value(QStringLiteral("message")).toString().trimmed();
        if (message.isEmpty()) {
            writeError(sock, 400, QStringLiteral("Empty message"));
            sock->disconnectFromHost();
            return;
        }
        m_webChatSocket = sock;
        emit webChatRequested(message);
        // Response comes later via completeWebChat() / failWebChat().
        return;
    }

    writeError(sock, 404, QStringLiteral("Not found"));
    sock->disconnectFromHost();
}

// =============================================================================
//  Async chat completion hook (called by MainWindow)
// =============================================================================

void JarvisHttpServer::completeWebChat(const QString& fullText) {
    if (!m_webChatSocket) return;
    QJsonObject reply;
    reply.insert(QStringLiteral("ok"), true);
    reply.insert(QStringLiteral("reply"), fullText);
    writeJson(m_webChatSocket, 200,
              QJsonDocument(reply).toJson(QJsonDocument::Compact));
    m_webChatSocket->disconnectFromHost();
    m_webChatSocket = nullptr;
}

void JarvisHttpServer::failWebChat(int httpCode, const QString& errorText) {
    if (!m_webChatSocket) return;
    writeError(m_webChatSocket, httpCode, errorText);
    m_webChatSocket->disconnectFromHost();
    m_webChatSocket = nullptr;
}

// =============================================================================
//  Response helpers
// =============================================================================

void JarvisHttpServer::writeJson(QTcpSocket* sock, int code,
                                 const QByteArray& json,
                                 bool keepAlive)
{
    if (!sock) return;
    sock->write(buildResponse(code, "application/json; charset=utf-8",
                              json, keepAlive));
    sock->flush();
}

void JarvisHttpServer::writeText(QTcpSocket* sock, int code,
                                 const QByteArray& mime,
                                 const QByteArray& body,
                                 bool keepAlive)
{
    if (!sock) return;
    sock->write(buildResponse(code, mime, body, keepAlive));
    sock->flush();
}

void JarvisHttpServer::writeError(QTcpSocket* sock, int code,
                                  const QString& message)
{
    QJsonObject o;
    o.insert(QStringLiteral("ok"), false);
    o.insert(QStringLiteral("error"), message);
    writeJson(sock, code,
              QJsonDocument(o).toJson(QJsonDocument::Compact));
}

// =============================================================================
//  Embedded HTML controller — mobile-first dark UI
// =============================================================================

QByteArray JarvisHttpServer::jarvisHtmlPage() const {
    static const char kPage[] =
R"HTML(<!DOCTYPE html>
<html lang="uk">
<head>
<meta charset="UTF-8" />
<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover" />
<meta name="theme-color" content="#06090d" />
<title>JARVIS · Remote</title>
<style>
  :root {
    --bg:        #06090d;
    --panel:     #0d141d;
    --panel-2:   #16202a;
    --accent:    #2f81f7;
    --accent-2:  #58a6ff;
    --text:      #e6edf3;
    --muted:     #94a3b8;
    --danger:    #f43f5e;
    --ok:        #22c55e;
  }
  * { box-sizing: border-box; }
  html, body {
    margin: 0; padding: 0; height: 100%; background: var(--bg); color: var(--text);
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Inter, Roboto, sans-serif;
    -webkit-font-smoothing: antialiased; overscroll-behavior-y: contain;
  }
  body {
    background:
      radial-gradient(1200px 800px at 80% -10%, rgba(47,129,247,0.18), transparent 60%),
      radial-gradient(900px 700px at -10% 110%, rgba(56,189,248,0.10), transparent 60%),
      var(--bg);
    min-height: 100vh;
  }
  header {
    display: flex; align-items: center; gap: 14px;
    padding: 18px 18px 8px;
  }
  .logo {
    width: 38px; height: 38px; border-radius: 12px;
    background: linear-gradient(135deg, #58a6ff, #1d6def);
    box-shadow: 0 6px 18px rgba(31,110,235,.45);
    display: grid; place-items: center;
    color: #fff; font-weight: 800; font-size: 18px;
  }
  .title { font-size: 16px; font-weight: 800; letter-spacing: 1.5px; }
  .sub   { font-size: 11px; color: var(--muted); letter-spacing: 1px; }
  .pill {
    margin-left: auto; font-size: 11px; padding: 4px 10px; border-radius: 999px;
    background: rgba(34,197,94,.14); color: #4ade80; border: 1px solid rgba(34,197,94,.35);
    font-weight: 700; letter-spacing: 1px;
  }
  .pill.off { background: rgba(244,63,94,.12); color: #fb7185; border-color: rgba(244,63,94,.35); }
  main { padding: 4px 14px 96px; display: flex; flex-direction: column; gap: 14px; }
  .card {
    background: linear-gradient(180deg, var(--panel-2) 0%, var(--panel) 100%);
    border: 1px solid rgba(60,78,102,.5);
    border-radius: 16px; padding: 14px;
    box-shadow: 0 10px 30px rgba(0,0,0,.35);
  }
  h2 {
    margin: 0 0 10px; font-size: 11px; font-weight: 800;
    letter-spacing: 1.6px; text-transform: uppercase; color: var(--muted);
  }
  .grid {
    display: grid; grid-template-columns: repeat(2, 1fr); gap: 8px;
  }
  button.q {
    background: rgba(20,28,40,.9); color: var(--text);
    border: 1px solid rgba(60,78,102,.7); border-radius: 12px;
    padding: 14px 12px; font-size: 13px; font-weight: 600;
    text-align: left; cursor: pointer; transition: all .18s ease;
  }
  button.q:active { transform: scale(.98); }
  button.q:hover  { border-color: var(--accent); color: var(--accent-2); }
  .row { display: flex; gap: 8px; align-items: stretch; }
  textarea, input[type="text"], input[type="password"] {
    flex: 1; background: rgba(10,16,24,.85); color: var(--text);
    border: 1px solid rgba(60,78,102,.7); border-radius: 12px;
    padding: 12px 14px; font-size: 14px; outline: none; resize: vertical;
    min-height: 48px; font-family: inherit;
  }
  textarea:focus, input:focus { border-color: var(--accent); }
  button.send {
    background: linear-gradient(135deg, var(--accent-2), var(--accent));
    color: #fff; border: 0; border-radius: 12px; min-width: 56px;
    font-size: 18px; font-weight: 800; cursor: pointer;
    box-shadow: 0 6px 18px rgba(31,110,235,.45);
  }
  button.send:active { transform: scale(.97); }
  .reply {
    margin-top: 10px; padding: 12px 14px; background: rgba(10,16,24,.7);
    border: 1px dashed rgba(120,140,170,.35); border-radius: 12px;
    color: #cbd5e1; white-space: pre-wrap; font-size: 13.5px; min-height: 24px;
  }
  .toast {
    position: fixed; bottom: 16px; left: 50%; transform: translateX(-50%);
    background: rgba(20,28,40,.95); border: 1px solid rgba(60,78,102,.7);
    border-radius: 12px; padding: 10px 14px; color: var(--text);
    font-size: 13px; opacity: 0; transition: opacity .25s ease; pointer-events: none;
    backdrop-filter: blur(6px);
  }
  .toast.show { opacity: 1; }
  .pin-card { display: none; }
  .pin-card.on { display: block; }
  footer { color: var(--muted); font-size: 11px; text-align: center; padding: 18px; }

  /* — quick-action header with edit toggle — */
  .card-head {
    display: flex; align-items: center; justify-content: space-between;
    margin: 0 0 10px;
  }
  .card-head h2 { margin: 0; }
  .edit-toggle {
    background: rgba(47,129,247,.12); color: var(--accent-2);
    border: 1px solid rgba(47,129,247,.35); border-radius: 999px;
    padding: 5px 12px; font-size: 11px; font-weight: 700; letter-spacing: 1px;
    cursor: pointer; text-transform: uppercase; user-select: none;
  }
  .edit-toggle:active { transform: scale(.96); }
  .edit-toggle.on {
    background: linear-gradient(135deg, var(--accent-2), var(--accent));
    color: #fff; border-color: transparent;
    box-shadow: 0 4px 14px rgba(31,110,235,.45);
  }

  /* edit-mode chrome on each tile */
  button.q { position: relative; }
  button.q .ico { margin-right: 4px; }
  button.q .del {
    position: absolute; top: -6px; right: -6px;
    width: 22px; height: 22px; border-radius: 50%;
    background: var(--danger); color: #fff;
    display: none; align-items: center; justify-content: center;
    font-size: 13px; font-weight: 800;
    box-shadow: 0 4px 10px rgba(244,63,94,.4);
  }
  body.editing button.q { border-style: dashed; cursor: grab; }
  body.editing button.q .del { display: flex; }
  button.add-tile {
    background: rgba(47,129,247,.08); color: var(--accent-2);
    border: 1px dashed rgba(47,129,247,.55); border-radius: 12px;
    padding: 14px 12px; font-size: 13px; font-weight: 700;
    cursor: pointer; text-align: center;
  }
  button.add-tile:active { transform: scale(.98); }

  /* — modal editor — */
  .modal {
    position: fixed; inset: 0; background: rgba(2,6,12,.6);
    display: none; align-items: flex-end; justify-content: center;
    z-index: 50; backdrop-filter: blur(6px);
  }
  .modal.on { display: flex; }
  .sheet {
    width: 100%; max-width: 520px;
    background: linear-gradient(180deg, var(--panel-2), var(--panel));
    border: 1px solid rgba(60,78,102,.6);
    border-radius: 18px 18px 0 0;
    padding: 18px 16px 22px;
    box-shadow: 0 -20px 60px rgba(0,0,0,.6);
    animation: slideUp .22s ease-out;
  }
  @keyframes slideUp { from { transform: translateY(40px); opacity: 0; }
                       to   { transform: translateY(0);    opacity: 1; } }
  .sheet h3 { margin: 0 0 12px; font-size: 14px; letter-spacing: 1px;
              color: var(--text); text-transform: uppercase; }
  .field { display: flex; flex-direction: column; gap: 6px; margin-bottom: 12px; }
  .field label { font-size: 11px; color: var(--muted);
                 text-transform: uppercase; letter-spacing: 1px; }
  .field input[type="text"] { min-height: 0; }
  .switch { display: flex; align-items: center; gap: 10px;
            font-size: 13px; color: var(--text); cursor: pointer;
            padding: 6px 0; user-select: none; }
  .switch input { width: 18px; height: 18px; accent-color: var(--accent); }
  .actions { display: flex; gap: 8px; margin-top: 6px; }
  .actions .ghost {
    flex: 1; background: rgba(20,28,40,.85); color: var(--text);
    border: 1px solid rgba(60,78,102,.7); border-radius: 12px;
    padding: 12px; font-size: 14px; font-weight: 700; cursor: pointer;
  }
  .actions .primary {
    flex: 2; background: linear-gradient(135deg, var(--accent-2), var(--accent));
    color: #fff; border: 0; border-radius: 12px;
    padding: 12px; font-size: 14px; font-weight: 800; cursor: pointer;
    box-shadow: 0 6px 18px rgba(31,110,235,.45);
  }
  .actions .danger {
    background: rgba(244,63,94,.15); color: var(--danger);
    border: 1px solid rgba(244,63,94,.45);
  }
  .hint { font-size: 11px; color: var(--muted); margin-top: -6px; margin-bottom: 8px; }
</style>
</head>
<body>
<header>
  <div class="logo">J</div>
  <div>
    <div class="title">JARVIS · REMOTE</div>
    <div class="sub" id="userLine">connecting…</div>
  </div>
  <div class="pill" id="statusPill">offline</div>
</header>
<main>
  <section class="card pin-card" id="pinCard">
    <h2>PIN</h2>
    <div class="row">
      <input id="pinInput" type="password" placeholder="Введи PIN із вікна налаштувань" />
      <button class="send" onclick="savePin()">OK</button>
    </div>
  </section>

  <section class="card">
    <div class="card-head">
      <h2>Швидкі дії</h2>
      <span class="edit-toggle" id="editToggle" onclick="toggleEdit()">✎ Редагувати</span>
    </div>
    <div class="grid" id="actionsGrid"></div>
  </section>

  <section class="card">
    <h2>Власна команда</h2>
    <div class="row">
      <input id="cmdInput" type="text" placeholder="open winword / close discord / ipconfig" />
      <button class="send" onclick="runCustom()">▶</button>
    </div>
  </section>

  <section class="card">
    <h2>Чат із JARVIS</h2>
    <div class="row">
      <textarea id="msgInput" rows="2" placeholder="Що зробити?"></textarea>
      <button class="send" onclick="sendChat()">▶</button>
    </div>
    <div class="reply" id="replyBox">—</div>
  </section>
</main>

<div class="modal" id="editor" onclick="if(event.target===this) closeEditor()">
  <div class="sheet">
    <h3 id="editorTitle">Нова кнопка</h3>
    <div class="field">
      <label>Іконка (емоджі)</label>
      <input id="fIcon" type="text" maxlength="4" placeholder="🎵" />
    </div>
    <div class="field">
      <label>Назва</label>
      <input id="fLabel" type="text" placeholder="SoundCloud" />
    </div>
    <div class="field">
      <label>Команда</label>
      <input id="fCmd" type="text" placeholder="open discord  /  https://...  /  PowerShell" />
      <div class="hint">
        Aliases: <code>open chrome</code>, <code>close discord</code>.
        Або URL (<code>https://...</code>). Для PS-команд увімкни перемикач нижче.
      </div>
    </div>
    <label class="switch">
      <input id="fPs" type="checkbox" />
      Виконати як PowerShell
    </label>
    <div class="field">
      <label>Підтвердження (опц.)</label>
      <input id="fConfirm" type="text" placeholder="Вимкнути ПК?" />
      <div class="hint">Якщо заповнено — на телефоні зʼявиться запит «Так/Ні».</div>
    </div>
    <div class="actions">
      <button class="ghost"   onclick="closeEditor()">Скасувати</button>
      <button class="ghost danger" id="delBtn" onclick="deleteCurrent()" style="display:none;">Видалити</button>
      <button class="primary" onclick="saveEditor()">Зберегти</button>
    </div>
  </div>
</div>

<div class="toast" id="toast"></div>
<footer>JARVIS · LAN control · v1</footer>

<script>
  const PIN_KEY = "jarvis_pin";
  let pin = localStorage.getItem(PIN_KEY) || "";

  function showToast(t, ms = 1800) {
    const el = document.getElementById("toast");
    el.textContent = t;
    el.classList.add("show");
    clearTimeout(showToast._t);
    showToast._t = setTimeout(() => el.classList.remove("show"), ms);
  }
  function savePin() {
    pin = document.getElementById("pinInput").value.trim();
    localStorage.setItem(PIN_KEY, pin);
    document.getElementById("pinCard").classList.remove("on");
    refreshStatus();
    if (typeof loadButtons === "function") loadButtons();
    showToast("PIN збережено");
  }
  async function api(path, opts = {}) {
    opts.headers = Object.assign({"Content-Type": "application/json"},
                                  opts.headers || {});
    if (pin) opts.headers["X-JARVIS-PIN"] = pin;
    const res = await fetch(path, opts);
    if (res.status === 401) {
      document.getElementById("pinCard").classList.add("on");
      showToast("Потрібен PIN");
      throw new Error("PIN required");
    }
    return res;
  }
  async function cmd(c) {
    try {
      const r = await api("/api/cmd", {
        method: "POST",
        body: JSON.stringify({cmd: c, ps: false})
      });
      if (r.ok) showToast("✓ " + c);
      else      showToast("⚠ " + (await r.json()).error);
    } catch (e) { /* toast already shown */ }
  }
  async function psCmd(c, confirmText) {
    if (confirmText && !confirm(confirmText)) return;
    try {
      const r = await api("/api/cmd", {
        method: "POST",
        body: JSON.stringify({cmd: c, ps: true})
      });
      if (r.ok) showToast("✓ ps");
      else      showToast("⚠ " + (await r.json()).error);
    } catch (e) {}
  }
  async function runCustom() {
    const v = document.getElementById("cmdInput").value.trim();
    if (!v) return;
    cmd(v);
    document.getElementById("cmdInput").value = "";
  }
  async function sendChat() {
    const ta = document.getElementById("msgInput");
    const v  = ta.value.trim();
    if (!v) return;
    document.getElementById("replyBox").textContent = "JARVIS думає…";
    try {
      const r = await api("/api/chat", {
        method: "POST",
        body: JSON.stringify({message: v})
      });
      const j = await r.json();
      document.getElementById("replyBox").textContent = j.reply || j.error || "—";
      if (r.ok) ta.value = "";
    } catch (e) {
      document.getElementById("replyBox").textContent = "⚠ " + e.message;
    }
  }
  async function refreshStatus() {
    try {
      const r = await api("/api/status");
      const j = await r.json();
      const pill = document.getElementById("statusPill");
      pill.textContent = j.generating ? "busy" : "online";
      pill.classList.toggle("off", !!j.generating);
      const u = document.getElementById("userLine");
      u.textContent = (j.user ? j.user + " · " : "") +
                      (j.model ? j.model : "no model");
      if (j.hasPin && !pin) {
        document.getElementById("pinCard").classList.add("on");
      }
    } catch (e) {
      const pill = document.getElementById("statusPill");
      pill.textContent = "offline";
      pill.classList.add("off");
    }
  }
  document.getElementById("cmdInput").addEventListener("keydown", e => {
    if (e.key === "Enter") runCustom();
  });
  document.getElementById("msgInput").addEventListener("keydown", e => {
    if (e.key === "Enter" && !e.shiftKey) { e.preventDefault(); sendChat(); }
  });

  /* ============================================================
     Dynamic quick-action buttons — live edit from the phone
     ============================================================ */
  let buttons     = [];
  let editing     = false;
  let editingIdx  = -1;   // -1 = adding a new one, otherwise edit existing

  function escapeHtml(s) {
    return String(s == null ? "" : s).replace(/[&<>"']/g, c => ({
      "&":"&amp;", "<":"&lt;", ">":"&gt;", "\"":"&quot;", "'":"&#39;"
    })[c]);
  }

  async function loadButtons() {
    try {
      const r = await api("/api/buttons");
      const j = await r.json();
      if (Array.isArray(j)) buttons = j;
      renderButtons();
    } catch (e) { /* PIN handler / offline */ }
  }

  function renderButtons() {
    const g = document.getElementById("actionsGrid");
    g.innerHTML = "";
    buttons.forEach((b, i) => {
      const el = document.createElement("button");
      el.className = "q";
      el.innerHTML =
        '<span class="ico">' + escapeHtml(b.icon || "•") + "</span>" +
        escapeHtml(b.label || "") +
        '<span class="del" data-idx="' + i + '">✕</span>';
      el.addEventListener("click", ev => {
        if (ev.target && ev.target.classList.contains("del")) {
          ev.stopPropagation();
          if (confirm("Видалити «" + (b.label || "") + "»?")) {
            buttons.splice(i, 1);
            saveButtons();
          }
          return;
        }
        if (editing) { openEditor(i); return; }
        if (b.ps) psCmd(b.cmd, b.confirm || "");
        else      cmd(b.cmd);
      });
      g.appendChild(el);
    });
    if (editing) {
      const add = document.createElement("button");
      add.className = "add-tile";
      add.textContent = "+ Додати";
      add.onclick = () => openEditor(-1);
      g.appendChild(add);
    }
  }

  function toggleEdit() {
    editing = !editing;
    document.body.classList.toggle("editing", editing);
    document.getElementById("editToggle").classList.toggle("on", editing);
    document.getElementById("editToggle").textContent = editing ? "✓ Готово" : "✎ Редагувати";
    renderButtons();
  }

  function openEditor(idx) {
    editingIdx = idx;
    const b = idx >= 0 ? buttons[idx] : { icon: "⭐", label: "", cmd: "", ps: false, confirm: "" };
    document.getElementById("editorTitle").textContent =
      idx >= 0 ? "Редагувати кнопку" : "Нова кнопка";
    document.getElementById("fIcon").value    = b.icon    || "";
    document.getElementById("fLabel").value   = b.label   || "";
    document.getElementById("fCmd").value     = b.cmd     || "";
    document.getElementById("fPs").checked    = !!b.ps;
    document.getElementById("fConfirm").value = b.confirm || "";
    document.getElementById("delBtn").style.display = idx >= 0 ? "block" : "none";
    document.getElementById("editor").classList.add("on");
  }
  function closeEditor() {
    document.getElementById("editor").classList.remove("on");
    editingIdx = -1;
  }
  function deleteCurrent() {
    if (editingIdx < 0) return;
    const b = buttons[editingIdx];
    if (!confirm("Видалити «" + (b && b.label || "") + "»?")) return;
    buttons.splice(editingIdx, 1);
    closeEditor();
    saveButtons();
  }
  function saveEditor() {
    const icon    = document.getElementById("fIcon").value.trim();
    const label   = document.getElementById("fLabel").value.trim();
    const cmdStr  = document.getElementById("fCmd").value.trim();
    const ps      = document.getElementById("fPs").checked;
    const conf    = document.getElementById("fConfirm").value.trim();
    if (!label || !cmdStr) {
      showToast("Введи назву і команду");
      return;
    }
    const b = { icon, label, cmd: cmdStr, ps };
    if (conf) b.confirm = conf;
    if (editingIdx >= 0) buttons[editingIdx] = b;
    else                 buttons.push(b);
    closeEditor();
    saveButtons();
  }

  async function saveButtons() {
    try {
      const r = await api("/api/buttons", {
        method: "POST",
        body: JSON.stringify(buttons)
      });
      if (!r.ok) {
        showToast("⚠ " + ((await r.json()).error || "помилка"));
        return;
      }
      const j = await r.json();
      if (Array.isArray(j)) buttons = j;   // canonicalised by server
      renderButtons();
      showToast("✓ збережено");
    } catch (e) {}
  }

  refreshStatus();
  loadButtons();
  setInterval(refreshStatus, 5000);
</script>
</body>
</html>
)HTML";
    return QByteArray::fromRawData(kPage, sizeof(kPage) - 1);
}
