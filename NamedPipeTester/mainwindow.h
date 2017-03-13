#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLocalSocket>
#include <QDebug>
#include <QThread>
namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    QLocalSocket socket;
    void readyRead();
private slots:
    void on_button_addBP_clicked();

    void on_button_Continue_clicked();

    void on_button_dumpMonitor_clicked();

    void on_button_removeBP_clicked();

    void on_button_disableHook_clicked();

    void on_button_enableHook_clicked();

    void on_button_Variable_clicked();

    void on_button_stuff_clicked();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
