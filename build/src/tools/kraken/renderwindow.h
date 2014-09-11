#ifndef RENDERWINDOW_H
#define RENDERWINDOW_H

#include <QWidget>
#include <QGLWidget>
#include <QTimer>

class RenderWindow : public QGLWidget
{
    Q_OBJECT

public:
    RenderWindow(const QGLFormat& format,
                QWidget* parent = 0,
                const QGLWidget* share_widget = 0,
                Qt::WindowFlags flags = 0);

signals:

public slots:


protected:
    virtual void initializeGL();
    virtual void paintGL();
    virtual void resizeGL(int w, int h);

private:
    QTimer timer;
};

#endif // RENDERWINDOW_H
