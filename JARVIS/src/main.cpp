#include <QApplication>
#include <QDir>
#include <QDebug>
#include <QIcon>
#include "ui/MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    QApplication::setOrganizationName("JARVIS");
    QApplication::setOrganizationDomain("jarvis.local");
    QApplication::setApplicationName("JARVIS");
    QApplication::setApplicationVersion("1.0.0");

    QString basePath = QCoreApplication::applicationDirPath();
    qDebug() << "Application started in Portable mode at:" << basePath;

    app.setWindowIcon(QIcon("C:/Papki/qt-labs/JARVIS/assets/icon.png"));

    MainWindow w;
    w.show();

    return app.exec();
}
