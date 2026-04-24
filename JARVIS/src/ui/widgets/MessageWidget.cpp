#include "MessageWidget.h"

MessageWidget::MessageWidget(const QString& text, bool isUser, QWidget *parent) 
    : QWidget(parent), m_isUser(isUser) {
    
    auto* layout = new QVBoxLayout(this);
    label = new QLabel(text, this);
    label->setWordWrap(true);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    
    QString style = isUser ? 
        "background-color: #1f6feb; color: white; border-radius: 15px; padding: 12px; font-size: 14px;" :
        "background-color: #21262d; color: #e6edf3; border: 1px solid #30363d; border-radius: 15px; padding: 12px; font-size: 14px;";
    
    label->setStyleSheet(style);
    
    layout->addWidget(label);
    layout->setContentsMargins(isUser ? 50 : 10, 5, isUser ? 10 : 50, 5);
    
    // Вирівнювання самого віджета в лейауті будемо робити через MainWindow
}

void MessageWidget::appendText(const QString& text) {
    label->setText(label->text() + text);
}
