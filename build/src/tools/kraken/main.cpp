            #include "mainwindow.h"
#include <QApplication>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;

    // setup the window
    w.setWindowTitle("gemini.editor");
    w.resize(1280, 720);

    w.setVisible(true);
    w.show();

    return a.exec();
}

