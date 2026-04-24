#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "ai/SystemPrompt.h"
#include "widgets/BrainVisualizer.h"
#include "widgets/ParticleBackground.h"
#include "widgets/MessageWidget.h"
#include <QScrollBar>
#include <QVBoxLayout>
#include <QTimer>
#include <QKeyEvent>
#include <QScrollArea>
#include <QPushButton>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), currentAiBubble(nullptr) {
    ui->setupUi(this);
    
    // 1. Мозок
    auto* brainLayout = new QVBoxLayout(ui->brainContainer);
    auto* brain = new BrainVisualizer(this);
    brainLayout->addWidget(brain);
    brainLayout->setContentsMargins(0, 0, 0, 0);

    // 2. Створюємо UI
    setupDynamicUi();

    // Додаємо телеметрію для краси (під кнопками)
    auto* telemetryLabel = new QLabel("CORE STABILITY: 99.8%\nNPU LOAD: 12%\nNEURAL SYNC: ACTIVE", this);
    telemetryLabel->setStyleSheet("color: #30363d; font-size: 10px; margin-top: 10px;");
    ui->sideLayout->insertWidget(ui->sideLayout->count() - 2, telemetryLabel);

    // Таймер для оновлення телеметрії (імітація життя)
    auto* tTimer = new QTimer(this);
    connect(tTimer, &QTimer::timeout, this, [telemetryLabel](){
        int load = 10 + (rand() % 15);
        telemetryLabel->setText(QString("CORE STABILITY: 99.9%\nNPU LOAD: %1%\nNEURAL SYNC: ACTIVE").arg(load));
    });
    tTimer->start(2000);

    aiThread = new LlamaWorkerThread(this);
    
    connect(ui->btn_clear, &QPushButton::clicked, this, [this](){
        aiThread->clearHistory();
        QLayoutItem *child;
        while ((child = chatLayout->takeAt(0)) != nullptr) {
            if (child->widget()) delete child->widget();
            delete child;
        }
        chatLayout->addStretch();
    });
    
    connect(aiThread, &LlamaWorkerThread::tokenGenerated, this, &MainWindow::updateAiStream, Qt::QueuedConnection);
    connect(aiThread, &LlamaWorkerThread::replyFinished, this, &MainWindow::onReplyFinished, Qt::QueuedConnection);
    
    connect(aiThread, &LlamaWorkerThread::errorOccurred, this, [this](const QString& err){
        addMessage("ERROR: " + err, false);
    }, Qt::QueuedConnection);

    connect(aiThread, &LlamaWorkerThread::modelLoaded, this, [this](bool success){
        addMessage("JARVIS CORE ONLINE. Чим можу допомогти?", false);
    }, Qt::QueuedConnection);
    
    aiThread->setStackSize(16 * 1024 * 1024);
    aiThread->start();

    QString modelPath = "C:/Papki/qt-labs/JARVIS/models/dolphin.gguf";
    aiThread->queueLoadModel(modelPath);
}

void MainWindow::setupDynamicUi() {
    // Головний фон
    auto* mainLayout = new QVBoxLayout(ui->mainArea);
    mainLayout->setContentsMargins(0,0,0,0);
    
    auto* bg = new ParticleBackground(this);
    mainLayout->addWidget(bg);
    
    // Контейнер поверх фону
    auto* bgLayout = new QVBoxLayout(bg);
    bgLayout->setContentsMargins(20, 20, 20, 20);
    bgLayout->setSpacing(15);
    
    // Scroll Area
    scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("background: transparent; border: none;");
    scrollArea->viewport()->setStyleSheet("background: transparent;");
    
    auto* scrollContent = new QWidget();
    scrollContent->setStyleSheet("background: transparent;");
    chatLayout = new QVBoxLayout(scrollContent);
    chatLayout->addStretch();
    scrollArea->setWidget(scrollContent);
    
    bgLayout->addWidget(scrollArea);
    
    // Input Area
    auto* inputWrapper = new QFrame(this);
    inputWrapper->setMinimumHeight(60);
    inputWrapper->setStyleSheet("background: rgba(13, 17, 23, 180); border: 1px solid #30363d; border-radius: 12px;");
    
    auto* inputLayout = new QHBoxLayout(inputWrapper);
    inputField = new QLineEdit(this);
    inputField->setPlaceholderText("Message JARVIS...");
    inputField->setStyleSheet("background: transparent; color: white; border: none; font-size: 14px;");
    
    sendButton = new QPushButton("→", this);
    sendButton->setFixedSize(40, 40);
    sendButton->setStyleSheet("background-color: #238636; color: white; border-radius: 20px; font-weight: bold; font-size: 18px;");
    
    inputLayout->addWidget(inputField);
    inputLayout->addWidget(sendButton);
    
    bgLayout->addWidget(inputWrapper);
    
    // Коннекти
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
    
    QTimer::singleShot(50, this, [this](){
        scrollArea->verticalScrollBar()->setValue(scrollArea->verticalScrollBar()->maximum());
    });
}

void MainWindow::onUserInput() {
    QString text = inputField->text().trimmed();
    if (text.isEmpty()) return;
    
    addMessage(text, true);
    inputField->clear();
    
    currentAiBubble = new MessageWidget("", false, this);
    chatLayout->insertWidget(chatLayout->count() - 1, currentAiBubble);
    
    aiThread->queuePrompt(Config::SYSTEM_PROMPT, text);
}

void MainWindow::updateAiStream(const QString& token) {
    if (currentAiBubble) {
        currentAiBubble->appendText(token);
        scrollArea->verticalScrollBar()->setValue(scrollArea->verticalScrollBar()->maximum());
    }
}

void MainWindow::onReplyFinished(const QString& fullResponse) {
    currentAiBubble = nullptr;
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
    // Більшість стилів тепер в setupDynamicUi та в .ui файлі
}
