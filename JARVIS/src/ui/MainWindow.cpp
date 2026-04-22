#include "MainWindow.h"
#include <QVBoxLayout>
#include <QPushButton>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
    setupUi();
    loadStyles();
    
    resize(800, 600);
}

MainWindow::~MainWindow() {}

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
    
    chatBrowser->append("<i style='color: #aaaaaa;'>Thinking...</i>");
}

void MainWindow::updateAiStream(const QString& token) {
}
