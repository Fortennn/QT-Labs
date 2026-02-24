#include "lessonloader.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QCoreApplication>

QString LessonLoader::lessonsDir()
{
    return QCoreApplication::applicationDirPath() + "/lessons";
}

QVector<LessonEntry> LessonLoader::scanLessons(const QString &dirPath)
{
    QVector<LessonEntry> result;

    QDir dir(dirPath);
    if (!dir.exists())
        return result;

    QStringList files = dir.entryList(QStringList{"*.txt"}, QDir::Files, QDir::Name);
    for (const QString &fileName : files) {
        QFileInfo fi(dir.filePath(fileName));
        LessonEntry entry;
        entry.title    = fi.baseName();
        entry.filePath = fi.absoluteFilePath();
        result.append(entry);
    }

    return result;
}

QString LessonLoader::loadLesson(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning("LessonLoader: cannot open '%s': %s",
                 qPrintable(filePath),
                 qPrintable(file.errorString()));
        return QString();
    }

    QTextStream in(&file);
    QString text = in.readAll();
    text.replace("\r\n", "\n");
    text.replace('\r', '\n');
    return text;
}
