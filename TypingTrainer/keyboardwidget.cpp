#include "keyboardwidget.h"

const QString KeyboardWidget::KEY_STYLE_NORMAL  =
    "QPushButton { background: #e8e8e8; border: 1px solid #b0b0b0; "
    "border-radius: 4px; font-size: 11px; padding: 2px; }";

const QString KeyboardWidget::KEY_STYLE_PRESSED =
    "QPushButton { background: #f0a500; border: 1px solid #cc8800; "
    "border-radius: 4px; font-size: 11px; padding: 2px; color: white; }";

KeyboardWidget::KeyboardWidget(QWidget *parent)
    : QWidget(parent)
{
    QGridLayout *layout = new QGridLayout(this);
    layout->setSpacing(3);
    layout->setContentsMargins(4, 4, 4, 4);

    QStringList r0labels = {"`","1","2","3","4","5","6","7","8","9","0","-","="};
    QStringList r0keys   = {"`","1","2","3","4","5","6","7","8","9","0","-","="};
    for (int i = 0; i < r0labels.size(); ++i)
        addKey(layout, r0labels[i], r0keys[i], 0, i);
    addKey(layout, "←", "BACKSPACE", 0, 13, 2);

    addKey(layout, "Tab", "TAB", 1, 0, 2);
    QStringList r1 = {"Q","W","E","R","T","Y","U","I","O","P","[","]","\\"};
    for (int i = 0; i < r1.size(); ++i)
        addKey(layout, r1[i], r1[i], 1, i + 2);

    addKey(layout, "Caps", "CAPS", 2, 0, 2);
    QStringList r2 = {"A","S","D","F","G","H","J","K","L",";","'"};
    for (int i = 0; i < r2.size(); ++i)
        addKey(layout, r2[i], r2[i], 2, i + 2);
    addKey(layout, "↵", "ENTER", 2, 13, 2);

    addKey(layout, "⇧", "SHIFT", 3, 0, 3);
    QStringList r3 = {"Z","X","C","V","B","N","M",",",".","/"}; 
    for (int i = 0; i < r3.size(); ++i)
        addKey(layout, r3[i], r3[i], 3, i + 3);
    addKey(layout, "⇧", "SHIFT_R", 3, 13, 2);

    addKey(layout, "Ctrl",  "CTRL",  4, 0, 2);
    addKey(layout, "Alt",   "ALT",   4, 2, 2);
    addKey(layout, "Space", "SPACE", 4, 4, 7);
    addKey(layout, "Alt",   "ALT_R", 4, 11, 2);
    addKey(layout, "Ctrl",  "CTRL_R",4, 13, 2);

    setLayout(layout);
}

void KeyboardWidget::addKey(QGridLayout *layout,
                             const QString &label,
                             const QString &mapKey,
                             int row, int col,
                             int colSpan)
{
    QPushButton *btn = new QPushButton(label);
    btn->setMinimumHeight(38);
    btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    btn->setStyleSheet(KEY_STYLE_NORMAL);
    btn->setFocusPolicy(Qt::NoFocus);
    layout->addWidget(btn, row, col, 1, colSpan);

    m_keys[mapKey.toUpper()] = btn;
}

void KeyboardWidget::pressKey(const QString &key)
{
    QString k = key.toUpper();
    if (m_keys.contains(k))
        m_keys[k]->setStyleSheet(KEY_STYLE_PRESSED);
}

void KeyboardWidget::releaseKey(const QString &key)
{
    QString k = key.toUpper();
    if (m_keys.contains(k))
        m_keys[k]->setStyleSheet(KEY_STYLE_NORMAL);
}
