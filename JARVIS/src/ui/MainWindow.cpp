#include "MainWindow.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QScrollBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
    setupUi();
    loadStyles();
    
    // Ініціалізуємо фоновий AI потік
    aiThread = new LlamaWorkerThread(this);
    connect(aiThread, &LlamaWorkerThread::tokenGenerated, this, &MainWindow::updateAiStream);
    connect(aiThread, &LlamaWorkerThread::replyFinished, this, &MainWindow::onReplyFinished);
    aiThread->start(); // Запускаємо нескінченний цикл
    
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
        QMainWindow { background-color: #36393f; }
        QTextBrowser { 
            background-color: #36393f; 
            color: #dcddde; 
            border: none;
            font-size: 14px;
            font-family: 'Segoe UI', Inter, sans-serif;
        }
        QLineEdit { 
            background-color: #40444b; 
            color: #dcddde; 
            border: none;
            border-radius: 8px;
            padding: 10px;
            font-size: 14px;
        }
        QPushButton {
            background-color: #5865f2;
            color: white;
            border-radius: 8px;
            padding: 8px 16px;
            font-weight: bold;
        }
        QPushButton:hover { background-color: #4752c4; }
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
