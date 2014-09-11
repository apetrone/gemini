#include "renderwindow.h"


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
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void RenderWindow::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}
