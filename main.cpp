#include <QApplication>
#include "mainWindow.h"


int main(int argc, char *argv[]) {

    QApplication app(argc, argv);
    mainWindow main;

    main.start();
    int QT = app.exec();

    main.stop();
    return QT;
}