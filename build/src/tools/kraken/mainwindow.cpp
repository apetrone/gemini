#include "mainwindow.h"

#include <QVBoxLayout>
#include <QDockWidget>
#include <QPlainTextEdit>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{

    QGLFormat format;
    format.setVersion(3, 2);
    format.setProfile(QGLFormat::CoreProfile);

    render_window = new RenderWindow(format, this);
    render_window->show();
    this->setCentralWidget(render_window);


//    QWidget* widget = new QWidget(this);
//    this->setCentralWidget(widget);

    QDockWidget* dockWidget1 = new QDockWidget("preview");
    this->setDockNestingEnabled(true);

    render_window->setMinimumWidth(200);
//    dockWidget1->setWidget(render_window);
    this->addDockWidget(Qt::RightDockWidgetArea, dockWidget1);


    QDockWidget* properties_panel = new QDockWidget("properties");
    this->addDockWidget(Qt::RightDockWidgetArea, properties_panel);

    QDockWidget* log_console = new QDockWidget("log console");
    QWidget* log = new QWidget(log_console);
    QVBoxLayout* layout = new QVBoxLayout(log);
    layout->setMargin(5);

    QPlainTextEdit* plainTextEdit = new QPlainTextEdit();
    plainTextEdit->setReadOnly(true);
    plainTextEdit->setStyleSheet("QAbstractScrollArea { background-color: black; color: red }");
    layout->addWidget(plainTextEdit);

    log_console->setWidget(log);

    this->addDockWidget(Qt::BottomDockWidgetArea, log_console);


    plainTextEdit->appendPlainText("log initialized");
}

MainWindow::~MainWindow()
{

}
