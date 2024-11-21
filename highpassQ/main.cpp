#include "mainwindow.h"

#include <QApplication>
#include "DatabaseManager.h"
#include "httpserver.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    //dbConnect
    DatabaseManager::instance().connectToDatabase(QString("../veda-qt-highpass-server/highpassQ/gotomars.db"));

    HttpServer server(DatabaseManager::instance());
    server.startServer(8080);


    MainWindow w;
    w.show();
    return a.exec();
}
