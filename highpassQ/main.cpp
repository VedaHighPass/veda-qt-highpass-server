#include "mainwindow.h"

#include <QDir>
#include <QCoreApplication>
#include <QApplication>
#include "DatabaseManager.h"
#include "httpserver.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    //dbConnect
    DatabaseManager::instance().connectToDatabase(QString("C:/Users/3kati/Desktop/db_qt/veda-qt-highpass-server/highpassQ/gotomars.db"));
    //DatabaseManager::instance().connectToDatabase(QString("../highpassQ/gotomars.db"));

    HttpServer server(DatabaseManager::instance());
    server.startServer(8080);


    MainWindow w;
    w.show();
    return a.exec();
}
