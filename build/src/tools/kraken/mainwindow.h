#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMenuBar>
#include <QPlainTextEdit>
#include "renderwindow.h"


class MainWindow : public QMainWindow
{
    Q_OBJECT

private:
    RenderWindow* render_window;
    QMenuBar* menu_bar;
    QPlainTextEdit* log_text_field;

    void create_menus();

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void about();
};

#endif // MAINWINDOW_H
