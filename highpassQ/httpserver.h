#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include "DatabaseManager.h"
#include <QSslSocket>
#include <QSslKey>
#include <QSslCertificate>

class HttpServer : public QTcpServer {
    Q_OBJECT

public:
    explicit HttpServer(DatabaseManager& dbManager, QObject* parent = nullptr);
    void startServer(quint16 port);

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private:
    DatabaseManager& dbManager;

    void handleRequest(QSslSocket* socket);
    void sendResponse(QSslSocket* socket, const QByteArray& body, int statusCode);
    void decodeBase64AndSaveToFile(const std::string& encoded, const std::string& filename);
private slots:
    void onEncrypted() {
            qDebug() << "SSL connection established!";
    }
    void onReadyRead();
};

#endif // HTTPSERVER_H
