#include <QApplication>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIcon>
#include "ui/MainWindow.h"

namespace {

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

} // namespace

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QApplication::setOrganizationName(QStringLiteral("JARVIS"));
    QApplication::setOrganizationDomain(QStringLiteral("jarvis.local"));
    QApplication::setApplicationName(QStringLiteral("JARVIS"));
    QApplication::setApplicationVersion(QStringLiteral("1.0.0"));

    const QIcon appIcon = loadAppIcon();
    if (!appIcon.isNull()) QApplication::setWindowIcon(appIcon);

    qDebug() << "[JARVIS] booted from" << QCoreApplication::applicationDirPath();

    MainWindow w;
    if (!appIcon.isNull()) w.setWindowIcon(appIcon);
    w.show();

    return app.exec();
}
