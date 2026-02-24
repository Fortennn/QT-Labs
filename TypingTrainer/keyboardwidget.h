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

    void highlightNextKey(const QString &ch);
    void clearHighlight();

private:
    void addKey(QGridLayout *layout,
                const QString &label,
                const QString &mapKey,
                int row, int col,
                int colSpan = 1);

    QString resolveKey(const QString &ch) const;

    QMap<QString, QPushButton*> m_keys;
    QString m_highlightedKey;

    static const QString STYLE_NORMAL;
    static const QString STYLE_PRESSED;
    static const QString STYLE_HINT;
};

#endif // KEYBOARDWIDGET_H
