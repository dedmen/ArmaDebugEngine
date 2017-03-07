#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFile>
#include <QTreeWidgetItem>
#include <QDebug>
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QFile f("P:\\12000.txt");
    f.open(QFile::ReadOnly);
    while (true) {
        QByteArray line = f.readLine(5555555);
        qDebug() << f.bytesAvailable();
        if (line.isEmpty()) break;
        auto string = QString::fromUtf8(line);
        auto tabs = string.lastIndexOf('\t');
        if (tabs == -1){
           ui->treeWidget->addTopLevelItem(new QTreeWidgetItem(QStringList{string.trimmed()}));
        } else {
            QTreeWidgetItem* lastItem =  ui->treeWidget->topLevelItem(ui->treeWidget->topLevelItemCount()-1);
            for (int i = 0; i < tabs;i++) {
                if (lastItem == nullptr) continue;
                lastItem =  lastItem->child(lastItem->childCount()-1);
            }
            if (lastItem == nullptr) continue;
           lastItem->addChild(new QTreeWidgetItem(QStringList{string.trimmed()}));
        }
    }










}

MainWindow::~MainWindow()
{
    delete ui;
}
