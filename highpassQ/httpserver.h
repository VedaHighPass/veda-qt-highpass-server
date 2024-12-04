#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include "DatabaseManager.h"

class HttpServer : public QTcpServer {
    Q_OBJECT

public:
    explicit HttpServer(DatabaseManager& dbManager, QObject* parent = nullptr);
    void startServer(quint16 port);

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private:
    DatabaseManager& dbManager;

    void handleRequest(QTcpSocket* socket);
    void sendResponse(QTcpSocket* socket, const QByteArray& body, int statusCode);
};

#endif // HTTPSERVER_H
