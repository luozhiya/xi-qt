#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>

namespace xi {
class Editor;
class XiBridge;
}

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_actionOpen_triggered();

    void on_actionClose_triggered();

private:
    void init();

    Ui::MainWindow *ui;
    QStackedWidget stack_;
    xi::Editor* editor_;
    xi::XiBridge* bridge_;
};

#endif // MAINWINDOW_H
