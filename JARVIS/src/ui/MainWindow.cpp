#include "MainWindow.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QScrollBar>
#include <QCoreApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
    setupUi();
    loadStyles();
    
    aiThread = new LlamaWorkerThread(this);
    connect(aiThread, &LlamaWorkerThread::tokenGenerated, this, &MainWindow::updateAiStream, Qt::QueuedConnection);
    connect(aiThread, &LlamaWorkerThread::replyFinished, this, &MainWindow::onReplyFinished, Qt::QueuedConnection);
    
    connect(aiThread, &LlamaWorkerThread::errorOccurred, this, [this](const QString& err){
        chatBrowser->append("<br><b style='color: #f04747;'>[JARVIS SYSTEM ERROR]:</b> " + err);
    }, Qt::QueuedConnection);
    connect(aiThread, &LlamaWorkerThread::modelLoaded, this, [this](bool success){
        chatBrowser->append("<b style='color: #43b581;'>[JARVIS SYSTEM]:</b> Нейромережевий мозок успішно підключено та ініціалізовано!");
    }, Qt::QueuedConnection);
    
    // llama.cpp вимагає великого стеку, стандартних 2 МБ в MinGW може не вистачити і потік просто "тихо" впаде
    aiThread->setStackSize(16 * 1024 * 1024); // 16 Мегабайт
    aiThread->start(); // Запускаємо нескінченний цикл

    // Вказуємо абсолютний шлях до папки з моделлю (для зручності під час розробки)
    QString modelPath = "C:/Papki/qt-labs/JARVIS/models/dolphin.gguf";
    aiThread->queueLoadModel(modelPath);
    
    resize(800, 600);
}

MainWindow::~MainWindow() {
    aiThread->stopGeneration();
    aiThread->quit();
    aiThread->wait();
}

void MainWindow::setupUi() {
    QWidget* centralWidget = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    
    chatBrowser = new QTextBrowser(this);
    chatBrowser->setReadOnly(true);
    chatBrowser->setOpenExternalLinks(true);
    
    QHBoxLayout* inputLayout = new QHBoxLayout();
    inputField = new QLineEdit(this);
    inputField->setPlaceholderText("Введіть запит до AI або системну команду...");
    
    QPushButton* sendButton = new QPushButton("Send", this);
    
    inputLayout->addWidget(inputField);
    inputLayout->addWidget(sendButton);
    
    mainLayout->addWidget(chatBrowser);
    mainLayout->addLayout(inputLayout);
    
    setCentralWidget(centralWidget);
    
    connect(sendButton, &QPushButton::clicked, this, &MainWindow::onUserInput);
    connect(inputField, &QLineEdit::returnPressed, this, &MainWindow::onUserInput);
}

void MainWindow::loadStyles() {
    QString qss = R"(
        QMainWindow { 
            background-color: #0b0f19; 
        }
        QTextBrowser { 
            background-color: #0b0f19; 
            color: #d2d6df; 
            border: none;
            font-size: 15px;
            font-family: 'Segoe UI', Inter, sans-serif;
            padding: 24px;
        }
        QLineEdit { 
            background-color: #151a28; 
            color: #c9d1d9; 
            border: 1px solid #2d3446;
            border-radius: 20px;
            padding: 12px 20px;
            font-size: 15px;
            margin-left: 10px;
            margin-bottom: 10px;
        }
        QLineEdit:focus {
            border: 1px solid #7289da;
            background-color: #1a2030;
        }
        QPushButton {
            background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #5865f2, stop:1 #8e44ad);
            color: white;
            border-radius: 20px;
            padding: 12px 24px;
            font-weight: bold;
            font-size: 14px;
            margin-right: 10px;
            margin-bottom: 10px;
        }
        QPushButton:hover { 
            background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #4752c4, stop:1 #9b59b6);
        }
        QPushButton:pressed {
            background-color: #5865f2;
            padding-top: 14px;
        }
        QScrollBar:vertical {
            border: none;
            background: #0b0f19;
            width: 8px;
            margin: 0px;
        }
        QScrollBar::handle:vertical {
            background: #2d3446;
            min-height: 20px;
            border-radius: 4px;
        }
        QScrollBar::handle:vertical:hover {
            background: #5865f2;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            border: none;
            background: none;
        }
    )";
    this->setStyleSheet(qss);
}

void MainWindow::onUserInput() {
    QString text = inputField->text().trimmed();
    if (text.isEmpty()) return;
    
    chatBrowser->append("<b style='color: #7289da;'>Ви:</b> " + text);
    inputField->clear();
    
    // Вставляємо індикатор початку відповіді JARVIS
    chatBrowser->append("<b style='color: #eb459e;'>JARVIS:</b> ");
    
    // Надсилаємо запит у фоновий потік
    aiThread->queuePrompt("Ти - розумний асистент.", text);
}

void MainWindow::updateAiStream(const QString& token) {
    // Вставляємо токен в існуючий рядок (ефект набору тексту в реальному часі)
    QTextCursor cursor = chatBrowser->textCursor();
    cursor.movePosition(QTextCursor::End);
    chatBrowser->setTextCursor(cursor);
    chatBrowser->insertPlainText(token);
    
    // Прокручуємо чат вниз, якщо текст виходить за межі екрана
    chatBrowser->verticalScrollBar()->setValue(chatBrowser->verticalScrollBar()->maximum());
}

void MainWindow::onReplyFinished(const QString& fullResponse) {
    chatBrowser->append("<br>"); // Невеликий відступ після завершення повідомлення
}
