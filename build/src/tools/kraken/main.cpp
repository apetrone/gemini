#include "mainwindow.h"
#include <QApplication>
#include <QFile>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;

    // load styles
    QFile stylesheet(":/resources/stylesheet.qss");
    if (stylesheet.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        a.setStyleSheet(stylesheet.readAll());
        stylesheet.close();
    }

    // setup the window
    w.setWindowTitle("kraken");
    w.resize(1280, 720);

    w.setVisible(true);
    w.show();

    return a.exec();
}

