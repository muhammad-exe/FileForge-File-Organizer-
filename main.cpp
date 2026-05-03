#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("FileForge");
    app.setApplicationDisplayName("FileForge");
    app.setOrganizationName("Salim Habib University");

    MainWindow window;
    window.show();
    return app.exec();
}