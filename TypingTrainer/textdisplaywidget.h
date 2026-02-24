#ifndef TEXTDISPLAYWIDGET_H
#define TEXTDISPLAYWIDGET_H

#include <QWidget>
#include <QTextEdit>
#include <QVBoxLayout>

class TextDisplayWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TextDisplayWidget(QWidget *parent = nullptr);

    void setText(const QString &text);

    void setTypedCount(int count);

private:
    QTextEdit *m_display;

    QString m_text;
    int     m_typedCount = 0;

    void refreshDisplay();
};

#endif // TEXTDISPLAYWIDGET_H
