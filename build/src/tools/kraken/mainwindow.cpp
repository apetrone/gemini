// -------------------------------------------------------------
// Copyright (C) 2014- Adam Petrone

// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM,OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
// -------------------------------------------------------------

#include "mainwindow.h"

#include <QVBoxLayout>

#include <QMessageBox>
#include <QFileDialog>
#include <QDockWidget>
#include <QApplication>
#include <QProcess>

MainWindow::MainWindow(ApplicationContext* application_context, QWidget *parent)
    : QMainWindow(parent)
{
    context = application_context;

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

    context->log()->set_target_widget(plainTextEdit);
    context->log()->log("log initialized");

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

    context->log()->log("setting project path to: " + project_path);
    context->assets()->start_watching_assets(project_path);
}
