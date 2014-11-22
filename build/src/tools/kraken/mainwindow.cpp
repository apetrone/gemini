#include "mainwindow.h"

#include <QVBoxLayout>

#include <QMessageBox>
#include <QFileDialog>
#include <QDockWidget>

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
    properties_panel->setMinimumWidth(200);

    QDockWidget* log_console = new QDockWidget("log console");
    QWidget* log = new QWidget(log_console);
    QVBoxLayout* layout = new QVBoxLayout(log);
    layout->setMargin(5);

    QPlainTextEdit* plainTextEdit = new QPlainTextEdit();
    plainTextEdit->setReadOnly(true);
    plainTextEdit->setObjectName("logwindow");
    layout->addWidget(plainTextEdit);

    log_console->setWidget(log);

    this->addDockWidget(Qt::BottomDockWidgetArea, log_console);


    plainTextEdit->appendPlainText("log initialized");
    log_text_field = plainTextEdit;

    create_menus();
}

MainWindow::~MainWindow()
{

}

void MainWindow::create_menus()
{
    // create main menu
    menu_bar = new QMenuBar(this);
    QMenu* file_menu = new QMenu("&File", this);
    QAction* test = file_menu->addAction("&Open Project...");
    connect(test, SIGNAL(triggered()), this, SLOT(about()));
    file_menu->addAction(test);
    menu_bar->addMenu(file_menu);
}

void MainWindow::about()
{
    QString directory = QFileDialog::getExistingDirectory(this, tr("Choose Project Path"), "", QFileDialog::ShowDirsOnly);

    // TODO: redirect this to a log service
    if (log_text_field)
    {
        log_text_field->appendPlainText(directory);
    }

    // startup blacksmith
}
