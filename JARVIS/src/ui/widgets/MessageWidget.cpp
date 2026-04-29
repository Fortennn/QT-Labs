#include "MessageWidget.h"

#include <QGraphicsDropShadowEffect>

MessageWidget::MessageWidget(const QString& text, bool isUser, QWidget *parent) 
    : QWidget(parent), m_isUser(isUser) {
    
    auto* layout = new QVBoxLayout(this);
    label = new QLabel(text, this);
    label->setWordWrap(true);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    
    QString style = isUser ?
        "background-color: #2f81f7; color: #f8fafc; border-radius: 14px; padding: 12px 13px; font-size: 14px;" :
        "background-color: rgba(15, 22, 31, 220); color: #dfe6ef; border: 1px solid #273344; border-radius: 14px; padding: 12px 13px; font-size: 14px;";
    
    label->setStyleSheet(style);

    auto* shadow = new QGraphicsDropShadowEffect(label);
    shadow->setBlurRadius(18.0);
    shadow->setOffset(0, 4);
    shadow->setColor(QColor(0, 0, 0, 120));
    label->setGraphicsEffect(shadow);
    
    layout->addWidget(label);
    layout->setContentsMargins(isUser ? 50 : 10, 5, isUser ? 10 : 50, 5);
    
    // Вирівнювання самого віджета в лейауті будемо робити через MainWindow
}

void MessageWidget::appendText(const QString& text) {
    label->setText(label->text() + text);
}
