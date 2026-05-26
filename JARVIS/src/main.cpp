#include <QApplication>
#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIcon>
#include <QMessageBox>
#include <QMutex>
#include <QStandardPaths>
#include <QtGlobal>

#include <cstdio>
#include <cstdlib>
#include <exception>

#include "ui/MainWindow.h"

namespace {

// =============================================================================
//  File logger — критично для діагностики «не запускається на чужому ПК»
// =============================================================================
//
// На Windows збірки з прапором WIN32 не мають консолі, тому будь-який вивід
// у stdout/stderr (включаючи qDebug() / qWarning() / qCritical()) йде в нікуди.
// Без логу користувач, у якого впав .exe ще до відображення вікна (типовий
// випадок з відсутнім Qt6Core.dll, qwindows.dll, MSVC runtime), не має жодних
// підказок — лише німе закриття вікна.
//
// Лог пишеться у:
//   <exe-dir>/jarvis.log         — пріоритет, бо «лежить поруч»
//   або, якщо туди немає прав запису:
//   %LOCALAPPDATA%/JARVIS/jarvis.log
//
// Файл обрізається до ~1 MB на старті — щоб лог не розпухав на сотні MB.

QFile* g_logFile = nullptr;
QMutex g_logMutex;

QString pickLogPath() {
    const QString exeDir = QCoreApplication::applicationDirPath();
    const QString primary = exeDir + QStringLiteral("/jarvis.log");

    // Спершу пробуємо «поруч з .exe» — портативний варіант.
    QFile probe(primary);
    if (probe.open(QIODevice::Append | QIODevice::Text)) {
        probe.close();
        return primary;
    }

    // Фолбек у LOCALAPPDATA / XDG_DATA_HOME.
    const QString appData = QStandardPaths::writableLocation(
        QStandardPaths::AppDataLocation);
    if (!appData.isEmpty()) {
        QDir().mkpath(appData);
        return appData + QStringLiteral("/jarvis.log");
    }
    return primary;
}

void rotateIfHuge(const QString& path) {
    QFileInfo fi(path);
    if (!fi.exists()) return;
    if (fi.size() < 1024 * 1024) return;
    const QString backup = path + QStringLiteral(".old");
    QFile::remove(backup);
    QFile::rename(path, backup);
}

void messageHandler(QtMsgType type,
                    const QMessageLogContext& ctx,
                    const QString& msg)
{
    const char* level = "INFO ";
    switch (type) {
    case QtDebugMsg:    level = "DEBUG"; break;
    case QtInfoMsg:     level = "INFO "; break;
    case QtWarningMsg:  level = "WARN "; break;
    case QtCriticalMsg: level = "ERROR"; break;
    case QtFatalMsg:    level = "FATAL"; break;
    }

    const QString line = QStringLiteral("[%1] [%2] %3")
        .arg(QDateTime::currentDateTime().toString(Qt::ISODateWithMs),
             QLatin1String(level),
             msg);

    QString full = line;
    if (ctx.file && type >= QtWarningMsg) {
        full += QStringLiteral("   (%1:%2)")
                    .arg(QString::fromUtf8(ctx.file)).arg(ctx.line);
    }
    full += QChar('\n');

    QMutexLocker locker(&g_logMutex);
    if (g_logFile && g_logFile->isOpen()) {
        // UTF-8 без BOM. Записуємо як байти, щоб уникнути різниці між Qt5 та
        // Qt6 у QTextStream::setCodec / QStringConverter API.
        g_logFile->write(full.toUtf8());
        g_logFile->flush();
    }

    // Дублюємо у stderr на випадок, якщо запустили з консолі (linux/debug).
    std::fprintf(stderr, "%s\n", line.toUtf8().constData());

    if (type == QtFatalMsg) {
        std::abort();
    }
}

void installFileLogger() {
    const QString path = pickLogPath();
    rotateIfHuge(path);

    g_logFile = new QFile(path);
    if (!g_logFile->open(QIODevice::Append | QIODevice::Text)) {
        delete g_logFile;
        g_logFile = nullptr;
        // Не критично — просто залишимось без файлу.
        std::fprintf(stderr, "[JARVIS] Failed to open log file: %s\n",
                     path.toUtf8().constData());
    }

    qInstallMessageHandler(messageHandler);
    qInfo() << "[JARVIS] ===== application start =====";
    qInfo() << "[JARVIS] log file:" << path;
    qInfo() << "[JARVIS] exe dir:"
            << QCoreApplication::applicationDirPath();
    qInfo() << "[JARVIS] Qt version (runtime):" << qVersion();
}

// Resolve the application icon at runtime — never hard-code an absolute
// path. We try, in order:
//   1) Qt Resource System (`:/assets/icon.png`) — works in any deployed exe
//      provided a .qrc with that path is compiled in.
//   2) `<exe-dir>/assets/icon.png` — portable layout, works regardless of
//      the user's current working directory.
//   3) `<exe-dir>/icon.png`         — flat layout fallback.
QIcon loadAppIcon() {
    if (QFile::exists(QStringLiteral(":/assets/icon.png"))) {
        return QIcon(QStringLiteral(":/assets/icon.png"));
    }
    const QString base = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
        base + QStringLiteral("/assets/icon.png"),
        base + QStringLiteral("/icon.png"),
        base + QStringLiteral("/../assets/icon.png"),
    };
    for (const QString& p : candidates) {
        if (QFileInfo::exists(p)) return QIcon(p);
    }
    qWarning() << "[JARVIS] icon.png not found near" << base
               << "or in :/assets — running without a window icon.";
    return QIcon();
}

// Перетворюємо std::exception на повідомлення в QMessageBox + лог. Без цього
// типовий C++ unhandled exception у GUI-додатку призведе до тихого падіння.
int runMain(int argc, char* argv[]) {
    QApplication app(argc, argv);

    QApplication::setOrganizationName(QStringLiteral("JARVIS"));
    QApplication::setOrganizationDomain(QStringLiteral("jarvis.local"));
    QApplication::setApplicationName(QStringLiteral("JARVIS"));
    QApplication::setApplicationVersion(QStringLiteral("1.0.0"));

    installFileLogger();

    const QIcon appIcon = loadAppIcon();
    if (!appIcon.isNull()) QApplication::setWindowIcon(appIcon);

    qInfo() << "[JARVIS] booted from" << QCoreApplication::applicationDirPath();

    MainWindow w;
    if (!appIcon.isNull()) w.setWindowIcon(appIcon);
    w.show();

    return app.exec();
}

} // namespace

int main(int argc, char *argv[]) {
    try {
        return runMain(argc, argv);
    } catch (const std::exception& e) {
        qCritical() << "[JARVIS] Fatal unhandled C++ exception:" << e.what();
        // На цьому етапі QApplication, ймовірно, ще живий — спробуємо показати
        // діалог. Якщо ні, qCritical вище принаймні залишить слід у лозі.
        if (QApplication::instance()) {
            QMessageBox::critical(
                nullptr,
                QStringLiteral("JARVIS"),
                QStringLiteral("Критична помилка: %1\n\n"
                               "Деталі дивіться у файлі jarvis.log поряд з .exe.")
                    .arg(QString::fromUtf8(e.what())));
        }
        return 1;
    } catch (...) {
        qCritical() << "[JARVIS] Fatal unhandled non-std exception.";
        if (QApplication::instance()) {
            QMessageBox::critical(
                nullptr,
                QStringLiteral("JARVIS"),
                QStringLiteral("Невідома критична помилка. "
                               "Дивись jarvis.log поряд з .exe."));
        }
        return 2;
    }
}
