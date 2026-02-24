#include "textdisplaywidget.h"
#include <QFont>

TextDisplayWidget::TextDisplayWidget(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_display = new QTextEdit(this);
    m_display->setReadOnly(true);
    m_display->setFocusPolicy(Qt::NoFocus);
    m_display->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QFont font("Courier New", 14);
    font.setStyleHint(QFont::Monospace);
    m_display->setFont(font);

    m_display->setStyleSheet(
        "QTextEdit {"
        "  background: #f9f9f9;"
        "  border: 1px solid #cccccc;"
        "  border-radius: 4px;"
        "  padding: 8px;"
        "}"
    );

    layout->addWidget(m_display);

    setText("jfj j jff ffjjjj fjjf fjjj jjjjjjj\n"
            "jfj j jff ffjjjj Fjn ffj\n"
            "asdf jkl; asdf jkl; asdf jkl;");
}

void TextDisplayWidget::setText(const QString &text)
{
    m_text = text;
    m_typedCount = 0;
    m_display->setPlainText(m_text);
}

void TextDisplayWidget::setTypedCount(int count)
{
    m_typedCount = count;
}

void TextDisplayWidget::refreshDisplay()
{
}
