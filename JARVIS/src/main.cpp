#include <QApplication>
#include <QDir>
#include <QDebug>
#include "ui/MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    QApplication::setApplicationName("AI Portable Assistant");
    QApplication::setApplicationVersion("1.0.0");

    QString basePath = QCoreApplication::applicationDirPath();
    qDebug() << "Application started in Portable mode at:" << basePath;

    MainWindow w;
    w.show();

    return app.exec();
}
