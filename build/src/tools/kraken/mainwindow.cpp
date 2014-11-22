#include "mainwindow.h"

#include <QVBoxLayout>

#include <QMessageBox>
#include <QFileDialog>
#include <QDockWidget>
#include <QApplication>
#include <QProcess>

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
    QString project_path = QFileDialog::getExistingDirectory(this, tr("Choose Project Path"), "", QFileDialog::ShowDirsOnly);
    project_path.append(QDir::separator());
    project_path.append("assets");

    // TODO: redirect this to a log service
    if (log_text_field)
    {
        log_text_field->appendPlainText(project_path);
    }
    else
    {
        return;
    }

    // startup blacksmith


    // TODO: if in development
    QDir dir = QDir(QDir::currentPath());
    dir.cd("../../../");

    // TODO: if on Mac
    dir.cd("../../../");

    QString engine_root = dir.path() + QDir::separator();
    log_text_field->appendPlainText(engine_root);

    QProcess process;

    QDir::setCurrent(engine_root);

    // TODO: need to setup virtualenv
    QString blacksmith_script_path = engine_root + "tools/blacksmith/blacksmith.py";
    QString config_path = engine_root + "tools/conf/blacksmith/desktop.conf";
    QString blacksmith = "python " + blacksmith_script_path + " -c " + config_path + " -y -s " + project_path;


    log_text_field->appendPlainText(blacksmith);
    process.start(blacksmith);
    process.waitForFinished();
    log_text_field->appendPlainText("process finished");

    log_text_field->appendPlainText(process.readAllStandardOutput());
    log_text_field->appendPlainText(process.readAllStandardError());
}
