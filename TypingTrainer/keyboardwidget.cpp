#include "keyboardwidget.h"

const QString KeyboardWidget::STYLE_NORMAL =
    "QPushButton { background: #e8e8e8; border: 1px solid #b0b0b0; "
    "border-radius: 4px; font-size: 11px; padding: 2px; }";

const QString KeyboardWidget::STYLE_PRESSED =
    "QPushButton { background: #f0a500; border: 1px solid #cc8800; "
    "border-radius: 4px; font-size: 11px; padding: 2px; color: white; }";

const QString KeyboardWidget::STYLE_HINT =
    "QPushButton { background: #4caf50; border: 2px solid #2e7d32; "
    "border-radius: 4px; font-size: 11px; padding: 2px; color: white; font-weight: bold; }";

KeyboardWidget::KeyboardWidget(QWidget *parent)
    : QWidget(parent)
{
    QGridLayout *layout = new QGridLayout(this);
    layout->setSpacing(3);
    layout->setContentsMargins(4, 4, 4, 4);

    QStringList r0l = {"`","1","2","3","4","5","6","7","8","9","0","-","="};
    for (int i = 0; i < r0l.size(); ++i)
        addKey(layout, r0l[i], r0l[i], 0, i);
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
    QStringList r3 = {"Z","X","C","V","B","N","M",",",".","/","'"};
    for (int i = 0; i < r3.size(); ++i)
        addKey(layout, r3[i], r3[i], 3, i + 3);
    addKey(layout, "⇧", "SHIFT_R", 3, 13, 2);

    addKey(layout, "Ctrl",  "CTRL",   4, 0, 2);
    addKey(layout, "Alt",   "ALT",    4, 2, 2);
    addKey(layout, "Space", "SPACE",  4, 4, 7);
    addKey(layout, "Alt",   "ALT_R",  4, 11, 2);
    addKey(layout, "Ctrl",  "CTRL_R", 4, 13, 2);

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
    btn->setStyleSheet(STYLE_NORMAL);
    btn->setFocusPolicy(Qt::NoFocus);
    layout->addWidget(btn, row, col, 1, colSpan);
    m_keys[mapKey.toUpper()] = btn;
}

QString KeyboardWidget::resolveKey(const QString &ch) const
{
    if (ch.isEmpty()) return QString();
    if (ch == " ")   return "SPACE";

    static const QMap<QChar, QString> shiftMap = {
        {'!', "1"}, {'@', "2"}, {'#', "3"}, {'$', "4"}, {'%', "5"},
        {'^', "6"}, {'&', "7"}, {'*', "8"}, {'(', "9"}, {')', "0"},
        {'_', "-"}, {'+', "="}, {'~', "`"},
        {'{', "["}, {'}', "]"}, {'|', "\\"},
        {':', ";"}, {'"', "'"},
        {'<', ","}, {'>', "."}, {'?', "/"},
        {'A', "A"}, {'B', "B"}, {'C', "C"}, {'D', "D"}, {'E', "E"},
        {'F', "F"}, {'G', "G"}, {'H', "H"}, {'I', "I"}, {'J', "J"},
        {'K', "K"}, {'L', "L"}, {'M', "M"}, {'N', "N"}, {'O', "O"},
        {'P', "P"}, {'Q', "Q"}, {'R', "R"}, {'S', "S"}, {'T', "T"},
        {'U', "U"}, {'V', "V"}, {'W', "W"}, {'X', "X"}, {'Y', "Y"},
        {'Z', "Z"}
    };

    QChar c = ch[0];
    if (shiftMap.contains(c))
        return shiftMap[c];

    return ch.toUpper();
}

void KeyboardWidget::highlightNextKey(const QString &ch)
{
    clearHighlight();

    QString key = resolveKey(ch);
    if (key.isEmpty()) return;

    if (m_keys.contains(key)) {
        m_keys[key]->setStyleSheet(STYLE_HINT);
        m_highlightedKey = key;
    }
}

void KeyboardWidget::clearHighlight()
{
    if (!m_highlightedKey.isEmpty() && m_keys.contains(m_highlightedKey))
        m_keys[m_highlightedKey]->setStyleSheet(STYLE_NORMAL);
    m_highlightedKey.clear();
}

void KeyboardWidget::pressKey(const QString &key)
{
    QString k = key.toUpper();
    if (k == "BACKSPACE") k = "BACKSPACE";
    else if (k == "ENTER") k = "ENTER";
    else if (k == "SPACE" || key == " ") k = "SPACE";

    if (m_keys.contains(k))
        m_keys[k]->setStyleSheet(STYLE_PRESSED);
    else {
        QString resolved = resolveKey(key);
        if (!resolved.isEmpty() && m_keys.contains(resolved))
            m_keys[resolved]->setStyleSheet(STYLE_PRESSED);
    }
}

void KeyboardWidget::releaseKey(const QString &key)
{
    QString k = key.toUpper();
    if (k == "BACKSPACE") k = "BACKSPACE";
    else if (k == "ENTER") k = "ENTER";
    else if (k == "SPACE" || key == " ") k = "SPACE";

    if (m_keys.contains(k))
        m_keys[k]->setStyleSheet(m_highlightedKey == k ? STYLE_HINT : STYLE_NORMAL);
    else {
        QString resolved = resolveKey(key);
        if (!resolved.isEmpty() && m_keys.contains(resolved))
            m_keys[resolved]->setStyleSheet(m_highlightedKey == resolved ? STYLE_HINT : STYLE_NORMAL);
    }
}
