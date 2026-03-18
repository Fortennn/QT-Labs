#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Helpdesk");
    app.setOrganizationName("Lab");

    MainWindow w;
    w.show();
    return app.exec();
}
