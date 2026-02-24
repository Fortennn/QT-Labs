#ifndef LESSONLOADER_H
#define LESSONLOADER_H

#include <QString>
#include <QVector>

struct LessonEntry {
    QString title;
    QString filePath;
};

class LessonLoader
{
public:
    static QVector<LessonEntry> scanLessons(const QString &dirPath);
    static QString loadLesson(const QString &filePath);
    static QString lessonsDir();
};

#endif // LESSONLOADER_H
