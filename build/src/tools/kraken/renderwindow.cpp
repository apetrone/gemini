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

#include "renderwindow.h"

#include <renderer/renderer.h>
#include <renderer/renderstream.h>

#if defined(__APPLE__)
#include <OpenGL/gl3.h>
#endif

using namespace renderer;

RenderWindow::RenderWindow(
        const QGLFormat& format,
        QWidget* parent,
        const QGLWidget* share_widget,
        Qt::WindowFlags flags) :
    QGLWidget(format)
{
    connect(&timer, SIGNAL(timeout()), this, SLOT(updateGL()));
    timer.start(16);
}


void RenderWindow::initializeGL()
{
}

void RenderWindow::paintGL()
{
    RenderStream rs;
    rs.add_clearcolor(0.15f, 0.15f, 0.35f, 1.0f);
    rs.add_clear(renderer::CLEAR_COLOR_BUFFER | renderer::CLEAR_DEPTH_BUFFER);
    rs.run_commands();
}

void RenderWindow::resizeGL(int w, int h)
{    
    RenderStream rs;
    rs.add_viewport(0, 0, w, h);
    rs.run_commands();
}
