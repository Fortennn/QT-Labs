#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "ai/SystemPrompt.h"
#include "ui/SettingsDialog.h"
#include "widgets/BrainVisualizer.h"
#include "widgets/ParticleBackground.h"
#include "widgets/MessageWidget.h"

#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QProcess>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionMatchIterator>
#include <QScrollArea>
#include <QScrollBar>
#include <QStackedLayout>
#include <QThread>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

namespace {

constexpr int kStickyScrollThresholdPx = 80; // user is "near bottom" within this many px

QGraphicsDropShadowEffect* makeSoftShadow(QObject* parent,
                                          int blur = 24,
                                          int dy   = 4,
                                          int alpha = 160) {
    auto* shadow = new QGraphicsDropShadowEffect(parent);
    shadow->setBlurRadius(blur);
    shadow->setOffset(0, dy);
    shadow->setColor(QColor(0, 0, 0, alpha));
    return shadow;
}

} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Premium dark window-level palette / typography
    applyPremiumStyles();

    // 1. Brain visualizer in the side panel
    auto* brainLayout = new QVBoxLayout(ui->brainContainer);
    auto* brain = new BrainVisualizer(this);
    brainLayout->addWidget(brain);
    brainLayout->setContentsMargins(0, 0, 0, 0);

    // 2. Build the chat surface
    setupDynamicUi();

    // Telemetry under the side buttons
    auto* telemetryLabel = new QLabel(
        QStringLiteral("CORE STABILITY: 99.8%\nNPU LOAD: 12%\nNEURAL SYNC: ACTIVE"), this);
    telemetryLabel->setStyleSheet("color: #6e7681; font-size: 10px; margin-top: 10px; letter-spacing: 1px;");
    ui->sideLayout->insertWidget(ui->sideLayout->count() - 2, telemetryLabel);

    auto* tTimer = new QTimer(this);
    connect(tTimer, &QTimer::timeout, this, [telemetryLabel]() {
        const int load = 10 + (rand() % 15);
        telemetryLabel->setText(
            QStringLiteral("CORE STABILITY: 99.9%\nNPU LOAD: %1%\nNEURAL SYNC: ACTIVE").arg(load));
    });
    tTimer->start(2000);

    // 3. AI worker thread
    aiThread = new LlamaWorkerThread(this);
    aiThread->setParams(m_temperature, m_contextSize);

    connect(ui->btn_clear, &QPushButton::clicked, this, [this]() {
        aiThread->clearHistory();
        QLayoutItem* child;
        while ((child = chatLayout->takeAt(0)) != nullptr) {
            if (child->widget()) delete child->widget();
            delete child;
        }
        chatLayout->addStretch();
    });

    connect(ui->btn_settings, &QPushButton::clicked, this, &MainWindow::openSettings);

    connect(aiThread, &LlamaWorkerThread::tokenGenerated,
            this, &MainWindow::updateAiStream, Qt::QueuedConnection);
    connect(aiThread, &LlamaWorkerThread::replyFinished,
            this, &MainWindow::onReplyFinished, Qt::QueuedConnection);

    connect(aiThread, &LlamaWorkerThread::errorOccurred, this, [this](const QString& err) {
        addMessage("ERROR: " + err, false);
    }, Qt::QueuedConnection);

    connect(aiThread, &LlamaWorkerThread::modelLoaded, this, [this](bool success) {
        if (success) {
            addMessage(QStringLiteral("JARVIS CORE ONLINE. Чим можу допомогти?"), false);
        } else {
            addMessage(QStringLiteral("JARVIS CORE OFFLINE — open Settings to load a .gguf model."), false);
        }
    }, Qt::QueuedConnection);

    aiThread->setStackSize(16 * 1024 * 1024);
    aiThread->start();

    // No hardcoded model path — the user picks one through SettingsDialog.
    addMessage(QStringLiteral("Welcome. Open Settings → Browse to load a .gguf model."), false);
}

MainWindow::~MainWindow() {
    if (aiThread) {
        aiThread->stopGeneration();
        aiThread->quit();
        aiThread->wait();
    }
    delete ui;
}

// ---------------------------------------------------------------------------
//  UI construction
// ---------------------------------------------------------------------------

void MainWindow::applyPremiumStyles() {
    // Premium "obsidian / graphite" tones, modern typography.
    setStyleSheet(R"(
        QMainWindow { background-color: #0a0d12; }
        QWidget {
            color: #e6edf3;
            font-family: 'Segoe UI', 'Inter', 'SF Pro Display', sans-serif;
            font-size: 13px;
        }
        QFrame#sidePanel {
            background-color: #0d1117;
            border-right: 1px solid #1c2128;
        }
        QFrame#sidePanel QPushButton {
            background-color: #161b22;
            color: #c9d1d9;
            border: 1px solid #21262d;
            border-radius: 10px;
            padding: 12px 14px;
            font-size: 12px;
            font-weight: 600;
            text-transform: uppercase;
            letter-spacing: 2px;
        }
        QFrame#sidePanel QPushButton:hover {
            border-color: #30363d;
            background-color: #1c2128;
            color: #ffffff;
        }
        QFrame#sidePanel QPushButton:pressed { background-color: #21262d; }
        QFrame#sidePanel QLabel {
            color: #8b949e;
            font-family: 'Segoe UI Semibold', 'Inter', sans-serif;
            letter-spacing: 1px;
        }
        QFrame#mainArea { background-color: #0a0d12; }
        QScrollBar:vertical {
            background: transparent;
            width: 8px;
            margin: 4px 2px 4px 0;
        }
        QScrollBar::handle:vertical {
            background: #30363d;
            border-radius: 4px;
            min-height: 30px;
        }
        QScrollBar::handle:vertical:hover { background: #484f58; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }
    )");
}

void MainWindow::setupDynamicUi() {
    // Root layout of the central area: a QStackedLayout in StackAll mode
    // so the particle background and the foreground chat surface coexist
    // as overlapping layers instead of stacking vertically.
    auto* stack = new QStackedLayout(ui->mainArea);
    stack->setStackingMode(QStackedLayout::StackAll);
    stack->setContentsMargins(0, 0, 0, 0);

    // ---- Bottom layer: animated particle background ----
    auto* bg = new ParticleBackground(ui->mainArea);
    bg->setAttribute(Qt::WA_TransparentForMouseEvents, true);

    // ---- Top layer: transparent foreground container ----
    auto* foreground = new QWidget(ui->mainArea);
    foreground->setAttribute(Qt::WA_TranslucentBackground, true);
    foreground->setStyleSheet("background: transparent;");

    auto* fgLayout = new QVBoxLayout(foreground);
    fgLayout->setContentsMargins(28, 28, 28, 22);
    fgLayout->setSpacing(16);

    // Chat scroll area
    scrollArea = new QScrollArea(foreground);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setStyleSheet("background: transparent; border: none;");
    scrollArea->viewport()->setStyleSheet("background: transparent;");

    auto* scrollContent = new QWidget;
    scrollContent->setStyleSheet("background: transparent;");
    chatLayout = new QVBoxLayout(scrollContent);
    chatLayout->setContentsMargins(0, 0, 6, 0);
    chatLayout->setSpacing(10);
    chatLayout->addStretch();
    scrollArea->setWidget(scrollContent);

    fgLayout->addWidget(scrollArea, /*stretch=*/1);

    // Input area — premium dark with subtle drop shadow
    inputWrapper = new QFrame(foreground);
    inputWrapper->setObjectName("inputWrapper");
    inputWrapper->setMinimumHeight(60);
    inputWrapper->setStyleSheet(R"(
        QFrame#inputWrapper {
            background-color: rgba(13, 17, 23, 220);
            border: 1px solid #21262d;
            border-radius: 14px;
        }
        QFrame#inputWrapper:focus-within {
            border: 1px solid #1f6feb;
        }
    )");
    inputWrapper->setGraphicsEffect(makeSoftShadow(inputWrapper, 30, 6, 180));

    auto* inputLayout = new QHBoxLayout(inputWrapper);
    inputLayout->setContentsMargins(18, 8, 8, 8);
    inputLayout->setSpacing(10);

    inputField = new QLineEdit(inputWrapper);
    inputField->setPlaceholderText(QStringLiteral("Message JARVIS…"));
    inputField->setStyleSheet(
        "background: transparent; color: #e6edf3; border: none; font-size: 14px;");

    sendButton = new QPushButton(QStringLiteral("→"), inputWrapper);
    sendButton->setFixedSize(42, 42);
    sendButton->setCursor(Qt::PointingHandCursor);
    sendButton->setStyleSheet(R"(
        QPushButton {
            background-color: #1f6feb;
            color: white;
            border: none;
            border-radius: 21px;
            font-weight: 700;
            font-size: 18px;
        }
        QPushButton:hover  { background-color: #388bfd; }
        QPushButton:pressed{ background-color: #1158c7; }
    )");

    inputLayout->addWidget(inputField, /*stretch=*/1);
    inputLayout->addWidget(sendButton);

    fgLayout->addWidget(inputWrapper);

    // Add layers — order matters: first widget is rendered behind, last on top.
    stack->addWidget(bg);
    stack->addWidget(foreground);
    stack->setCurrentWidget(foreground);

    // Connections
    connect(sendButton, &QPushButton::clicked, this, &MainWindow::onUserInput);
    connect(inputField, &QLineEdit::returnPressed, this, &MainWindow::onUserInput);
}

// ---------------------------------------------------------------------------
//  Chat helpers
// ---------------------------------------------------------------------------

bool MainWindow::isNearBottom() const {
    if (!scrollArea) return true;
    auto* bar = scrollArea->verticalScrollBar();
    return (bar->maximum() - bar->value()) <= kStickyScrollThresholdPx;
}

void MainWindow::scrollToBottom() {
    if (!scrollArea) return;
    auto* bar = scrollArea->verticalScrollBar();
    bar->setValue(bar->maximum());
}

void MainWindow::addMessage(const QString& text, bool isUser) {
    auto* msg = new MessageWidget(text, isUser, this);
    // Subtle drop shadow under each chat bubble
    msg->setGraphicsEffect(makeSoftShadow(msg, 22, 4, 140));

    chatLayout->insertWidget(chatLayout->count() - 1, msg);

    // Always pin to bottom when WE add a message; sticky logic only
    // gates auto-scroll for incoming streamed tokens.
    QTimer::singleShot(20, this, [this]() { scrollToBottom(); });
}

// ---------------------------------------------------------------------------
//  Slots
// ---------------------------------------------------------------------------

void MainWindow::onUserInput() {
    const QString text = inputField->text().trimmed();
    if (text.isEmpty()) return;

    addMessage(text, true);
    inputField->clear();

    currentAiBubble = new MessageWidget(QString(), /*isUser=*/false, this);
    currentAiBubble->setGraphicsEffect(makeSoftShadow(currentAiBubble, 22, 4, 140));
    chatLayout->insertWidget(chatLayout->count() - 1, currentAiBubble);

    aiThread->queuePrompt(Config::SYSTEM_PROMPT, text);
}

void MainWindow::updateAiStream(const QString& token) {
    if (!currentAiBubble) return;

    // Smart sticky scroll: capture user position BEFORE growing the bubble.
    const bool stickToBottom = isNearBottom();

    currentAiBubble->appendText(token);

    if (stickToBottom) {
        // Defer until the layout has applied the new bubble size.
        QTimer::singleShot(0, this, [this]() { scrollToBottom(); });
    }
}

void MainWindow::onReplyFinished(const QString& fullResponse) {
    currentAiBubble = nullptr;

    // Parse [CMD: ...] / [PS: ...] tags out of the *full* (unfiltered) response
    // and dispatch them to the system shell. Case-insensitive, multi-line aware.
    static const QRegularExpression cmdRe(
        QStringLiteral(R"(\[\s*CMD\s*:\s*([^\]]+?)\s*\])"),
        QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression psRe(
        QStringLiteral(R"(\[\s*PS\s*:\s*([^\]]+?)\s*\])"),
        QRegularExpression::CaseInsensitiveOption);

    auto runAll = [this](const QRegularExpression& re,
                         const QString& haystack,
                         bool isPs) {
        QRegularExpressionMatchIterator it = re.globalMatch(haystack);
        while (it.hasNext()) {
            const QRegularExpressionMatch m = it.next();
            const QString cmd = m.captured(1).trimmed();
            if (!cmd.isEmpty()) {
                handleSystemCommand(cmd, isPs);
            }
        }
    };

    runAll(cmdRe, fullResponse, /*isPs=*/false);
    runAll(psRe,  fullResponse, /*isPs=*/true);
}

void MainWindow::handleSystemCommand(const QString& shellCmd, bool isPowerShell) {
    if (shellCmd.trimmed().isEmpty()) return;

    if (isPowerShell) {
        QProcess::startDetached(QStringLiteral("powershell.exe"), {
            QStringLiteral("-NoProfile"),
            QStringLiteral("-WindowStyle"), QStringLiteral("Hidden"),
            QStringLiteral("-Command"), shellCmd
        });
    } else {
        QProcess::startDetached(QStringLiteral("cmd.exe"), {
            QStringLiteral("/c"), shellCmd
        });
    }
}

void MainWindow::openSettings() {
    SettingsDialog dlg(this, m_temperature, m_contextSize, m_modelPath);
    if (dlg.exec() != QDialog::Accepted) return;

    m_temperature = dlg.getTemperature();
    m_contextSize = dlg.getContextSize();

    aiThread->setParams(m_temperature, m_contextSize);

    const QString chosen = dlg.getSelectedModel();
    if (!chosen.isEmpty() && chosen != m_modelPath) {
        m_modelPath = chosen;
        addMessage(QStringLiteral("Loading model: %1").arg(m_modelPath), false);
        aiThread->queueLoadModel(m_modelPath);
    }
}

void MainWindow::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_F11) {
        if (isFullScreen()) showNormal();
        else                showFullScreen();
        return;
    }
    QMainWindow::keyPressEvent(event);
}
