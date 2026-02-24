#ifndef TEXTMODEL_H
#define TEXTMODEL_H

#include <QString>
#include <QStringList>
#include <QVector>

struct Lesson {
    QString name;
    QString description;
    QString text;
};

class TextModel
{
public:
    TextModel();

    static QVector<Lesson> builtinLessons();

    void loadText(const QString &text);
    void reset();

    bool isFinished() const;
    int  lineIndex()  const { return m_lineIndex; }
    int  charIndex()  const { return m_charIndex; }
    int  lineCount()  const { return m_lines.size(); }

    QString currentLine()  const;
    QString previousLine() const;

    QString typedPart()     const;
    QString remainingPart() const;

    QChar   expectedChar()  const;

    bool advance();
    bool goBack();

    const QString     &fullText() const { return m_fullText; }
    const QStringList &lines()    const { return m_lines; }

private:
    QString     m_fullText;
    QStringList m_lines;
    int         m_lineIndex = 0;
    int         m_charIndex = 0;

    void clampIndices();
};

#endif // TEXTMODEL_H
