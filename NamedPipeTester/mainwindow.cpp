#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    socket.connectToServer("\\\\.\\pipe\\ArmaDebugEnginePipeIface",QLocalSocket::ReadWrite);
    qDebug() << "connected";
    connect(&socket,&QLocalSocket::readyRead,this,&MainWindow::readyRead);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::readyRead()
{
    QByteArray read = socket.readAll();
    ui->textEdit->append(QString::fromUtf8(read) + "\n");
}

void MainWindow::on_button_addBP_clicked()
{
    QByteArray bb;
    //bb.fill(' ',5000); //Test bigger packet than buffersize
    bb += R"({"command" : 1,
             "data" : {
                         "action": {
                             "code": "systemChat \"BPCMDTEST\"",
                             "basePath" : "P:\\",
                             "type": 2
                         },
                         "condition": null,
                         "filename": "z\\ace\\addons\\explosives\\functions\\fnc_setupExplosive.sqf",
                         "line": 8,
                         "label" : "condSelect"
                    }
         })";



        /*
         *
         *  "filename": "A3\\functions_f\\arrays\\fn_conditionalSelect.sqf",
                         "line": 9,

                                 "filename": "z\\ace\\addons\\explosives\\functions\\fnc_setupExplosive.sqf",
                                 "line": 8,

        */


    socket.write(bb);
    qDebug() << "BP Added";
}

void MainWindow::on_button_Continue_clicked()
{
    QByteArray bb = R"({"command" : 3,
             "data" : {
                         "action": {
                             "code": "systemChat \"BPCMDTEST\"",
                             "type": 2
                         },
                         "condition": {
                             "code": "true",
                             "type": 1
                         },
                         "line": 8
                    }
         })";


    socket.write(bb);
    qDebug() << "Continued";
}

void MainWindow::on_button_dumpMonitor_clicked()
{
    QByteArray bb = R"({"command" : 4})";


    socket.write(bb);
    qDebug() << "Mons dumped";
}

void MainWindow::on_button_removeBP_clicked()
{
    QByteArray bb = R"({"command" : 2,
                    "data" : {
                    "filename": "z\\ace\\addons\\explosives\\functions\\fnc_setupExplosive.sqf",
                    "line": 8
                    }
         })";


    socket.write(bb);
    qDebug() << "BP Removed";
}

void MainWindow::on_button_disableHook_clicked()
{
    QByteArray bb = R"({"command" : 5,
                    "state" : 0
         })";


    socket.write(bb);
    qDebug() << "Hook disabled";
}

void MainWindow::on_button_enableHook_clicked()
{
    QByteArray bb = R"({"command" : 5,
                    "state" : 1
         })";


    socket.write(bb);
    qDebug() << "Hook enabled";
}

void MainWindow::on_button_Variable_clicked()
{
    QByteArray bb = R"({"command" : 6,
             "data" : {
                         "scope":  15,
                         "name":  ["ace_advanced_fatigue_loadFactor","_currentFrequency","_localNil","globalNil","_lVar"]
                    }
         })";


    socket.write(bb);
    qDebug() << "Var Got";
}

void MainWindow::on_button_stuff_clicked()
{
    QByteArray bb = R"({"command":1,"data":{"action":{"code":"","basePath":"","type":2},"condition":null,"filename":"test.sqf","line":11}})";



    socket.write(bb);
    qDebug() << "STUFFZ!!";
}
