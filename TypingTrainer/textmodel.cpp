#include "textmodel.h"

QVector<Lesson> TextModel::builtinLessons()
{
    QVector<Lesson> lessons;

    lessons.append({"Starter Text",
        "Practice basic home row keys: a s d f j k l ;",
        "jfj j jff ffjjjj fjjf fjjj jjjjjjj\n"
        "jfj j jff ffjjjj fjn ffj\n"
        "asdf jkl; asdf jkl; asdf jkl;\n"
        "aa ss dd ff jj kk ll ;;\n"
        "fj fj fj jf jf jf fjfjfj"});

    lessons.append({"Numbers and Symbols",
        "Practice digits and common punctuation marks",
        "1234567890 !@#$%^&*()\n"
        "abc123 def456 ghi789\n"
        "price: $9.99, qty: 42, total: $419.58\n"
        "phone: +1 (800) 555-1234\n"
        "date: 2025-01-19, time: 14:30:00"});

    lessons.append({"Common Words Part 1",
        "Practice the most frequent English words",
        "the quick brown fox jumps over the lazy dog\n"
        "a man a plan a canal panama\n"
        "to be or not to be that is the question\n"
        "she sells sea shells by the sea shore\n"
        "how much wood would a woodchuck chuck"});

    lessons.append({"Common Words Part 2",
        "Sentences with punctuation and mixed case",
        "Hello, World! This is a typing trainer.\n"
        "Practice makes perfect, so keep going.\n"
        "Qt is a cross-platform application framework.\n"
        "C++ offers performance and control over memory.\n"
        "Good code is clear, correct, and maintainable."});

    lessons.append({"Python Code Sample",
        "Practice typing real source code",
        "def greet(name):\n"
        "    print(f\"Hello, {name}!\")\n"
        "\n"
        "for i in range(10):\n"
        "    greet(f\"user_{i}\")\n"
        "\n"
        "result = [x * x for x in range(1, 6)]"});

    lessons.append({"Lorem Ipsum",
        "Classic placeholder text for longer practice",
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit.\n"
        "Sed do eiusmod tempor incididunt ut labore et dolore magna.\n"
        "Ut enim ad minim veniam, quis nostrud exercitation.\n"
        "Duis aute irure dolor in reprehenderit in voluptate velit.\n"
        "Excepteur sint occaecat cupidatat non proident sunt in culpa."});

    return lessons;
}

TextModel::TextModel()
{
    auto lessons = builtinLessons();
    if (!lessons.isEmpty())
        loadText(lessons.first().text);
}

void TextModel::loadText(const QString &text)
{
    m_fullText = text;
    m_fullText.replace("\r\n", "\n");
    m_fullText.replace('\r', '\n');
    m_lines = m_fullText.split('\n');
    if (m_lines.isEmpty())
        m_lines.append(QString());
    reset();
}

void TextModel::reset()
{
    m_lineIndex = 0;
    m_charIndex = 0;
    clampIndices();
}

void TextModel::clampIndices()
{
    if (m_lines.isEmpty()) { m_lineIndex = 0; m_charIndex = 0; return; }
    if (m_lineIndex < 0) m_lineIndex = 0;
    if (m_lineIndex >= m_lines.size()) m_lineIndex = m_lines.size() - 1;
    int lineLen = m_lines[m_lineIndex].size();
    if (m_charIndex < 0) m_charIndex = 0;
    if (m_charIndex > lineLen) m_charIndex = lineLen;
}

bool TextModel::isFinished() const
{
    if (m_lines.isEmpty()) return true;
    return (m_lineIndex == m_lines.size() - 1) &&
           (m_charIndex >= m_lines[m_lineIndex].size());
}

QString TextModel::currentLine() const
{
    if (m_lines.isEmpty() || m_lineIndex < 0 || m_lineIndex >= m_lines.size())
        return QString();
    return m_lines[m_lineIndex];
}

QString TextModel::previousLine() const
{
    if (m_lineIndex <= 0) return QString();
    return m_lines[m_lineIndex - 1];
}

QString TextModel::typedPart() const
{
    QString line = currentLine();
    if (line.isEmpty() || m_charIndex <= 0) return QString();
    return line.left(m_charIndex);
}

QString TextModel::remainingPart() const
{
    QString line = currentLine();
    if (line.isEmpty() || m_charIndex >= line.size()) return QString();
    return line.mid(m_charIndex);
}

QChar TextModel::expectedChar() const
{
    QString line = currentLine();
    if (m_charIndex < line.size())
        return line[m_charIndex];
    return QChar();
}

bool TextModel::advance()
{
    if (isFinished()) return false;
    QString line = currentLine();
    m_charIndex++;
    if (m_charIndex >= line.size() && m_lineIndex < m_lines.size() - 1) {
        m_lineIndex++;
        m_charIndex = 0;
    }
    clampIndices();
    return true;
}

bool TextModel::goBack()
{
    if (m_lineIndex == 0 && m_charIndex == 0) return false;
    if (m_charIndex > 0) {
        m_charIndex--;
    } else if (m_lineIndex > 0) {
        m_lineIndex--;
        m_charIndex = m_lines[m_lineIndex].size();
        clampIndices();
    }
    return true;
}
