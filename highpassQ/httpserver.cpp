#include "httpserver.h"
#include <QDebug>
#include <QJsonArray>
#include <QFile>
#include <QString>
#include <QStringList>
#include <QTcpServer>
#include <QUrlQuery>
HttpServer::HttpServer(DatabaseManager& dbManager, QObject* parent)
    : QTcpServer(parent), dbManager(dbManager) {}

void HttpServer::startServer(quint16 port) {
    if (!this->listen(QHostAddress::Any, port)) {
        qCritical() << "Failed to start server on port" << port;
    } else {
        qDebug() << "Server started on port" << port;
    }
}

void HttpServer::incomingConnection(qintptr socketDescriptor) {
    QTcpSocket* socket = new QTcpSocket();
    if (!socket->setSocketDescriptor(socketDescriptor)) {
        qDebug() << "Failed to set socket descriptor";
        socket->deleteLater();
        return;
    }

    connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
        handleRequest(socket);
    });
    connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
}

// 안쓰는듯..?
QList<QVariantMap> convertToQVariantMapList(const QStringList& stringList) {
    QList<QVariantMap> result;

    for (const QString& str : stringList) {
        QStringList fields = str.split(","); // Assuming fields are comma-separated
        if (fields.size() < 4) {
            qWarning() << "Skipping malformed entry:" << str;
            continue;
        }

        QVariantMap map;
        map["ID"] = fields[0].trimmed();
        map["EntryTime"] = fields[1].trimmed();
        map["PlateNumber"] = fields[2].trimmed();
        map["GateNumber"] = fields[3].trimmed();
        result.append(map);
    }

    return result;
}


void HttpServer::handleRequest(QTcpSocket* socket) {
    QByteArray requestData = socket->readAll();
    QString request = QString::fromUtf8(requestData);

    // Parse HTTP Request (simple parser for GET and POST)
    QString method, path;
    QTextStream stream(&request);
    stream >> method >> path;

    if (method == "GET" && path.startsWith("/records")) {
        // http://localhost:8080/records?startDate=2024-11-01&endDate=2024-11-30
        QUrl url(path);
        QUrlQuery queryParams(url.query());

        // Query Parameters
        QString startDateStr = queryParams.queryItemValue("startDate");
        QString endDateStr = queryParams.queryItemValue("endDate");
        QString plateNumber = queryParams.queryItemValue("plateNumber");
        QString entryGateStr = queryParams.queryItemValue("entryGate");
        QString exitGateStr = queryParams.queryItemValue("exitGate");

        // Validate Dates
        QDate startDate = QDate::fromString(startDateStr, "yyyy-MM-dd");
        QDate endDate = QDate::fromString(endDateStr, "yyyy-MM-dd");

        if (!startDate.isValid() || !endDate.isValid()) {
            sendResponse(socket, "Invalid date format. Use yyyy-MM-dd.", 400);
            return;
        }

        // Parse entryGate and exitGate as lists of integers
        QStringList entryGateList = entryGateStr.split(",", Qt::SkipEmptyParts);
        QList<int> entryGates;
        for (const QString& gate : entryGateList) {
            bool ok;
            int gateNumber = gate.toInt(&ok);
            if (ok) {
                entryGates.append(gateNumber);
            }
        }

        QStringList exitGateList = exitGateStr.split(",", Qt::SkipEmptyParts);
        QList<int> exitGates;
        for (const QString& gate : exitGateList) {
            bool ok;
            int gateNumber = gate.toInt(&ok);
            if (ok) {
                exitGates.append(gateNumber);
            }
        }

        // Query database
        QList<QVariantMap> records = DatabaseManager::instance().getRecordsByFilters(
            startDate, endDate, plateNumber, entryGates, exitGates
            );

        // Convert records to JSON
        QJsonArray jsonArray;
        for (const auto& record : records) {
            QJsonObject jsonObject;
            for (const auto& key : record.keys()) {
                jsonObject[key] = QJsonValue::fromVariant(record[key]);
            }
            jsonArray.append(jsonObject);
        }

        QJsonDocument jsonDoc(jsonArray);
        sendResponse(socket, jsonDoc.toJson(), 200);
    }
/*
    else if (method == "POST" && path == "/records") {
        QString body = request.split("\r\n\r\n").last();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(body.toUtf8());
        QJsonObject jsonObject = jsonDoc.object();

        QString entryTime = jsonObject.value("EntryTime").toString();
        QString plateNumber = jsonObject.value("PlateNumber").toString();
        int gateNumber = jsonObject.value("GateNumber").toInt();

        //add record in HIGHPASS_RECORD table.
        int newRecordID = DatabaseManager::instance().addHighPassRecord(entryTime, plateNumber, gateNumber);
        if (newRecordID != -1) {
            sendResponse(socket, "Record added successfully", 200);

            //insert data in BILL table.
            if(dbManager.checkIsEnterGate(gateNumber)){
                dbManager.insertEnterStepBill(plateNumber, gateNumber, newRecordID);
            }else {
                dbManager.insertExitStepBill(plateNumber, gateNumber, newRecordID);
            }

        } else {
            sendResponse(socket, "Failed to add record", 500);
        }
    }
*/
    else {
        sendResponse(socket, "Not Found", 404);
    }
}

void HttpServer::sendResponse(QTcpSocket* socket, const QString& response, int statusCode) {
    QString statusMessage = (statusCode == 200) ? "OK" :
                            (statusCode == 404) ? "Not Found" : "Internal Server Error";

    QByteArray httpResponse = QString("HTTP/1.1 %1 %2\r\n"
                                      "Content-Type: application/json\r\n"
                                      "Content-Length: %3\r\n"
                                      "\r\n%4")
                                  .arg(statusCode)
                                  .arg(statusMessage)
                                  .arg(response.size())
                                  .arg(response)
                                  .toUtf8();
    socket->write(httpResponse);
    socket->flush();
    socket->disconnectFromHost();
}
