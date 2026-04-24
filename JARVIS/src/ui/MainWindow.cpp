#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "ai/SystemPrompt.h"
#include <QScrollBar>
#include <QCoreApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    
    aiThread = new LlamaWorkerThread(this);
    
    connect(ui->sendButton, &QPushButton::clicked, this, &MainWindow::onUserInput);
    connect(ui->inputField, &QLineEdit::returnPressed, this, &MainWindow::onUserInput);
    
    connect(aiThread, &LlamaWorkerThread::tokenGenerated, this, &MainWindow::updateAiStream, Qt::QueuedConnection);
    connect(aiThread, &LlamaWorkerThread::replyFinished, this, &MainWindow::onReplyFinished, Qt::QueuedConnection);
    
    connect(aiThread, &LlamaWorkerThread::errorOccurred, this, [this](const QString& err){
        ui->chatBrowser->append("<br><b style='color: #f04747;'>[JARVIS SYSTEM ERROR]:</b> " + err);
    }, Qt::QueuedConnection);

    connect(aiThread, &LlamaWorkerThread::modelLoaded, this, [this](bool success){
        ui->chatBrowser->append("<b style='color: #43b581;'>[JARVIS SYSTEM]:</b> Нейромережевий мозок успішно підключено та ініціалізовано!");
    }, Qt::QueuedConnection);
    
    aiThread->setStackSize(16 * 1024 * 1024);
    aiThread->start();

    QString modelPath = "C:/Papki/qt-labs/JARVIS/models/dolphin.gguf";
    aiThread->queueLoadModel(modelPath);
    
    resize(900, 700);
}

MainWindow::~MainWindow() {
    aiThread->stopGeneration();
    aiThread->quit();
    aiThread->wait();
    delete ui;
}

void MainWindow::onUserInput() {
    QString text = ui->inputField->text().trimmed();
    if (text.isEmpty()) return;
    
    ui->chatBrowser->append("<b style='color: #7289da;'>Ви:</b> " + text);
    ui->inputField->clear();
    
    ui->chatBrowser->append("<b style='color: #eb459e;'>JARVIS:</b> ");
    aiThread->queuePrompt(Config::SYSTEM_PROMPT, text);
}

void MainWindow::updateAiStream(const QString& token) {
    QTextCursor cursor = ui->chatBrowser->textCursor();
    cursor.movePosition(QTextCursor::End);
    ui->chatBrowser->setTextCursor(cursor);
    ui->chatBrowser->insertPlainText(token);
    ui->chatBrowser->verticalScrollBar()->setValue(ui->chatBrowser->verticalScrollBar()->maximum());
}

void MainWindow::onReplyFinished(const QString& fullResponse) {
    ui->chatBrowser->append("<br>");
}
