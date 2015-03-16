// -------------------------------------------------------------
// Copyright (C) 2014- Adam Petrone
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//      * Redistributions of source code must retain the above copyright notice,
//      this list of conditions and the following disclaimer.

//      * Redistributions in binary form must reproduce the above copyright notice,
//      this list of conditions and the following disclaimer in the documentation
//      and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//       SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
    render_window->setMinimumWidth(200);
    this->setCentralWidget(render_window);


//    QWidget* widget = new QWidget(this);
//    this->setCentralWidget(widget);

//    QDockWidget* dockWidget1 = new QDockWidget("preview");
    this->setDockNestingEnabled(true);


//    dockWidget1->setWidget(render_window);
//    this->addDockWidget(Qt::RightDockWidgetArea, dockWidget1);


//    QDockWidget* properties_panel = new QDockWidget("properties");
//    this->addDockWidget(Qt::RightDockWidgetArea, properties_panel);
//    properties_panel->setMinimumWidth(200);

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

    context->log()->log("current working directory: " + QDir::currentPath());

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
