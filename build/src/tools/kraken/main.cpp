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

#include "applicationcontext.h"
#include "mainwindow.h"
#include <QApplication>
#include <QFile>

#include "common.h" // TODO: move this to "tools" ?

using namespace gemini;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    tools::startup("arcfusion.net/orion");
    ApplicationContext context;

    // load styles
    QFile stylesheet(":/resources/stylesheet.qss");
    if (stylesheet.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        a.setStyleSheet(stylesheet.readAll());
        stylesheet.close();
    }


    // setup the window
    MainWindow w(&context);
    w.setWindowTitle("kraken");
    w.resize(1280, 720);

    w.setVisible(true);
    w.show();

    context.startup();

    int result = a.exec();

    context.shutdown();

    tools::shutdown();

    return result;
}

