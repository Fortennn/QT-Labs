#ifndef MESSAGE_WIDGET_H
#define MESSAGE_WIDGET_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>

class MessageWidget : public QWidget {
    Q_OBJECT
public:
    explicit MessageWidget(const QString& text, bool isUser, QWidget *parent = nullptr);
    void appendText(const QString& text);

private:
    QLabel* label;
    bool m_isUser;
};

#endif // MESSAGE_WIDGET_H
