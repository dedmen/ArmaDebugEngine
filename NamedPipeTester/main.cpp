#include <QCoreApplication>
#include <QLocalSocket>
#include <QDebug>
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QLocalSocket socket;
    socket.connectToServer("\\\\.\\pipe\\ArmaDebugEnginePipeIface",QLocalSocket::ReadWrite);
    while (true){
        QByteArray bb;bb.fill('t',5000);

        socket.write(bb);
        socket.waitForReadyRead(50000);
        qDebug() << socket.readAll();
    }

    return a.exec();
}
