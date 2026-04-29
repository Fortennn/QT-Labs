#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "ai/SystemPrompt.h"
#include "SettingsDialog.h"
#include "widgets/BrainVisualizer.h"
#include "widgets/ParticleBackground.h"
#include "widgets/MessageWidget.h"
#include <QCoreApplication>
#include <QColor>
#include <QDialog>
#include <QDir>
#include <QFont>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QScrollBar>
#include <QKeyEvent>
#include <QLabel>
#include <QProcess>
#include <QScrollArea>
#include <QPushButton>
#include <QStackedLayout>
#include <QThread>
#include <QTimer>
#include <QVBoxLayout>
#include <cstdlib>

namespace {

QString defaultModelPath() {
    QStringList rootCandidates;
    rootCandidates << QDir::current().absoluteFilePath("models")
                   << QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("models");

    for (const QString& rootPath : rootCandidates) {
        QDir modelDir(rootPath);
        if (!modelDir.exists()) {
            continue;
        }

        const QStringList models = modelDir.entryList({"*.gguf"}, QDir::Files, QDir::Name);
        if (models.isEmpty()) {
            continue;
        }

        for (const QString& modelFile : models) {
            if (modelFile.contains("dolphin", Qt::CaseInsensitive)) {
                return modelDir.absoluteFilePath(modelFile);
            }
        }

        return modelDir.absoluteFilePath(models.first());
    }

    return QDir::current().absoluteFilePath("models/dolphin.gguf");
}

}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , aiThread(nullptr)
    , currentAiBubble(nullptr)
    , scrollArea(nullptr)
    , chatLayout(nullptr)
    , inputField(nullptr)
    , sendButton(nullptr)
    , inputWrapper(nullptr) {
    ui->setupUi(this);

    applyPremiumStyles();

    auto* brainLayout = new QVBoxLayout(ui->brainContainer);
    auto* brain = new BrainVisualizer(this);
    brainLayout->addWidget(brain);
    brainLayout->setContentsMargins(0, 0, 0, 0);

    setupDynamicUi();

    auto* telemetryLabel = new QLabel("CORE STABILITY: 99.8%\nNPU LOAD: 12%\nNEURAL SYNC: ACTIVE", this);
    telemetryLabel->setStyleSheet("color: #7f8ea3; font-size: 10px; margin-top: 8px; letter-spacing: 0.5px;");
    ui->sideLayout->insertWidget(ui->sideLayout->count() - 2, telemetryLabel);

    auto* tTimer = new QTimer(this);
    connect(tTimer, &QTimer::timeout, this, [telemetryLabel]() {
        int load = 10 + (rand() % 15);
        telemetryLabel->setText(QString("CORE STABILITY: 99.9%\nNPU LOAD: %1%\nNEURAL SYNC: ACTIVE").arg(load));
    });
    tTimer->start(2000);

    aiThread = new LlamaWorkerThread(this);

    connect(ui->btn_clear, &QPushButton::clicked, this, [this]() {
        aiThread->clearHistory();
        QLayoutItem* child = nullptr;
        while ((child = chatLayout->takeAt(0)) != nullptr) {
            if (child->widget()) {
                delete child->widget();
            }
            delete child;
        }
        chatLayout->addStretch();
    });

    connect(ui->btn_settings, &QPushButton::clicked, this, [this]() {
        SettingsDialog dlg(this);
        if (dlg.exec() != QDialog::Accepted) {
            return;
        }

        aiThread->setParams(dlg.getTemperature(), dlg.getContextSize());
        aiThread->queueLoadModel(dlg.getSelectedModel());
    });

    connect(aiThread, &LlamaWorkerThread::tokenGenerated, this, &MainWindow::updateAiStream, Qt::QueuedConnection);
    connect(aiThread, &LlamaWorkerThread::replyFinished, this, &MainWindow::onReplyFinished, Qt::QueuedConnection);

    connect(aiThread, &LlamaWorkerThread::errorOccurred, this, [this](const QString& err) {
        addMessage("ERROR: " + err, false);
    }, Qt::QueuedConnection);

    connect(aiThread, &LlamaWorkerThread::modelLoaded, this, [this](bool success) {
        if (!success) {
            return;
        }

        addMessage("JARVIS CORE ONLINE. Чим можу допомогти?", false);
    }, Qt::QueuedConnection);

    aiThread->setStackSize(16 * 1024 * 1024);
    aiThread->start();
    aiThread->queueLoadModel(defaultModelPath());
}

void MainWindow::setupDynamicUi() {
    auto* mainLayout = new QVBoxLayout(ui->mainArea);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    auto* stackedRoot = new QWidget(ui->mainArea);
    stackedRoot->setAttribute(Qt::WA_StyledBackground, true);

    auto* layeredLayout = new QStackedLayout(stackedRoot);
    layeredLayout->setStackingMode(QStackedLayout::StackAll);
    layeredLayout->setContentsMargins(0, 0, 0, 0);

    auto* bg = new ParticleBackground(stackedRoot);
    bg->setObjectName("particleBackground");
    bg->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    layeredLayout->addWidget(bg);

    auto* foreground = new QWidget(stackedRoot);
    foreground->setAttribute(Qt::WA_StyledBackground, true);
    foreground->setStyleSheet("background: transparent;");

    auto* fgLayout = new QVBoxLayout(foreground);
    fgLayout->setContentsMargins(24, 24, 24, 24);
    fgLayout->setSpacing(14);

    scrollArea = new QScrollArea(foreground);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setStyleSheet(
        "QScrollArea { background: transparent; border: none; }"
        "QScrollBar:vertical {"
        "  background: rgba(16, 22, 31, 110);"
        "  width: 10px;"
        "  margin: 8px 2px;"
        "  border-radius: 5px;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: rgba(108, 126, 150, 170);"
        "  min-height: 30px;"
        "  border-radius: 5px;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
    );
    scrollArea->viewport()->setStyleSheet("background: transparent;");

    auto* scrollContent = new QWidget();
    scrollContent->setAttribute(Qt::WA_StyledBackground, true);
    scrollContent->setStyleSheet("background: transparent;");
    chatLayout = new QVBoxLayout(scrollContent);
    chatLayout->setContentsMargins(4, 4, 4, 4);
    chatLayout->setSpacing(12);
    chatLayout->addStretch();
    scrollArea->setWidget(scrollContent);

    fgLayout->addWidget(scrollArea, 1);

    inputWrapper = new QFrame(foreground);
    inputWrapper->setMinimumHeight(60);
    inputWrapper->setStyleSheet(
        "QFrame {"
        "  background: rgba(10, 14, 19, 235);"
        "  border: 1px solid #2a3340;"
        "  border-radius: 14px;"
        "}"
    );

    auto* inputShadow = new QGraphicsDropShadowEffect(inputWrapper);
    inputShadow->setBlurRadius(28.0);
    inputShadow->setOffset(0, 8);
    inputShadow->setColor(QColor(0, 0, 0, 140));
    inputWrapper->setGraphicsEffect(inputShadow);

    auto* inputLayout = new QHBoxLayout(inputWrapper);
    inputLayout->setContentsMargins(14, 8, 8, 8);
    inputLayout->setSpacing(10);

    inputField = new QLineEdit(inputWrapper);
    inputField->setPlaceholderText("Message JARVIS...");
    inputField->setStyleSheet(
        "QLineEdit {"
        "  background: transparent;"
        "  color: #e6edf3;"
        "  border: none;"
        "  font-size: 14px;"
        "  padding: 6px 4px;"
        "}"
        "QLineEdit::placeholder { color: #7f8ea3; }"
    );

    sendButton = new QPushButton("Send", inputWrapper);
    sendButton->setFixedSize(64, 40);
    sendButton->setCursor(Qt::PointingHandCursor);
    sendButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #2f81f7;"
        "  color: #f8fafc;"
        "  border: none;"
        "  border-radius: 12px;"
        "  font-size: 13px;"
        "  font-weight: 600;"
        "}"
        "QPushButton:hover { background-color: #4b93f8; }"
        "QPushButton:pressed { background-color: #2468c7; }"
    );

    inputLayout->addWidget(inputField);
    inputLayout->addWidget(sendButton);

    fgLayout->addWidget(inputWrapper);

    layeredLayout->addWidget(foreground);
    layeredLayout->setCurrentWidget(foreground);
    foreground->raise();
    mainLayout->addWidget(stackedRoot);

    connect(sendButton, &QPushButton::clicked, this, &MainWindow::onUserInput);
    connect(inputField, &QLineEdit::returnPressed, this, &MainWindow::onUserInput);
}

MainWindow::~MainWindow() {
    if (aiThread) {
        aiThread->stopGeneration();
        aiThread->quit();
        aiThread->wait();
    }
    delete ui;
}

void MainWindow::addMessage(const QString& text, bool isUser) {
    auto* msg = new MessageWidget(text, isUser, this);
    chatLayout->insertWidget(chatLayout->count() - 1, msg);
    QTimer::singleShot(0, this, [this]() { scrollToBottom(); });
}

void MainWindow::onUserInput() {
    QString text = inputField->text().trimmed();
    if (text.isEmpty()) {
        return;
    }

    addMessage(text, true);
    inputField->clear();

    currentAiBubble = new MessageWidget("", false, this);
    chatLayout->insertWidget(chatLayout->count() - 1, currentAiBubble);

    aiThread->queuePrompt(Config::SYSTEM_PROMPT, text);
    scrollToBottom();
}

void MainWindow::updateAiStream(const QString& token) {
    if (!currentAiBubble) {
        return;
    }

    const bool keepPinned = isNearBottom();
    currentAiBubble->appendText(token);
    if (keepPinned) {
        scrollToBottom();
    }
}

void MainWindow::onReplyFinished(const QString& fullResponse) {
    static const QRegularExpression cmdRegex(
        R"(\[(cmd|ps)\s*:\s*([^\]]+)\])",
        QRegularExpression::CaseInsensitiveOption
    );

    QRegularExpressionMatchIterator it = cmdRegex.globalMatch(fullResponse);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        const QString shell = match.captured(1).toLower();
        const QString command = match.captured(2).trimmed();
        if (command.isEmpty()) {
            continue;
        }

        handleSystemCommand(command, shell == "ps");
    }

    currentAiBubble = nullptr;
}

void MainWindow::handleSystemCommand(const QString& shellCmd, bool isPowerShell) {
    if (shellCmd.trimmed().isEmpty()) {
        return;
    }

    if (isPowerShell) {
        QProcess::startDetached(
            "powershell.exe",
            {"-NoProfile", "-WindowStyle", "Hidden", "-Command", shellCmd}
        );
        return;
    }

    QProcess::startDetached("cmd.exe", {"/c", shellCmd});
}

bool MainWindow::isNearBottom() const {
    if (!scrollArea) {
        return true;
    }

    QScrollBar* bar = scrollArea->verticalScrollBar();
    if (!bar) {
        return true;
    }

    return (bar->maximum() - bar->value()) <= 24;
}

void MainWindow::scrollToBottom() {
    if (!scrollArea) {
        return;
    }

    QScrollBar* bar = scrollArea->verticalScrollBar();
    if (!bar) {
        return;
    }

    bar->setValue(bar->maximum());
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_F11) {
        if (isFullScreen()) showNormal();
        else showFullScreen();
    } else {
        QMainWindow::keyPressEvent(event);
    }
}

void MainWindow::applyPremiumStyles() {
    QFont appFont("Segoe UI", 10);
    appFont.setStyleStrategy(QFont::PreferAntialias);
    setFont(appFont);

    setStyleSheet(
        "QMainWindow {"
        "  background-color: #090d12;"
        "  color: #e6edf3;"
        "}"
        "QFrame#sidePanel {"
        "  background-color: #0a1018;"
        "  border-right: 1px solid #1f2a38;"
        "}"
        "QPushButton#btn_clear, QPushButton#btn_settings {"
        "  background-color: #111925;"
        "  color: #d7dee7;"
        "  border: 1px solid #253244;"
        "  border-radius: 10px;"
        "  padding: 10px 12px;"
        "  font-size: 12px;"
        "  letter-spacing: 1px;"
        "}"
        "QPushButton#btn_clear:hover, QPushButton#btn_settings:hover {"
        "  background-color: #162232;"
        "  border-color: #364960;"
        "}"
        "QLabel#label_status {"
        "  color: #95a2b5;"
        "  font-size: 11px;"
        "  letter-spacing: 1px;"
        "}"
    );
}
