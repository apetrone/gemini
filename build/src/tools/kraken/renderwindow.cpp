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

#include "renderwindow.h"

#include <renderer/renderer.h>
#include <renderer/renderstream.h>

#if defined(__APPLE__)
#include <OpenGL/gl3.h>
#endif

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
