#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "ai/SystemPrompt.h"
#include "net/JarvisHttpServer.h"
#include "ui/SettingsDialog.h"
#include "ui/WelcomeDialog.h"
#include "widgets/BrainVisualizer.h"
#include "widgets/MessageWidget.h"
#include "widgets/ParticleBackground.h"

#include <QColor>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDialog>
#include <QDir>
#include <QEasingCurve>
#include <QEvent>
#include <QFileInfo>
#include <QFont>
#include <QFontDatabase>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QCloseEvent>
#include <QFileDialog>
#include <QIcon>
#include <QAction>
#include <QLineEdit>
#include <QMenu>
#include <QObject>
#include <QSystemTrayIcon>
#include <QTextDocument>
#include <QTextEdit>
#include <functional>
#include <QPainter>
#include <QPainterPath>
#include <QProcess>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionMatchIterator>
#include <QScrollArea>
#include <QScrollBar>
#include <QAbstractAnimation>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QSaveFile>
#include <QSettings>
#include <QStackedLayout>
#include <QUuid>
#include <QWheelEvent>

#include "ChatHistoryDialog.h"
#include <QStandardPaths>
#include <QStyle>
#include <QString>
#include <QStringList>
#include <QThread>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

#include <cstdlib>
#include <optional>

// =============================================================================
//  Private helpers (anonymous namespace) — kept here to avoid touching headers.
// =============================================================================

namespace {

// ---------- Animated drop-shadow on hover ---------------------------------------------------
//
// A tiny QObject that watches a widget for HoverEnter/HoverLeave events and
// animates the blur radius and color of an already-installed
// QGraphicsDropShadowEffect via QPropertyAnimation. Used to give the Send
// button and side panel buttons a "lift + glow" feel without having to
// subclass QPushButton.
class ShadowHoverAnimator : public QObject {
public:
    ShadowHoverAnimator(QWidget* target,
                        QGraphicsDropShadowEffect* effect,
                        qreal blurRest,
                        qreal blurHover,
                        QColor colorRest,
                        QColor colorHover,
                        int durationMs = 180)
        : QObject(target),
          m_target(target),
          m_effect(effect),
          m_blurRest(blurRest),
          m_blurHover(blurHover),
          m_colorRest(std::move(colorRest)),
          m_colorHover(std::move(colorHover))
    {
        target->setAttribute(Qt::WA_Hover, true);

        m_blurAnim = new QPropertyAnimation(effect, "blurRadius", this);
        m_blurAnim->setDuration(durationMs);
        m_blurAnim->setEasingCurve(QEasingCurve::OutCubic);

        m_colorAnim = new QPropertyAnimation(effect, "color", this);
        m_colorAnim->setDuration(durationMs);
        m_colorAnim->setEasingCurve(QEasingCurve::OutCubic);

        target->installEventFilter(this);
    }

protected:
    bool eventFilter(QObject* obj, QEvent* ev) override {
        if (obj == m_target) {
            if (ev->type() == QEvent::HoverEnter) {
                animate(m_blurHover, m_colorHover);
            } else if (ev->type() == QEvent::HoverLeave) {
                animate(m_blurRest, m_colorRest);
            }
        }
        return QObject::eventFilter(obj, ev);
    }

private:
    void animate(qreal blur, const QColor& color) {
        m_blurAnim->stop();
        m_blurAnim->setStartValue(m_effect->blurRadius());
        m_blurAnim->setEndValue(blur);
        m_blurAnim->start();

        m_colorAnim->stop();
        m_colorAnim->setStartValue(m_effect->color());
        m_colorAnim->setEndValue(color);
        m_colorAnim->start();
    }

    QWidget*                   m_target  = nullptr;
    QGraphicsDropShadowEffect* m_effect  = nullptr;
    qreal                      m_blurRest;
    qreal                      m_blurHover;
    QColor                     m_colorRest;
    QColor                     m_colorHover;
    QPropertyAnimation*        m_blurAnim  = nullptr;
    QPropertyAnimation*        m_colorAnim = nullptr;
};

// ---------- Focus animator for the input widget ---------------------------------------------
//
// Animates the input wrapper's drop-shadow color/blur and updates the wrapper's
// dynamic "focused" property (driven from QSS) when the input widget gains or
// loses focus, producing a soft accent glow. We accept any QWidget so the
// wrapper works for both QLineEdit and QTextEdit.
class FocusGlowAnimator : public QObject {
public:
    FocusGlowAnimator(QWidget* lineEdit,
                      QFrame* wrapper,
                      QGraphicsDropShadowEffect* effect)
        : QObject(lineEdit),
          m_lineEdit(lineEdit),
          m_wrapper(wrapper),
          m_effect(effect)
    {
        m_blurAnim = new QPropertyAnimation(effect, "blurRadius", this);
        m_blurAnim->setDuration(220);
        m_blurAnim->setEasingCurve(QEasingCurve::OutCubic);

        m_colorAnim = new QPropertyAnimation(effect, "color", this);
        m_colorAnim->setDuration(220);
        m_colorAnim->setEasingCurve(QEasingCurve::OutCubic);

        lineEdit->installEventFilter(this);
        wrapper->setProperty("focused", false);
    }

protected:
    bool eventFilter(QObject* obj, QEvent* ev) override {
        if (obj == m_lineEdit) {
            if (ev->type() == QEvent::FocusIn) {
                setFocused(true);
            } else if (ev->type() == QEvent::FocusOut) {
                setFocused(false);
            }
        }
        return QObject::eventFilter(obj, ev);
    }

private:
    void setFocused(bool focused) {
        m_wrapper->setProperty("focused", focused);
        // Re-polish so the QSS branch :property([focused="true"]) takes effect.
        m_wrapper->style()->unpolish(m_wrapper);
        m_wrapper->style()->polish(m_wrapper);

        m_blurAnim->stop();
        m_blurAnim->setStartValue(m_effect->blurRadius());
        m_blurAnim->setEndValue(focused ? 46.0 : 28.0);
        m_blurAnim->start();

        m_colorAnim->stop();
        m_colorAnim->setStartValue(m_effect->color());
        m_colorAnim->setEndValue(focused
                                     ? QColor(47, 129, 247, 110)   // soft accent glow
                                     : QColor(0, 0, 0, 150));
        m_colorAnim->start();
    }

    QWidget*                    m_lineEdit = nullptr;
    QFrame*                     m_wrapper  = nullptr;
    QGraphicsDropShadowEffect*  m_effect   = nullptr;
    QPropertyAnimation*         m_blurAnim  = nullptr;
    QPropertyAnimation*         m_colorAnim = nullptr;
};

// ---------- Multi-line text input -----------------------------------------------------------
//
// QTextEdit subclass that:
//   * sends on plain Enter (calls onUserInput via the host MainWindow);
//   * inserts a newline on Shift+Enter;
//   * grows vertically with content up to a hard cap so the input row stays
//     compact for short messages but lets the user type a paragraph.
class ChatInputEdit : public QTextEdit {
public:
    using SendFn = std::function<void()>;

    explicit ChatInputEdit(SendFn onSend, QWidget* parent = nullptr)
        : QTextEdit(parent), m_onSend(std::move(onSend))
    {
        setAcceptRichText(false);
        setTabChangesFocus(true);
        setFrameShape(QFrame::NoFrame);
        setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setLineWrapMode(QTextEdit::WidgetWidth);
        setPlaceholderText(QStringLiteral(
            "Message JARVIS…   (Enter — надіслати, Shift+Enter — новий рядок)"));
        connect(document(), &QTextDocument::contentsChanged,
                this, &ChatInputEdit::adjustHeightToContent);
        adjustHeightToContent();
    }

protected:
    void keyPressEvent(QKeyEvent* e) override {
        const bool isEnter = (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter);
        const bool shift   = e->modifiers().testFlag(Qt::ShiftModifier);
        if (isEnter && !shift) {
            if (m_onSend) m_onSend();
            return;
        }
        QTextEdit::keyPressEvent(e);
    }

private:
    void adjustHeightToContent() {
        // 1 line ~= fontMetrics().height() + a couple of pixels for descender.
        const int oneLine = qMax(20, fontMetrics().height() + 4);
        const int padding = 14;     // top + bottom internal padding
        const int maxLines = 6;
        document()->setTextWidth(viewport() ? viewport()->width() : width());
        const int docH = static_cast<int>(document()->size().height());
        const int desired = qBound(oneLine + padding,
                                   docH + padding,
                                   oneLine * maxLines + padding);
        if (desired != height()) setFixedHeight(desired);
    }

    SendFn m_onSend;
};

// ---------- Tiny utility helpers ------------------------------------------------------------

constexpr int kStickyScrollThresholdPx = 80;

QGraphicsDropShadowEffect* makeShadow(QObject* parent,
                                      qreal blur,
                                      QPointF offset,
                                      QColor color) {
    auto* fx = new QGraphicsDropShadowEffect(parent);
    fx->setBlurRadius(blur);
    fx->setOffset(offset.x(), offset.y());
    fx->setColor(std::move(color));
    return fx;
}

// Expand %VAR% style env-var references using the process environment.
QString expandWindowsEnv(const QString& path) {
    static const QRegularExpression varRe(QStringLiteral("%([^%]+)%"));
    QString out = path;
    QRegularExpressionMatchIterator it = varRe.globalMatch(path);
    while (it.hasNext()) {
        const auto m = it.next();
        const QString varName = m.captured(1);
        const QByteArray val = qgetenv(varName.toLocal8Bit().constData());
        if (!val.isEmpty()) {
            out.replace(QLatin1Char('%') + varName + QLatin1Char('%'),
                        QString::fromLocal8Bit(val));
        }
    }
    return out;
}

QString defaultModelPath() {
    QStringList rootCandidates = {
        QDir::current().absoluteFilePath("models"),
        QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("models"),
    };

    for (const QString& rootPath : rootCandidates) {
        QDir modelDir(rootPath);
        if (!modelDir.exists()) continue;

        const QStringList models = modelDir.entryList({"*.gguf"}, QDir::Files, QDir::Name);
        if (models.isEmpty()) continue;

        for (const QString& f : models) {
            if (f.contains("dolphin", Qt::CaseInsensitive)) {
                return modelDir.absoluteFilePath(f);
            }
        }
        return modelDir.absoluteFilePath(models.first());
    }
    return QDir::current().absoluteFilePath("models/dolphin.gguf");
}

// ---------- Known-app resolution ------------------------------------------------------------
//
// Each entry maps one or more aliases to a list of candidate executable paths
// (with %ENV% references) and optional default args. The first candidate that
// exists on disk wins.
struct KnownApp {
    QStringList aliases;
    QStringList candidates;
    QStringList defaultArgs;
};

const QList<KnownApp>& knownAppRegistry() {
    static const QList<KnownApp> apps = {
        // --- Browsers ---
        {{"chrome", "googlechrome", "google-chrome"},
         {"%ProgramFiles%/Google/Chrome/Application/chrome.exe",
          "%ProgramFiles(x86)%/Google/Chrome/Application/chrome.exe",
          "%LOCALAPPDATA%/Google/Chrome/Application/chrome.exe"},
         {}},
        {{"edge", "msedge"},
         {"%ProgramFiles(x86)%/Microsoft/Edge/Application/msedge.exe",
          "%ProgramFiles%/Microsoft/Edge/Application/msedge.exe"},
         {}},
        {{"firefox", "mozilla"},
         {"%ProgramFiles%/Mozilla Firefox/firefox.exe",
          "%ProgramFiles(x86)%/Mozilla Firefox/firefox.exe"},
         {}},
        {{"brave"},
         {"%ProgramFiles%/BraveSoftware/Brave-Browser/Application/brave.exe",
          "%ProgramFiles(x86)%/BraveSoftware/Brave-Browser/Application/brave.exe",
          "%LOCALAPPDATA%/BraveSoftware/Brave-Browser/Application/brave.exe"},
         {}},
        {{"opera"},
         {"%LOCALAPPDATA%/Programs/Opera/launcher.exe",
          "%ProgramFiles%/Opera/launcher.exe"},
         {}},

        // --- Communication ---
        {{"discord"},
         {"%LOCALAPPDATA%/Discord/Update.exe"},
         {"--processStart", "Discord.exe"}},
        {{"telegram"},
         {"%APPDATA%/Telegram Desktop/Telegram.exe",
          "%LOCALAPPDATA%/Telegram Desktop/Telegram.exe"},
         {}},
        {{"slack"},
         {"%LOCALAPPDATA%/slack/slack.exe"},
         {}},
        {{"zoom"},
         {"%APPDATA%/Zoom/bin/Zoom.exe"},
         {}},
        {{"teams", "msteams"},
         {"%LOCALAPPDATA%/Microsoft/Teams/current/Teams.exe",
          "%LOCALAPPDATA%/Microsoft/WindowsApps/ms-teams.exe"},
         {}},

        // --- Media ---
        {{"spotify"},
         {"%APPDATA%/Spotify/Spotify.exe",
          "%LOCALAPPDATA%/Microsoft/WindowsApps/Spotify.exe"},
         {}},
        {{"vlc"},
         {"%ProgramFiles%/VideoLAN/VLC/vlc.exe",
          "%ProgramFiles(x86)%/VideoLAN/VLC/vlc.exe"},
         {}},
        {{"obs", "obsstudio"},
         {"%ProgramFiles%/obs-studio/bin/64bit/obs64.exe",
          "%ProgramFiles(x86)%/obs-studio/bin/64bit/obs64.exe"},
         {}},

        // --- Gaming ---
        {{"steam"},
         {"%ProgramFiles(x86)%/Steam/steam.exe",
          "%ProgramFiles%/Steam/steam.exe"},
         {}},
        {{"epic", "epicgames"},
         {"%ProgramFiles(x86)%/Epic Games/Launcher/Portal/Binaries/Win64/EpicGamesLauncher.exe"},
         {}},

        // --- Dev / Editors ---
        {{"vscode", "code"},
         {"%LOCALAPPDATA%/Programs/Microsoft VS Code/Code.exe",
          "%ProgramFiles%/Microsoft VS Code/Code.exe"},
         {}},
        {{"notepad++", "notepadplusplus"},
         {"%ProgramFiles%/Notepad++/notepad++.exe",
          "%ProgramFiles(x86)%/Notepad++/notepad++.exe"},
         {}},
        {{"sublime", "sublimetext"},
         {"%ProgramFiles%/Sublime Text/sublime_text.exe",
          "%ProgramFiles%/Sublime Text 3/sublime_text.exe"},
         {}},

        // --- Office / Misc ---
        {{"obsidian"},
         {"%LOCALAPPDATA%/Obsidian/Obsidian.exe"},
         {}},
        {{"figma"},
         {"%LOCALAPPDATA%/Figma/Figma.exe"},
         {}},

        // --- Built-in Windows ---
        {{"notepad"},
         {"%WINDIR%/system32/notepad.exe"},
         {}},
        {{"calc", "calculator"},
         {"%WINDIR%/system32/calc.exe"},
         {}},
        {{"explorer", "files"},
         {"%WINDIR%/explorer.exe"},
         {}},
        {{"cmd", "console"},
         {"%WINDIR%/system32/cmd.exe"},
         {}},
        {{"powershell", "ps"},
         {"%WINDIR%/system32/WindowsPowerShell/v1.0/powershell.exe"},
         {}},
        {{"taskmgr", "taskmanager"},
         {"%WINDIR%/system32/Taskmgr.exe"},
         {}},
        {{"mspaint", "paint"},
         {"%WINDIR%/system32/mspaint.exe"},
         {}},
        {{"snippingtool"},
         {"%WINDIR%/system32/SnippingTool.exe"},
         {}},
    };
    return apps;
}

struct ResolvedApp {
    QString     program;
    QStringList args;
};

std::optional<ResolvedApp> resolveKnownApp(const QString& aliasIn) {
    const QString alias = aliasIn.trimmed().toLower();
    if (alias.isEmpty()) return std::nullopt;

    for (const auto& app : knownAppRegistry()) {
        for (const auto& a : app.aliases) {
            if (a.compare(alias, Qt::CaseInsensitive) != 0) continue;
            for (const auto& candidate : app.candidates) {
                const QString resolved = QDir::cleanPath(expandWindowsEnv(candidate));
                if (QFileInfo::exists(resolved)) {
                    return ResolvedApp{resolved, app.defaultArgs};
                }
            }
            // Alias matched but no candidate exists on disk — return the
            // *first* candidate anyway so PowerShell `Start-Process` can
            // try to find it via the App Paths registry as a last resort.
            if (!app.candidates.isEmpty()) {
                return ResolvedApp{
                    QDir::cleanPath(expandWindowsEnv(app.candidates.first())),
                    app.defaultArgs};
            }
        }
    }
    return std::nullopt;
}

// "open google.com", "start https://...", "https://..." — all should be URLs.
bool looksLikeUrl(const QString& s) {
    static const QRegularExpression re(
        QStringLiteral(R"(^[a-z][a-z0-9+\-.]*://)"),
        QRegularExpression::CaseInsensitiveOption);
    if (re.match(s).hasMatch()) return true;
    // bare host with TLD, e.g. "google.com" or "www.example.org/path"
    static const QRegularExpression hostRe(
        QStringLiteral(R"(^(?:www\.)?[a-z0-9][a-z0-9\-]*(?:\.[a-z]{2,})+(?:/.*)?$)"),
        QRegularExpression::CaseInsensitiveOption);
    return hostRe.match(s).hasMatch();
}

// ---------- Smooth wheel scroll for QScrollArea -----------------------------
//
// Replaces the default snap-by-step scrolling with an interpolated
// QPropertyAnimation on the vertical scroll bar's value. Multiple wheel
// events accumulate so the scroll feels continuous instead of stuttering.
class SmoothScrollFilter : public QObject {
public:
    explicit SmoothScrollFilter(QScrollArea* area)
        : QObject(area), m_area(area)
    {
        m_anim = new QPropertyAnimation(area->verticalScrollBar(), "value", this);
        m_anim->setDuration(220);
        m_anim->setEasingCurve(QEasingCurve::OutCubic);
    }
protected:
    bool eventFilter(QObject* obj, QEvent* ev) override {
        if (ev->type() == QEvent::Wheel && obj == m_area->viewport()) {
            auto* we = static_cast<QWheelEvent*>(ev);
            const int delta = we->angleDelta().y();
            if (delta == 0) return false;

            QScrollBar* bar = m_area->verticalScrollBar();
            if (!bar) return false;

            const int currentTarget = (m_anim->state() == QAbstractAnimation::Running)
                                          ? m_anim->endValue().toInt()
                                          : bar->value();
            int next = currentTarget - delta;            // wheel up → smaller value
            next = qBound(bar->minimum(), next, bar->maximum());

            m_anim->stop();
            m_anim->setStartValue(bar->value());
            m_anim->setEndValue(next);
            m_anim->start();
            return true; // consume — we're driving the bar ourselves
        }
        return QObject::eventFilter(obj, ev);
    }
private:
    QScrollArea*        m_area;
    QPropertyAnimation* m_anim = nullptr;
};

} // namespace

// =============================================================================
//  MainWindow
// =============================================================================

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    applyPremiumStyles();

    // The MainWindow.ui hard-codes a per-widget stylesheet on `sidePanel`
    // with `border-right: 1px solid #1f6feb;` — that local stylesheet wins
    // over the global one applied in applyPremiumStyles(), so the blue
    // stripe stayed visible. Wipe the per-widget rule so our global rule
    // (no accent border) takes effect.
    if (ui->sidePanel) ui->sidePanel->setStyleSheet(QString());

    // 0. Onboarding (first run): ask the user for a display name. Persisted via
    // QSettings, so subsequent launches skip the dialog entirely.
    {
        QString savedName = WelcomeDialog::savedName();
        if (savedName.isEmpty()) {
            WelcomeDialog dlg(this);
            if (dlg.exec() == QDialog::Accepted) {
                const QString chosen = dlg.chosenName();
                if (!chosen.isEmpty()) {
                    WelcomeDialog::persist(chosen);
                    savedName = chosen;
                }
            }
        }
        if (!savedName.isEmpty()) {
            MessageWidget::setUserDisplayName(savedName);
        }
    }

    // 1. Brain visualizer in the side panel
    auto* brainLayout = new QVBoxLayout(ui->brainContainer);
    auto* brain = new BrainVisualizer(this);
    brainLayout->addWidget(brain);
    brainLayout->setContentsMargins(0, 0, 0, 0);

    // 2. Build chat surface
    setupDynamicUi();

    // 3. Build the polished side panel (header card + status + telemetry + Stop btn).
    buildSidebar();

    // 4. AI worker thread
    aiThread = new LlamaWorkerThread(this);

    // "Новий чат" — saves the current conversation to disk (if not empty)
    // and starts a fresh one.
    ui->btn_clear->setText(QStringLiteral("Новий чат"));
    connect(ui->btn_clear, &QPushButton::clicked, this, [this]() {
        newChat(/*persistOld=*/true);
    });

    connect(ui->btn_settings, &QPushButton::clicked, this, [this]() {
        SettingsDialog dlg(this);
        if (dlg.exec() != QDialog::Accepted) return;

        aiThread->setGenParams(dlg.getGenParams());
        aiThread->setSystemPromptOverride(dlg.getSystemPromptOverride());
        const QString model = dlg.getSelectedModel();
        if (!model.isEmpty()) {
            m_lastModelPath = model;
            aiThread->queueLoadModel(model);
        }

        // Apply visual prefs immediately + update user name + chip.
        const QString name = dlg.getUserName();
        if (!name.isEmpty()) {
            MessageWidget::setUserDisplayName(name);
            WelcomeDialog::persist(name);
        }
        applyUiPreferences(dlg);
        refreshUserChip();
        // Toggle / port / pin may have changed — re-apply the LAN server.
        applyServerPreferences();
    });

    connect(aiThread, &LlamaWorkerThread::tokenGenerated,
            this, &MainWindow::updateAiStream, Qt::QueuedConnection);
    connect(aiThread, &LlamaWorkerThread::replyFinished,
            this, [this](const QString& full) {
                onReplyFinished(full);
                setGenerating(false);
            }, Qt::QueuedConnection);

    connect(aiThread, &LlamaWorkerThread::errorOccurred, this, [this](const QString& err) {
        addMessage(QStringLiteral("ERROR: ") + err, false);
        setGenerating(false);
        // If a phone is parked on /api/chat, surface the error there too
        // so the mobile UI doesn't hang on "JARVIS думає…" forever.
        if (m_webChatPending && httpServer) {
            m_webChatPending = false;
            httpServer->failWebChat(500, err);
        }
    }, Qt::QueuedConnection);

    connect(aiThread, &LlamaWorkerThread::modelLoaded, this, [this](bool success) {
        if (!success) {
            if (sidebarStatusBig) sidebarStatusBig->setText(QStringLiteral("OFFLINE"));
            return;
        }
        addMessage(QStringLiteral("JARVIS CORE ONLINE. Чим можу допомогти?"), false);
        if (sidebarModelLabel) {
            const QString p = m_lastModelPath.isEmpty() ? defaultModelPath()
                                                        : m_lastModelPath;
            sidebarModelLabel->setText(QFileInfo(p).fileName());
        }
        if (sidebarStatusBig) sidebarStatusBig->setText(QStringLiteral("ONLINE"));
    }, Qt::QueuedConnection);

    aiThread->setStackSize(16 * 1024 * 1024);
    aiThread->start();

    // Apply persisted preferences (theme, opacity, timestamps, sampler params,
    // system prompt, last selected model) BEFORE the first model load so the
    // worker boots up with the correct knobs.
    applyPersistedPreferences();

    // 5. Micro-animations — installed AFTER widgets exist
    installSendButtonAnimation();
    installSideButtonAnimations();
    installInputFocusAnimation();

    // 6. System tray. Safe no-op on systems without one.
    setupTrayIcon();

    // 7. LAN web server (phone control panel). Reads QSettings; no-op if
    //    the user disabled it. Spinning up after the AI thread + tray so
    //    /api/status reports a sensible model name immediately.
    applyServerPreferences();
}

MainWindow::~MainWindow() {
    // Persist whatever the user typed before we tear everything down.
    saveCurrentChat();

    if (httpServer) {
        httpServer->stop();
        // Owned-by-this; deleteLater lets pending socket events drain.
        httpServer->deleteLater();
        httpServer = nullptr;
    }
    if (aiThread) {
        aiThread->stopGeneration();
        aiThread->quit();
        aiThread->wait();
    }
    delete ui;
}

// =============================================================================
//  UI construction
// =============================================================================

void MainWindow::applyPremiumStyles() {
    // Pick the most refined system font available, with sensible fallbacks.
    QFont appFont(QStringLiteral("Inter"), 10);
    if (!QFontDatabase().families().contains(QStringLiteral("Inter"))) {
        appFont = QFont(QStringLiteral("Segoe UI Variable Display"), 10);
        if (!QFontDatabase().families().contains(QStringLiteral("Segoe UI Variable Display"))) {
            appFont = QFont(QStringLiteral("Segoe UI"), 10);
        }
    }
    appFont.setStyleStrategy(QFont::PreferAntialias);
    appFont.setHintingPreference(QFont::PreferFullHinting);
    setFont(appFont);

    // Premium dark "obsidian/graphite" stylesheet.
    setStyleSheet(R"QSS(
        /* ---------- Window-level palette ---------- */
        QMainWindow {
            background-color: #06090d;
            color: #e6edf3;
        }
        QWidget {
            color: #e6edf3;
            font-family: 'Inter', 'Segoe UI Variable Display', 'Segoe UI',
                         'SF Pro Display', sans-serif;
            font-size: 13px;
        }

        /* ---------- Side panel (glassmorphic graphite) ---------- */
        QFrame#sidePanel {
            background-color: qlineargradient(
                x1:0, y1:0, x2:0, y2:1,
                stop:0 #0d141d, stop:1 #07101b);
            border: none;
        }
        QFrame#sidePanel QLabel {
            color: #8a99b1;
            font-family: 'Inter', 'Segoe UI Semibold', sans-serif;
            letter-spacing: 1.2px;
            font-weight: 600;
        }

        /* Sidebar buttons — neumorphic-ish: soft inset border, gradient fill */
        QPushButton#btn_clear, QPushButton#btn_settings {
            color: #d7dee7;
            background-color: qlineargradient(
                x1:0, y1:0, x2:0, y2:1,
                stop:0 #131c28, stop:1 #0c1420);
            border: 1px solid #1d2a3b;
            border-radius: 12px;
            padding: 12px 14px;
            font-size: 11px;
            font-weight: 700;
            letter-spacing: 2px;
            text-transform: uppercase;
        }
        QPushButton#btn_clear:hover, QPushButton#btn_settings:hover {
            background-color: qlineargradient(
                x1:0, y1:0, x2:0, y2:1,
                stop:0 #1a2638, stop:1 #111b2a);
            border-color: #2f81f7;
            color: #ffffff;
        }
        QPushButton#btn_clear:pressed, QPushButton#btn_settings:pressed {
            background-color: #0d1422;
        }

        /* Status label */
        QLabel#label_status {
            color: #6b7a90;
            font-size: 10px;
            font-weight: 600;
            letter-spacing: 1.5px;
        }

        /* ---------- Main area background ---------- */
        QFrame#mainArea {
            background-color: #06090d;
        }

        /* ---------- Premium scrollbars ---------- */
        QScrollBar:vertical {
            background: transparent;
            width: 14px;
            margin: 8px 4px 8px 0;
            border: none;
        }
        QScrollBar::handle:vertical {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 rgba(120, 145, 180, 150),
                stop:1 rgba(70,  90,  120, 150));
            border: 1px solid rgba(255, 255, 255, 12);
            border-radius: 6px;
            min-height: 40px;
        }
        QScrollBar::handle:vertical:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #79bbff, stop:1 #2f81f7);
            border: 1px solid rgba(255, 255, 255, 28);
        }
        QScrollBar::handle:vertical:pressed {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #2f81f7, stop:1 #1158c7);
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0; background: transparent;
        }
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
            background: transparent;
        }
        QScrollBar:horizontal { height: 0; background: transparent; }
    )QSS");
}

void MainWindow::setupDynamicUi() {
    // ---- Root: QStackedLayout::StackAll ----
    // Particle bg on bottom, transparent foreground (chat + input) on top.
    auto* mainLayout = new QVBoxLayout(ui->mainArea);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    auto* stackedRoot = new QWidget(ui->mainArea);
    stackedRoot->setAttribute(Qt::WA_StyledBackground, true);

    auto* layered = new QStackedLayout(stackedRoot);
    layered->setStackingMode(QStackedLayout::StackAll);
    layered->setContentsMargins(0, 0, 0, 0);

    // -- Bottom: animated aurora background --
    particleBg = new ParticleBackground(stackedRoot);
    particleBg->setObjectName("particleBackground");
    particleBg->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    layered->addWidget(particleBg);

    // -- Top: foreground container --
    auto* foreground = new QWidget(stackedRoot);
    foreground->setAttribute(Qt::WA_StyledBackground, true);
    foreground->setStyleSheet("background: transparent;");

    auto* fgLayout = new QVBoxLayout(foreground);
    fgLayout->setContentsMargins(32, 28, 32, 24);
    fgLayout->setSpacing(18);

    // ---- Chat scroll area ----
    scrollArea = new QScrollArea(foreground);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; }");
    scrollArea->viewport()->setStyleSheet("background: transparent;");

    auto* scrollContent = new QWidget;
    scrollContent->setAttribute(Qt::WA_StyledBackground, true);
    scrollContent->setStyleSheet("background: transparent;");
    chatLayout = new QVBoxLayout(scrollContent);
    chatLayout->setContentsMargins(2, 4, 6, 4);
    chatLayout->setSpacing(12);
    chatLayout->addStretch();
    scrollArea->setWidget(scrollContent);

    // Smooth wheel scroll: interpolated QPropertyAnimation on the vertical
    // scroll bar, accumulating across rapid wheel events.
    {
        auto* smooth = new SmoothScrollFilter(scrollArea);
        scrollArea->viewport()->installEventFilter(smooth);
    }

    fgLayout->addWidget(scrollArea, /*stretch=*/1);

    // ---- Input wrapper (glassmorphic, focus-glow ready) ----
    inputWrapper = new QFrame(foreground);
    inputWrapper->setObjectName("inputWrapper");
    inputWrapper->setMinimumHeight(64);
    inputWrapper->setAttribute(Qt::WA_StyledBackground, true);
    inputWrapper->setStyleSheet(R"QSS(
        QFrame#inputWrapper {
            background-color: qlineargradient(
                x1:0, y1:0, x2:0, y2:1,
                stop:0 rgba(20, 26, 36, 235),
                stop:1 rgba(10, 14, 22, 245));
            border: 1px solid rgba(60, 78, 102, 180);
            border-radius: 18px;
        }
        QFrame#inputWrapper[focused="true"] {
            border: 1px solid #2f81f7;
        }
    )QSS");

    inputShadow = makeShadow(inputWrapper, 28.0, QPointF(0, 10), QColor(0, 0, 0, 150));
    inputWrapper->setGraphicsEffect(inputShadow);

    auto* inputLayout = new QHBoxLayout(inputWrapper);
    inputLayout->setContentsMargins(20, 10, 10, 10);
    inputLayout->setSpacing(12);

    inputField = new ChatInputEdit([this]() { onUserInput(); }, inputWrapper);
    inputField->setStyleSheet(R"QSS(
        QTextEdit {
            background: transparent;
            color: #e6edf3;
            border: none;
            font-size: 14.5px;
            font-weight: 500;
            padding: 4px 2px;
            selection-background-color: #2f81f7;
        }
        QScrollBar:vertical {
            background: transparent;
            width: 8px;
            margin: 4px 0 4px 0;
        }
        QScrollBar::handle:vertical {
            background: rgba(80, 100, 130, 140);
            border-radius: 4px;
            min-height: 24px;
        }
        QScrollBar::handle:vertical:hover {
            background: rgba(120, 150, 200, 200);
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
    )QSS");

    // ---- Send button: gradient circle with chevron + animated glow ----
    sendButton = new QPushButton(QStringLiteral("▶"), inputWrapper);
    sendButton->setObjectName("sendButton");
    sendButton->setFixedSize(46, 46);
    sendButton->setCursor(Qt::PointingHandCursor);
    sendButton->setStyleSheet(R"QSS(
        QPushButton#sendButton {
            background-color: qlineargradient(
                x1:0, y1:0, x2:1, y2:1,
                stop:0 #58a6ff, stop:0.55 #2f81f7, stop:1 #1d6def);
            color: #ffffff;
            border: none;
            border-radius: 23px;
            font-size: 15px;
            font-weight: 700;
            padding-left: 2px;   /* visually center the chevron */
        }
        QPushButton#sendButton:hover {
            background-color: qlineargradient(
                x1:0, y1:0, x2:1, y2:1,
                stop:0 #79bbff, stop:0.55 #4593f9, stop:1 #2f81f7);
        }
        QPushButton#sendButton:pressed {
            background-color: qlineargradient(
                x1:0, y1:0, x2:1, y2:1,
                stop:0 #2f81f7, stop:1 #1158c7);
        }
    )QSS");

    sendShadow = makeShadow(sendButton, 18.0, QPointF(0, 4), QColor(31, 110, 235, 110));
    sendButton->setGraphicsEffect(sendShadow);

    inputLayout->addWidget(inputField, /*stretch=*/1);
    inputLayout->addWidget(sendButton);

    fgLayout->addWidget(inputWrapper);

    // Add layers — first widget is rendered behind, last on top.
    layered->addWidget(foreground);
    layered->setCurrentWidget(foreground);
    foreground->raise();

    mainLayout->addWidget(stackedRoot);

    // Connections — Enter is wired up inside ChatInputEdit so we don't need
    // returnPressed() here. The send button still routes through onUserInput.
    connect(sendButton, &QPushButton::clicked, this, &MainWindow::onUserInput);
}

void MainWindow::installSendButtonAnimation() {
    if (!sendButton || !sendShadow) return;
    new ShadowHoverAnimator(
        sendButton, sendShadow,
        /*blurRest=*/18.0,
        /*blurHover=*/40.0,
        /*colorRest=*/ QColor(31, 110, 235, 110),
        /*colorHover=*/QColor(88, 166, 255, 220),
        /*duration=*/220);
}

void MainWindow::installSideButtonAnimations() {
    auto installFor = [this](QPushButton* btn) {
        if (!btn) return;
        auto* fx = makeShadow(btn, 0.0, QPointF(0, 0), QColor(47, 129, 247, 0));
        btn->setGraphicsEffect(fx);
        new ShadowHoverAnimator(
            btn, fx,
            /*blurRest=*/0.0,
            /*blurHover=*/22.0,
            /*colorRest=*/ QColor(47, 129, 247, 0),
            /*colorHover=*/QColor(47, 129, 247, 110),
            /*duration=*/200);
    };
    installFor(ui->btn_clear);
    installFor(ui->btn_settings);
}

void MainWindow::installInputFocusAnimation() {
    if (!inputField || !inputWrapper || !inputShadow) return;
    new FocusGlowAnimator(inputField, inputWrapper, inputShadow);
}

// =============================================================================
//  Sidebar polish
// =============================================================================
//
// The .ui file gives us a fixed skeleton:
//   0: brainContainer
//   1: spacer1
//   2: btn_clear
//   3: btn_settings
//   4: spacer2
//   5: label_status
//
// We graft on top of that:
//   - a "JARVIS / PERSONAL AI" header card (index 0)
//   - a "MODEL" indicator card just under the brain
//   - a "STOP GENERATION" button between btn_settings and the bottom
//   - a status card at the very bottom that wraps label_status + telemetry
// We never delete the existing widgets — we only insert / re-style.

void MainWindow::buildSidebar() {
    if (!ui || !ui->sideLayout) return;

    // ---- 1. Header card (top of the panel) -----------------------------------
    auto* headerCard = new QFrame(this);
    headerCard->setObjectName("sidebarHeaderCard");
    headerCard->setStyleSheet(QStringLiteral(R"_(
        QFrame#sidebarHeaderCard {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 rgba(31, 110, 235, 35),
                stop:1 rgba(13,  19,  28, 230));
            border: 1px solid rgba(80, 130, 200, 90);
            border-radius: 14px;
        }
        QFrame#sidebarHeaderCard QLabel { color: #d8e3f5; }
    )_"));
    auto* headerLay = new QVBoxLayout(headerCard);
    headerLay->setContentsMargins(16, 12, 16, 12);
    headerLay->setSpacing(2);
    auto* hTitle = new QLabel(QStringLiteral("JARVIS"), headerCard);
    hTitle->setStyleSheet(QStringLiteral(
        "color: #ffffff; font-size: 19px; font-weight: 800; letter-spacing: 4px;"));
    auto* hSub = new QLabel(QStringLiteral("PERSONAL · AI · CORE"), headerCard);
    hSub->setStyleSheet(QStringLiteral(
        "color: #58a6ff; font-size: 9.5px; font-weight: 700; letter-spacing: 3px;"));
    headerLay->addWidget(hTitle);
    headerLay->addWidget(hSub);
    ui->sideLayout->insertWidget(0, headerCard);

    // ---- 2. Model card (just under the brain visualizer) ---------------------
    // After the header insertion, brainContainer is at index 1, spacer1 at 2.
    auto* modelCard = new QFrame(this);
    modelCard->setObjectName("sidebarModelCard");
    modelCard->setStyleSheet(QStringLiteral(R"_(
        QFrame#sidebarModelCard {
            background: rgba(13, 19, 28, 220);
            border: 1px solid rgba(60, 78, 102, 130);
            border-radius: 12px;
        }
    )_"));
    auto* modelLay = new QVBoxLayout(modelCard);
    modelLay->setContentsMargins(14, 10, 14, 10);
    modelLay->setSpacing(2);

    auto* modelCaption = new QLabel(QStringLiteral("ACTIVE MODEL"), modelCard);
    modelCaption->setStyleSheet(QStringLiteral(
        "color: #58a6ff; font-size: 9.5px; font-weight: 700; letter-spacing: 2.5px;"));

    sidebarModelLabel = new QLabel(QStringLiteral("loading…"), modelCard);
    sidebarModelLabel->setStyleSheet(QStringLiteral(
        "color: #e6edf3; font-size: 12.5px; font-weight: 600;"));
    sidebarModelLabel->setWordWrap(true);

    modelLay->addWidget(modelCaption);
    modelLay->addWidget(sidebarModelLabel);
    ui->sideLayout->insertWidget(2, modelCard);

    // ---- 3. "Історія чатів" sidebar button (programmatically added). ----------
    btnHistory = new QPushButton(QStringLiteral("Історія чатів"), this);
    btnHistory->setObjectName(QStringLiteral("btn_history"));
    btnHistory->setCursor(Qt::PointingHandCursor);
    btnHistory->setMinimumHeight(40);
    btnHistory->setStyleSheet(QStringLiteral(R"_(
        QPushButton#btn_history {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #182336, stop:1 #0e1626);
            color: #d1d7df;
            border: 1px solid #2a3a52;
            border-radius: 12px;
            padding: 10px 14px;
            font-size: 12.5px;
            font-weight: 700;
            letter-spacing: 1px;
        }
        QPushButton#btn_history:hover {
            border-color: #2f81f7;
            color: #ffffff;
        }
        QPushButton#btn_history:pressed {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #2f81f7, stop:1 #1d6def);
            color: #ffffff;
        }
    )_"));
    {
        // Place it just below btn_clear so the order is: Новий чат → Історія
        //  → Налаштування.
        const int insertAt =
            ui->sideLayout->indexOf(ui->btn_clear) + 1;
        ui->sideLayout->insertWidget(insertAt, btnHistory);
    }
    connect(btnHistory, &QPushButton::clicked,
            this, &MainWindow::openChatHistoryDialog);

    // ---- 3b. "Експорт чату" (markdown). Same look as btn_history. ----
    auto* btnExport = new QPushButton(QStringLiteral("Експорт чату"), this);
    btnExport->setObjectName(QStringLiteral("btn_export"));
    btnExport->setCursor(Qt::PointingHandCursor);
    btnExport->setMinimumHeight(36);
    btnExport->setStyleSheet(QStringLiteral(R"_(
        QPushButton#btn_export {
            background: rgba(20, 28, 40, 200);
            color: #b6c4d8;
            border: 1px solid rgba(60, 78, 102, 160);
            border-radius: 10px;
            padding: 8px 14px;
            font-size: 11.5px;
            font-weight: 600;
            letter-spacing: 0.6px;
        }
        QPushButton#btn_export:hover {
            border-color: #2f81f7;
            color: #ffffff;
        }
    )_"));
    {
        const int insertAt =
            ui->sideLayout->indexOf(btnHistory) + 1;
        ui->sideLayout->insertWidget(insertAt, btnExport);
    }
    connect(btnExport, &QPushButton::clicked,
            this, &MainWindow::exportCurrentChatAsMarkdown);

    // No standalone Stop button — the Send button morphs into Stop while
    // generating. See setGenerating().

    // ---- 4. Status / telemetry card at the very bottom -----------------------
    // We wrap the existing label_status into a polished frame + add a
    // telemetry sub-label and a re-styled "ONLINE" chip.
    if (auto* status = ui->label_status) {
        // Move the status label out of the sideLayout into our card.
        ui->sideLayout->removeWidget(status);

        auto* statusCard = new QFrame(this);
        statusCard->setObjectName("sidebarStatusCard");
        statusCard->setStyleSheet(QStringLiteral(R"_(
            QFrame#sidebarStatusCard {
                background: rgba(13, 19, 28, 230);
                border: 1px solid rgba(60, 78, 102, 130);
                border-radius: 12px;
            }
        )_"));
        auto* sLay = new QVBoxLayout(statusCard);
        sLay->setContentsMargins(14, 10, 14, 10);
        sLay->setSpacing(4);

        auto* sCaption = new QLabel(QStringLiteral("STATUS"), statusCard);
        sCaption->setStyleSheet(QStringLiteral(
            "color: #58a6ff; font-size: 9.5px; font-weight: 700; letter-spacing: 2.5px;"));

        sidebarStatusBig = new QLabel(QStringLiteral("BOOTING…"), statusCard);
        sidebarStatusBig->setStyleSheet(QStringLiteral(
            "color: #67e8a4; font-size: 14px; font-weight: 800; letter-spacing: 2.5px;"));

        // Re-style the original label_status as the "details" line.
        status->setText(QStringLiteral("All systems nominal"));
        status->setStyleSheet(QStringLiteral(
            "color: #8a99b1; font-size: 10.5px; letter-spacing: 1.2px; font-weight: 500;"));
        status->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        telemetryLabel = new QLabel(
            QStringLiteral("CORE 99.8 %   ·   NPU  12 %"), statusCard);
        telemetryLabel->setObjectName(QStringLiteral("label_telemetry"));
        telemetryLabel->setStyleSheet(QStringLiteral(
            "color: #6b7a90; font-size: 10px; letter-spacing: 1.5px; font-weight: 500;"));

        sLay->addWidget(sCaption);
        sLay->addWidget(sidebarStatusBig);
        sLay->addWidget(status);
        sLay->addWidget(telemetryLabel);

        ui->sideLayout->addWidget(statusCard);
    }

    // ---- 5. User chip (avatar + display name) at the very bottom -------------
    {
        auto* userChip = new QFrame(this);
        userChip->setObjectName(QStringLiteral("sidebarUserChip"));
        userChip->setStyleSheet(QStringLiteral(R"_(
            QFrame#sidebarUserChip {
                background: rgba(13, 19, 28, 235);
                border: 1px solid rgba(80, 130, 200, 90);
                border-radius: 14px;
            }
        )_"));
        auto* row = new QHBoxLayout(userChip);
        row->setContentsMargins(10, 8, 12, 8);
        row->setSpacing(10);

        userChipAvatar = new QLabel(userChip);
        userChipAvatar->setFixedSize(38, 38);
        userChipAvatar->setAlignment(Qt::AlignCenter);
        // Visuals are set by refreshUserChip() (initials gradient OR
        // a circular crop of the user's saved avatar).

        auto* col = new QVBoxLayout;
        col->setSpacing(2);
        col->setContentsMargins(0, 1, 0, 1);

        auto* eyebrow = new QLabel(QStringLiteral("КОРИСТУВАЧ"), userChip);
        eyebrow->setStyleSheet(QStringLiteral(
            "color: #58a6ff; font-size: 8.5px; font-weight: 700; "
            "letter-spacing: 2.5px;"));

        userChipName = new QLabel(QStringLiteral("…"), userChip);
        userChipName->setStyleSheet(QStringLiteral(
            "color: #e6edf3; font-size: 12.5px; font-weight: 700; "
            "letter-spacing: 0.4px;"));

        col->addStretch();
        col->addWidget(eyebrow);
        col->addWidget(userChipName);
        col->addStretch();

        row->addWidget(userChipAvatar, 0, Qt::AlignVCenter);
        row->addLayout(col, 1);

        ui->sideLayout->addWidget(userChip);
        refreshUserChip();
    }

    // ---- 6. Animate telemetry numbers ----------------------------------------
    auto* tTimer = new QTimer(this);
    connect(tTimer, &QTimer::timeout, this, [this]() {
        if (!telemetryLabel) return;
        const int load = 8 + (rand() % 18);
        telemetryLabel->setText(QStringLiteral("CORE 99.9 %   ·   NPU %1 %")
                                    .arg(load, 2, 10, QChar(' ')));
    });
    tTimer->start(2000);
}

// =============================================================================
//  UI preferences (Settings → live UI)
// =============================================================================

void MainWindow::applyUiPreferences(const SettingsDialog& dlg) {
    // Accent recolors the aurora background.
    if (particleBg) particleBg->setAccentColor(dlg.getAccentColor());

    // Window translucency. Anything < 1.0 enables WA_TranslucentBackground
    // semantics; setWindowOpacity is the cleanest path on top-level windows.
    setWindowOpacity(qBound(0.7, dlg.getWindowOpacity(), 1.0));

    // Toggle the per-message timestamp globally.
    MessageWidget::setShowTimestamps(dlg.showTimestamps());
}

// Pulls every persisted setting from QSettings and applies it. Called once
// from the constructor *after* the UI is built but *before* the model load.
// We avoid showing the SettingsDialog visually here.
void MainWindow::applyPersistedPreferences() {
    QSettings s;

    // ---- Sampling / model params ----
    LlamaWorkerThread::GenParams gen;
    if (s.contains(QStringLiteral("settings/temperature")))
        gen.temperature = static_cast<float>(s.value("settings/temperature").toDouble());
    if (s.contains(QStringLiteral("settings/contextSize")))
        gen.contextSize = s.value("settings/contextSize").toInt();
    if (s.contains(QStringLiteral("settings/topP")))
        gen.topP = static_cast<float>(s.value("settings/topP").toDouble());
    if (s.contains(QStringLiteral("settings/topK")))
        gen.topK = s.value("settings/topK").toInt();
    if (s.contains(QStringLiteral("settings/minP")))
        gen.minP = static_cast<float>(s.value("settings/minP").toDouble());
    if (s.contains(QStringLiteral("settings/repeatPenalty")))
        gen.repeatPenalty = static_cast<float>(s.value("settings/repeatPenalty").toDouble());
    if (s.contains(QStringLiteral("settings/maxTokens")))
        gen.maxTokens = s.value("settings/maxTokens").toInt();
    if (s.contains(QStringLiteral("settings/gpuLayers")))
        gen.gpuLayers = s.value("settings/gpuLayers").toInt();
    if (s.contains(QStringLiteral("settings/promptTemplate"))) {
        gen.promptTemplate = static_cast<LlamaWorkerThread::PromptTemplate>(
            s.value("settings/promptTemplate").toInt());
    }
    if (aiThread) aiThread->setGenParams(gen);

    if (s.contains(QStringLiteral("settings/systemPrompt"))) {
        const QString prompt = s.value("settings/systemPrompt").toString();
        if (!prompt.isEmpty() && prompt != Config::SYSTEM_PROMPT && aiThread)
            aiThread->setSystemPromptOverride(prompt);
    }

    // ---- Visual prefs ----
    if (particleBg && s.contains(QStringLiteral("settings/accentRgb"))) {
        particleBg->setAccentColor(QColor::fromRgb(
            static_cast<QRgb>(s.value("settings/accentRgb").toUInt())));
    }
    if (s.contains(QStringLiteral("settings/opacityPct"))) {
        const int pct = s.value("settings/opacityPct").toInt();
        setWindowOpacity(qBound(0.7, pct / 100.0, 1.0));
    }
    if (s.contains(QStringLiteral("settings/showTimestamps"))) {
        MessageWidget::setShowTimestamps(s.value("settings/showTimestamps").toBool());
    }

    // ---- Model selection ----
    QString modelToLoad;
    if (s.contains(QStringLiteral("settings/modelPath"))) {
        const QString p = s.value("settings/modelPath").toString();
        if (!p.isEmpty() && QFileInfo::exists(p)) modelToLoad = p;
    }
    if (modelToLoad.isEmpty()) modelToLoad = defaultModelPath();
    if (aiThread && !modelToLoad.isEmpty()) {
        m_lastModelPath = modelToLoad;
        aiThread->queueLoadModel(modelToLoad);
    }
}

// Flip the Send button between idle (▶) and generating (◼). While generating,
// clicks are routed to LlamaWorkerThread::stopGeneration() in onUserInput().
void MainWindow::setGenerating(bool generating) {
    m_generating = generating;
    if (!sendButton) return;

    if (generating) {
        sendButton->setText(QStringLiteral("◼"));
        sendButton->setToolTip(QStringLiteral("Зупинити генерацію"));
        sendButton->setStyleSheet(QStringLiteral(R"_(
            QPushButton#sendButton {
                background-color: qlineargradient(
                    x1:0, y1:0, x2:1, y2:1,
                    stop:0 #f43f5e, stop:0.55 #d92c4a, stop:1 #b41f3a);
                color: #ffffff;
                border: none;
                border-radius: 23px;
                font-size: 15px;
                font-weight: 800;
            }
            QPushButton#sendButton:hover {
                background-color: qlineargradient(
                    x1:0, y1:0, x2:1, y2:1,
                    stop:0 #ff6b87, stop:0.55 #f43f5e, stop:1 #d92c4a);
            }
        )_"));
        if (sendShadow) sendShadow->setColor(QColor(244, 63, 94, 180));
    } else {
        sendButton->setText(QStringLiteral("▶"));
        sendButton->setToolTip(QStringLiteral("Надіслати"));
        sendButton->setStyleSheet(QStringLiteral(R"_(
            QPushButton#sendButton {
                background-color: qlineargradient(
                    x1:0, y1:0, x2:1, y2:1,
                    stop:0 #58a6ff, stop:0.55 #2f81f7, stop:1 #1d6def);
                color: #ffffff;
                border: none;
                border-radius: 23px;
                font-size: 15px;
                font-weight: 700;
                padding-left: 2px;
            }
            QPushButton#sendButton:hover {
                background-color: qlineargradient(
                    x1:0, y1:0, x2:1, y2:1,
                    stop:0 #79bbff, stop:0.55 #4593f9, stop:1 #2f81f7);
            }
            QPushButton#sendButton:pressed {
                background-color: qlineargradient(
                    x1:0, y1:0, x2:1, y2:1,
                    stop:0 #2f81f7, stop:1 #1158c7);
            }
        )_"));
        if (sendShadow) sendShadow->setColor(QColor(31, 110, 235, 110));
    }
}

void MainWindow::refreshUserChip() {
    if (!userChipName) return;
    QString name = MessageWidget::userDisplayName();
    if (name.isEmpty()) name = QStringLiteral("YOU");
    userChipName->setText(name);

    if (!userChipAvatar) return;

    const int side = userChipAvatar->width();
    const int radius = side / 2;

    // Prefer the user-supplied avatar. Round-mask + halo + ring are baked
    // into the bitmap (see WelcomeDialog::roundAvatar), so the QLabel
    // itself renders nothing extra — no square border bleeding outside
    // the circle.
    QPixmap pix = WelcomeDialog::savedAvatar(side);
    if (!pix.isNull()) {
        userChipAvatar->setPixmap(WelcomeDialog::roundAvatar(pix, side));
        userChipAvatar->setText(QString());
        userChipAvatar->setStyleSheet(QStringLiteral(
            "background: transparent; border: none;"));
    } else {
        userChipAvatar->setPixmap(QPixmap());
        userChipAvatar->setText(name.left(1).toUpper());
        userChipAvatar->setStyleSheet(QStringLiteral(
            "color: #ffffff; font-size: 14px; font-weight: 800; "
            "letter-spacing: 1px; "
            "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, "
            "  stop:0 #2f81f7, stop:1 #1d6def); "
            "border: 1px solid rgba(255,255,255,40); "
            "border-radius: %1px;").arg(radius));
    }
}

// =============================================================================
//  Chat helpers
// =============================================================================

bool MainWindow::isNearBottom() const {
    if (!scrollArea) return true;
    auto* bar = scrollArea->verticalScrollBar();
    if (!bar) return true;
    return (bar->maximum() - bar->value()) <= kStickyScrollThresholdPx;
}

void MainWindow::scrollToBottom() {
    if (!scrollArea) return;
    auto* bar = scrollArea->verticalScrollBar();
    if (!bar) return;
    bar->setValue(bar->maximum());
}

void MainWindow::addMessage(const QString& text, bool isUser) {
    auto* msg = new MessageWidget(text, isUser, this);
    chatLayout->insertWidget(chatLayout->count() - 1, msg);
    QTimer::singleShot(0, this, [this]() { scrollToBottom(); });
}

// =============================================================================
//  Chat history persistence
// =============================================================================

void MainWindow::clearChatLayout() {
    if (!chatLayout) return;
    QLayoutItem* child = nullptr;
    while ((child = chatLayout->takeAt(0)) != nullptr) {
        if (child->widget()) delete child->widget();
        delete child;
    }
    chatLayout->addStretch();
}

void MainWindow::newChat(bool persistOld) {
    if (persistOld && !m_currentMessages.isEmpty()) {
        saveCurrentChat();
    }

    if (aiThread) aiThread->clearHistory();
    clearChatLayout();
    m_currentMessages.clear();
    m_currentAiText.clear();
    currentAiBubble    = nullptr;
    m_currentChatId    = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_currentChatTitle.clear();
}

void MainWindow::appendUserMessage(const QString& text) {
    if (m_currentChatId.isEmpty()) {
        m_currentChatId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    addMessage(text, /*isUser=*/true);
    m_currentMessages.push_back({true, text, QDateTime::currentDateTime()});

    // Use the first user message as the chat title.
    if (m_currentChatTitle.isEmpty()) {
        QString trimmed = text.simplified();
        if (trimmed.length() > 60) trimmed = trimmed.left(58) + QStringLiteral("…");
        m_currentChatTitle = trimmed;
    }
    saveCurrentChat();
}

void MainWindow::appendAiMessage(const QString& text) {
    // The bubble is already on screen — we only persist the text for history.
    m_currentMessages.push_back({false, text, QDateTime::currentDateTime()});
    saveCurrentChat();
}

void MainWindow::saveCurrentChat() const {
    if (m_currentChatId.isEmpty() || m_currentMessages.isEmpty()) return;

    QJsonArray msgs;
    for (const StoredMessage& m : m_currentMessages) {
        QJsonObject o;
        o.insert(QStringLiteral("role"),
                 m.isUser ? QStringLiteral("user") : QStringLiteral("ai"));
        o.insert(QStringLiteral("text"), m.text);
        o.insert(QStringLiteral("time"), m.time.toString(Qt::ISODate));
        msgs.append(o);
    }

    QJsonObject root;
    root.insert(QStringLiteral("title"),
                m_currentChatTitle.isEmpty()
                    ? QStringLiteral("Без назви") : m_currentChatTitle);
    root.insert(QStringLiteral("updatedAt"),
                QDateTime::currentDateTime().toString(Qt::ISODate));
    root.insert(QStringLiteral("messages"), msgs);

    QSaveFile file(ChatHistoryDialog::chatFilePath(m_currentChatId));
    if (!file.open(QIODevice::WriteOnly)) return;
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.commit();
}

void MainWindow::loadChatById(const QString& id) {
    QFile f(ChatHistoryDialog::chatFilePath(id));
    if (!f.open(QIODevice::ReadOnly)) return;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    f.close();
    if (!doc.isObject()) return;

    // Persist whatever is currently on screen first, then swap.
    if (!m_currentMessages.isEmpty() && m_currentChatId != id) {
        saveCurrentChat();
    }

    if (aiThread) aiThread->clearHistory();
    clearChatLayout();
    m_currentMessages.clear();
    m_currentAiText.clear();
    currentAiBubble = nullptr;

    const QJsonObject root = doc.object();
    m_currentChatId    = id;
    m_currentChatTitle = root.value(QStringLiteral("title")).toString();

    const QJsonArray msgs = root.value(QStringLiteral("messages")).toArray();
    for (const QJsonValue& v : msgs) {
        const QJsonObject m = v.toObject();
        const bool isUser = m.value(QStringLiteral("role")).toString() ==
                            QStringLiteral("user");
        const QString text = m.value(QStringLiteral("text")).toString();
        addMessage(text, isUser);
        m_currentMessages.push_back({
            isUser, text,
            QDateTime::fromString(m.value(QStringLiteral("time")).toString(),
                                  Qt::ISODate)
        });
    }
}

void MainWindow::openChatHistoryDialog() {
    // Make sure the *current* chat is on disk before showing the dialog so
    // the user sees an up-to-date list.
    saveCurrentChat();

    ChatHistoryDialog dlg(m_currentChatId, this);
    if (dlg.exec() != QDialog::Accepted) return;

    switch (dlg.resultAction()) {
    case ChatHistoryDialog::Action::Open:
        if (!dlg.selectedChatId().isEmpty()
            && dlg.selectedChatId() != m_currentChatId) {
            loadChatById(dlg.selectedChatId());
        }
        break;
    case ChatHistoryDialog::Action::NewChat:
        newChat(/*persistOld=*/true);
        break;
    case ChatHistoryDialog::Action::None:
    default:
        break;
    }
}

// =============================================================================
//  Slots
// =============================================================================

void MainWindow::onUserInput() {
    // While generating, the Send button doubles as a Stop button.
    if (m_generating) {
        if (aiThread) aiThread->stopGeneration();
        return;
    }

    const QString text = inputField->toPlainText().trimmed();
    if (text.isEmpty()) return;

    appendUserMessage(text);
    inputField->clear();

    currentAiBubble = new MessageWidget(QString(), /*isUser=*/false, this);
    chatLayout->insertWidget(chatLayout->count() - 1, currentAiBubble);
    m_currentAiText.clear();

    aiThread->queuePrompt(Config::SYSTEM_PROMPT, text);
    setGenerating(true);
    scrollToBottom();
}

void MainWindow::updateAiStream(const QString& token) {
    if (!currentAiBubble) return;

    const bool keepPinned = isNearBottom();
    currentAiBubble->appendText(token);
    m_currentAiText.append(token);
    if (keepPinned) {
        scrollToBottom();
    }
}

void MainWindow::onReplyFinished(const QString& fullResponse) {
    // Strip the streaming caret and run markdown -> HTML on the visible
    // bubble before we drop our handle on it.
    if (currentAiBubble) currentAiBubble->finalize();

    static const QRegularExpression cmdRegex(
        QStringLiteral(R"(\[(cmd|ps)\s*:\s*([^\]]+)\])"),
        QRegularExpression::CaseInsensitiveOption);

    QRegularExpressionMatchIterator it = cmdRegex.globalMatch(fullResponse);
    while (it.hasNext()) {
        const auto match    = it.next();
        const QString shell = match.captured(1).toLower();
        const QString cmd   = match.captured(2).trimmed();
        if (cmd.isEmpty()) continue;
        handleSystemCommand(cmd, shell == QLatin1String("ps"));
    }

    // Persist the full visible AI response into the current chat record.
    const QString trimmedAi = m_currentAiText.trimmed();
    if (!trimmedAi.isEmpty()) {
        appendAiMessage(trimmedAi);
    }
    m_currentAiText.clear();
    currentAiBubble = nullptr;

    // If this reply was kicked off by a phone via /api/chat, finish the
    // HTTP response now (the socket has been parked since the request
    // arrived). Strip the [CMD:..]/[PS:..] tags so the phone gets clean text.
    if (m_webChatPending && httpServer) {
        m_webChatPending = false;
        QString clean = fullResponse;
        static const QRegularExpression tagsRe(
            QStringLiteral(R"(\[(?:cmd|ps)\s*:\s*[^\]]+\])"),
            QRegularExpression::CaseInsensitiveOption);
        clean.remove(tagsRe);
        clean = clean.trimmed();
        httpServer->completeWebChat(clean.isEmpty() ? trimmedAi : clean);
    }
}

// =============================================================================
//  System command dispatch
// =============================================================================

void MainWindow::handleSystemCommand(const QString& shellCmd, bool isPowerShell) {
    const QString trimmed = shellCmd.trimmed();
    if (trimmed.isEmpty()) return;

    // [PS: ...] — capture output. PowerShell resolves Start-Process /
    // Stop-Process / taskkill / Get-* / etc. naturally.
    if (isPowerShell) {
        runCapturedShell(QStringLiteral("powershell.exe"),
                         {QStringLiteral("-NoProfile"),
                          QStringLiteral("-WindowStyle"), QStringLiteral("Hidden"),
                          QStringLiteral("-Command"), trimmed},
                         QStringLiteral("PS> ") + trimmed);
        return;
    }

    // [CMD: ...] — try to be smart before falling back to cmd.exe.

    // Pattern: open / start / launch / run <"target with spaces"|target> [extra args]
    static const QRegularExpression openRe(
        QStringLiteral(R"_(^\s*(?:start|open|launch|run|exec)\b\s*(?:""\s+)?(?:"([^"]+)"|(\S+))\s*(.*)$)_"),
        QRegularExpression::CaseInsensitiveOption);

    // Pattern: close / kill / stop <name>
    static const QRegularExpression closeRe(
        QStringLiteral(R"_(^\s*(?:close|kill|stop|terminate)\b\s+(?:"([^"]+)"|(\S+))\s*$)_"),
        QRegularExpression::CaseInsensitiveOption);

    // Pattern: bare known-app alias (e.g. just "chrome", "discord")
    static const QRegularExpression bareRe(
        QStringLiteral(R"_(^\s*([A-Za-z][A-Za-z0-9+\-_.]*)\s*$)_"));

    // ---- 1. open/start/launch ----
    if (auto m = openRe.match(trimmed); m.hasMatch()) {
        const QString target = !m.captured(1).isEmpty() ? m.captured(1) : m.captured(2);
        const QString tail   = m.captured(3).trimmed();
        QStringList tailArgs = tail.isEmpty() ? QStringList() : QProcess::splitCommand(tail);

        if (tryOpenUrlOrFile(target)) {
            appendSystemBubble(QStringLiteral("✓ Відкрито: %1").arg(target));
            return;
        }
        if (tryLaunchKnownApp(target, tailArgs)) {
            appendSystemBubble(QStringLiteral("✓ Запущено: %1").arg(target));
            return;
        }

        // Fallback: hand off to PowerShell's `Start-Process` so the App Paths
        // registry can locate exotic apps. Run the PS itself hidden so no
        // console window flashes.
        QString psTarget = target;
        psTarget.replace(QLatin1Char('\''), QStringLiteral("''"));
        QString launch = QStringLiteral("Start-Process -FilePath '%1'").arg(psTarget);
        if (!tail.isEmpty()) {
            QString psTail = tail;
            psTail.replace(QLatin1Char('\''), QStringLiteral("''"));
            launch += QStringLiteral(" -ArgumentList '%1'").arg(psTail);
        }
        runHiddenPowerShell(launch);
        appendSystemBubble(QStringLiteral("→ Спроба запуску через PowerShell: %1").arg(target));
        return;
    }

    // ---- 2. close/kill/stop ----
    if (auto m = closeRe.match(trimmed); m.hasMatch()) {
        const QString name = !m.captured(1).isEmpty() ? m.captured(1) : m.captured(2);
        if (tryCloseProcess(name)) {
            appendSystemBubble(QStringLiteral("✓ Закрито: %1").arg(name));
            return;
        }
        appendSystemBubble(QStringLiteral("⚠ Не вдалося закрити: %1").arg(name));
        return;
    }

    // ---- 3. bare alias (e.g. just "chrome") ----
    if (auto m = bareRe.match(trimmed); m.hasMatch()) {
        const QString alias = m.captured(1);
        if (tryOpenUrlOrFile(alias)) {
            appendSystemBubble(QStringLiteral("✓ Відкрито: %1").arg(alias));
            return;
        }
        if (tryLaunchKnownApp(alias, /*extra=*/{})) {
            appendSystemBubble(QStringLiteral("✓ Запущено: %1").arg(alias));
            return;
        }
    }

    // ---- 4. Already a taskkill / Stop-Process / Get-Process / etc. ----
    //    Run via hidden PowerShell with output capture so the user sees
    //    a result.
    static const QRegularExpression silentRe(
        QStringLiteral(R"_(^\s*(taskkill|tskill|stop-process|get-process|start-process)\b)_"),
        QRegularExpression::CaseInsensitiveOption);
    if (silentRe.match(trimmed).hasMatch()) {
        runCapturedShell(QStringLiteral("powershell.exe"),
                         {QStringLiteral("-NoProfile"),
                          QStringLiteral("-WindowStyle"), QStringLiteral("Hidden"),
                          QStringLiteral("-Command"), trimmed},
                         QStringLiteral("> ") + trimmed);
        return;
    }

    // ---- 5. Fallback: classic cmd /c, but capture so the user sees output
    //    (ipconfig, dir, netstat, etc.).
    runCapturedShell(QStringLiteral("cmd.exe"),
                     {QStringLiteral("/c"), trimmed},
                     QStringLiteral("> ") + trimmed);
}

bool MainWindow::tryOpenUrlOrFile(const QString& target) {
    // Explicit URL
    if (looksLikeUrl(target)) {
        QUrl url = target.contains(QStringLiteral("://"))
                       ? QUrl(target)
                       : QUrl(QStringLiteral("https://") + target);
        if (url.isValid()) {
            return QDesktopServices::openUrl(url);
        }
    }

    // Existing local file / folder
    const QString expanded = expandWindowsEnv(target);
    QFileInfo fi(expanded);
    if (fi.exists()) {
        if (fi.isDir() || !fi.suffix().compare(QStringLiteral("exe"), Qt::CaseInsensitive)) {
            // For dirs and non-exe files, hand off to the shell association.
            return QDesktopServices::openUrl(QUrl::fromLocalFile(fi.absoluteFilePath()));
        }
        // Executable — launch directly so we control the working dir.
        return QProcess::startDetached(fi.absoluteFilePath(), {}, fi.absolutePath());
    }
    return false;
}

bool MainWindow::tryLaunchKnownApp(const QString& alias, const QStringList& extraArgs) {
    auto resolved = resolveKnownApp(alias);
    if (!resolved) return false;

    QStringList finalArgs = resolved->args;
    finalArgs += extraArgs;

    if (QFileInfo::exists(resolved->program)) {
        const QString workingDir = QFileInfo(resolved->program).absolutePath();
        return QProcess::startDetached(resolved->program, finalArgs, workingDir);
    }

    // Path didn't exist — let PowerShell try via App Paths.
    QString psArgs;
    if (!finalArgs.isEmpty()) {
        QString joined;
        for (QString a : finalArgs) {
            a.replace(QLatin1Char('\''), QStringLiteral("''"));
            if (!joined.isEmpty()) joined += QLatin1Char(',');
            joined += QLatin1Char('\'') + a + QLatin1Char('\'');
        }
        psArgs = QStringLiteral(" -ArgumentList %1").arg(joined);
    }
    QString fallbackName = alias;
    fallbackName.replace(QLatin1Char('\''), QStringLiteral("''"));
    runHiddenPowerShell(QStringLiteral("Start-Process -FilePath '%1'%2")
                            .arg(fallbackName, psArgs));
    return true;
}

bool MainWindow::tryCloseProcess(const QString& aliasIn) {
    QString name = aliasIn.trimmed();
    if (name.isEmpty()) return false;

    // If we have a known-app entry, prefer the executable's basename
    // (e.g. "discord" -> "Discord.exe") for a more reliable match.
    if (auto resolved = resolveKnownApp(name)) {
        const QString exeName = QFileInfo(resolved->program).fileName();
        if (!exeName.isEmpty()) name = exeName;
    }

    if (!name.endsWith(QStringLiteral(".exe"), Qt::CaseInsensitive)) {
        name += QStringLiteral(".exe");
    }

    QString safe = name;
    safe.replace(QLatin1Char('\''), QStringLiteral("''"));
    runHiddenPowerShell(
        QStringLiteral("taskkill /F /IM '%1' 2>$null; "
                       "if ($LASTEXITCODE -ne 0) { "
                       "  Get-Process -ErrorAction SilentlyContinue | "
                       "    Where-Object { $_.ProcessName -ieq '%2' } | "
                       "    Stop-Process -Force -ErrorAction SilentlyContinue }")
            .arg(safe, QFileInfo(safe).completeBaseName()));
    return true;
}

void MainWindow::runHiddenPowerShell(const QString& cmdLine) const {
    QProcess::startDetached(QStringLiteral("powershell.exe"), {
        QStringLiteral("-NoProfile"),
        QStringLiteral("-WindowStyle"), QStringLiteral("Hidden"),
        QStringLiteral("-Command"), cmdLine,
    });
}

// Run a shell command WITH stdout/stderr capture and emit a system bubble
// when it finishes. The QProcess auto-deletes itself on completion.
void MainWindow::runCapturedShell(const QString& program,
                                  const QStringList& args,
                                  const QString& label)
{
    auto* proc = new QProcess(this);
    proc->setProcessChannelMode(QProcess::MergedChannels);
#ifdef Q_OS_WIN
    proc->setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments* a) {
        // CREATE_NO_WINDOW = 0x08000000 — suppresses the black console flash.
        a->flags |= 0x08000000;
    });
#endif

    appendSystemBubble(label);

    connect(proc,
        QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
        this,
        [this, proc, label](int exitCode, QProcess::ExitStatus /*status*/) {
            QString out = QString::fromLocal8Bit(proc->readAllStandardOutput());
            // Trim ANSI / trailing whitespace and clamp to a sensible chunk.
            out = out.trimmed();
            const int kMaxOut = 2000;
            if (out.size() > kMaxOut) {
                out = out.left(kMaxOut) + QStringLiteral("\n…(обрізано)");
            }
            if (out.isEmpty()) {
                appendSystemBubble(QStringLiteral("✓ Готово (код %1)").arg(exitCode));
            } else {
                appendSystemBubble(QStringLiteral("Вивід (код %1):\n```\n%2\n```")
                                       .arg(QString::number(exitCode), out));
            }
            proc->deleteLater();
        });
    connect(proc, &QProcess::errorOccurred, this, [this, proc](QProcess::ProcessError) {
        appendSystemBubble(QStringLiteral("⚠ Помилка процесу: %1")
                               .arg(proc->errorString()));
        proc->deleteLater();
    });

    proc->start(program, args);
}

// Push a small grey "system" bubble into the chat log. Persisted into the
// current chat record so re-opening the chat shows the same status line.
void MainWindow::appendSystemBubble(const QString& text) {
    if (!chatLayout) return;
    auto* w = new MessageWidget(text, MessageWidget::Kind::System, this);
    // chatLayout always ends with addStretch() — insert before that.
    chatLayout->insertWidget(chatLayout->count() - 1, w);
    if (isNearBottom()) scrollToBottom();
    // Do NOT persist into m_currentMessages — system pills are ephemeral.
}

// =============================================================================
//  Chat export
// =============================================================================

void MainWindow::exportCurrentChatAsMarkdown() {
    if (m_currentMessages.isEmpty()) return;

    const QString defaultName = QStringLiteral("jarvis-chat-%1.md")
        .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss")));
    const QString suggested = QDir(QStandardPaths::writableLocation(
                                      QStandardPaths::DocumentsLocation))
                                  .filePath(defaultName);

    const QString file = QFileDialog::getSaveFileName(
        this, QStringLiteral("Експорт чату"), suggested,
        QStringLiteral("Markdown (*.md);;Текст (*.txt);;Усі файли (*.*)"));
    if (file.isEmpty()) return;

    QString out;
    out += QStringLiteral("# %1\n\n")
               .arg(m_currentChatTitle.isEmpty()
                        ? QStringLiteral("Чат JARVIS") : m_currentChatTitle);
    out += QStringLiteral("_Експортовано: %1_\n\n---\n\n")
               .arg(QDateTime::currentDateTime().toString(Qt::ISODate));
    for (const StoredMessage& m : m_currentMessages) {
        out += QStringLiteral("**%1** _(%2)_\n\n%3\n\n")
                   .arg(m.isUser ? QStringLiteral("Ти") : QStringLiteral("JARVIS"),
                        m.time.toString(QStringLiteral("yyyy-MM-dd HH:mm")),
                        m.text);
    }

    QFile f(file);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        appendSystemBubble(QStringLiteral("⚠ Не вдалося відкрити файл для запису"));
        return;
    }
    f.write(out.toUtf8());
    f.close();
    appendSystemBubble(QStringLiteral("✓ Чат збережено у %1").arg(file));
}

// =============================================================================
//  System tray
// =============================================================================

void MainWindow::setupTrayIcon() {
    if (!QSystemTrayIcon::isSystemTrayAvailable()) return;

    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(windowIcon().isNull() ? QIcon(QStringLiteral(":/assets/icon.png"))
                                            : windowIcon());
    trayIcon->setToolTip(QStringLiteral("JARVIS — Personal AI Core"));

    auto* menu = new QMenu(this);
    auto* showAct = menu->addAction(QStringLiteral("Показати JARVIS"));
    auto* hideAct = menu->addAction(QStringLiteral("Сховати"));
    menu->addSeparator();
    auto* quitAct = menu->addAction(QStringLiteral("Вийти"));

    connect(showAct, &QAction::triggered, this, [this]() {
        showNormal();
        raise();
        activateWindow();
    });
    connect(hideAct, &QAction::triggered, this, &QWidget::hide);
    connect(quitAct, &QAction::triggered, this, [this]() {
        m_quitting = true;
        QCoreApplication::quit();
    });

    trayIcon->setContextMenu(menu);

    connect(trayIcon, &QSystemTrayIcon::activated,
            this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger ||
            reason == QSystemTrayIcon::DoubleClick) {
            if (isVisible()) hide();
            else { showNormal(); raise(); activateWindow(); }
        }
    });
    trayIcon->show();
}

// =============================================================================
//  LAN web server (phone control panel)
// =============================================================================

void MainWindow::applyServerPreferences() {
    QSettings s;
    const bool    enabled = s.value(QStringLiteral("server/enabled"),
                                    false).toBool();
    const quint16 port    = static_cast<quint16>(
                                s.value(QStringLiteral("server/port"),
                                        17320).toInt());
    const QString pin     = s.value(QStringLiteral("server/pin")).toString();

    if (!enabled) {
        if (httpServer) {
            httpServer->stop();
            httpServer->deleteLater();
            httpServer = nullptr;
        }
        return;
    }

    // Lazy-create on first enable, then reconfigure on later edits. Using
    // restart() == stop+listen guarantees we re-bind the new port if the
    // user changed it from Settings.
    if (!httpServer) {
        httpServer = new JarvisHttpServer(this);
        connect(httpServer, &JarvisHttpServer::webCommandRequested,
                this, &MainWindow::onWebCommandRequested);
        connect(httpServer, &JarvisHttpServer::webChatRequested,
                this, &MainWindow::onWebChatRequested);
        // Phone edited the quick-action grid — persist the new layout so it
        // survives restart and stays consistent across all connected clients.
        connect(httpServer, &JarvisHttpServer::buttonsChanged,
                this, [](const QByteArray& json) {
            QSettings s2;
            s2.setValue(QStringLiteral("server/buttons"),
                        QString::fromUtf8(json));
        });
    }
    httpServer->setStatusProvider(
        aiThread,
        s.value(QStringLiteral("user/name")).toString());
    httpServer->setPin(pin);
    // Hydrate the in-memory button set from QSettings (or seed with defaults
    // on first run). The setter validates the JSON, so a corrupt value
    // gracefully falls back to the built-in layout.
    const QByteArray savedButtons =
        s.value(QStringLiteral("server/buttons")).toString().toUtf8();
    httpServer->setButtonsJson(
        savedButtons.isEmpty()
            ? JarvisHttpServer::defaultButtonsJson()
            : savedButtons);
    if (!httpServer->start(port)) {
        appendSystemBubble(QStringLiteral(
            "⚠ Не вдалося запустити веб-сервер на порту %1 — імовірно зайнятий.")
            .arg(port));
        delete httpServer;
        httpServer = nullptr;
        // Reflect the failure back into QSettings so the toggle on the
        // next Settings open is honest about state.
        s.setValue(QStringLiteral("server/enabled"), false);
        return;
    }
    appendSystemBubble(QStringLiteral(
        "✓ JARVIS-сервер запущено на порту %1. URL для телефону: %2")
        .arg(port)
        .arg(JarvisHttpServer::lanUrls(port).join(QStringLiteral(", "))));
}

void MainWindow::onWebChatRequested(const QString& message) {
    if (!aiThread) {
        if (httpServer) httpServer->failWebChat(503, QStringLiteral("AI offline"));
        return;
    }
    if (m_generating || aiThread->isBusy()) {
        if (httpServer)
            httpServer->failWebChat(503,
                QStringLiteral("Зайнято — JARVIS зараз відповідає."));
        return;
    }
    // Surface the phone's question on the desktop so the user sees it too.
    appendUserMessage(message);
    currentAiBubble = new MessageWidget(QString(), /*isUser=*/false, this);
    if (chatLayout) {
        const int insertAt = chatLayout->count() - 1;
        chatLayout->insertWidget(qMax(0, insertAt), currentAiBubble);
    }
    m_currentAiText.clear();
    m_webChatPending = true;
    setGenerating(true);
    aiThread->queuePrompt(Config::SYSTEM_PROMPT, message);
}

void MainWindow::onWebCommandRequested(const QString& cmd, bool isPowerShell) {
    handleSystemCommand(cmd, isPowerShell);
}

// =============================================================================
//  Misc
// =============================================================================

void MainWindow::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_F11) {
        if (isFullScreen()) showNormal();
        else                showFullScreen();
        return;
    }
    QMainWindow::keyPressEvent(event);
}

void MainWindow::closeEvent(QCloseEvent* event) {
    // First close while the tray is up: minimize-to-tray and notify.
    if (trayIcon && trayIcon->isVisible() && !m_quitting) {
        QSettings s;
        const bool warned = s.value(QStringLiteral("ui/trayWarned")).toBool();
        if (!warned) {
            trayIcon->showMessage(
                QStringLiteral("JARVIS"),
                QStringLiteral("Працюю у треї. Подвійний клік по іконці — відкрити; "
                               "правий клік — меню."),
                QSystemTrayIcon::Information, 4000);
            s.setValue(QStringLiteral("ui/trayWarned"), true);
        }
        hide();
        event->ignore();
        return;
    }
    QMainWindow::closeEvent(event);
}
