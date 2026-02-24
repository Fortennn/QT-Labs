#ifndef KEYBOARDWIDGET_H
#define KEYBOARDWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QGridLayout>
#include <QMap>

class KeyboardWidget : public QWidget
{
    Q_OBJECT

public:
    explicit KeyboardWidget(QWidget *parent = nullptr);

    void pressKey(const QString &key);
    void releaseKey(const QString &key);

private:
    void addKey(QGridLayout *layout,
                const QString &label,
                const QString &mapKey,
                int row, int col,
                int colSpan = 1);

    QMap<QString, QPushButton*> m_keys;

    static const QString KEY_STYLE_NORMAL;
    static const QString KEY_STYLE_PRESSED;
};

#endif // KEYBOARDWIDGET_H
