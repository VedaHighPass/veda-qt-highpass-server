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

        QString startDateStr = queryParams.queryItemValue("startDate");
        QString endDateStr = queryParams.queryItemValue("endDate");

        // Default to a wide range if dates are not provided
        if (startDateStr.isEmpty()) startDateStr = "1900-01-01"; // 기본 시작일
        if (endDateStr.isEmpty()) endDateStr = "2099-12-31";     // 기본 종료일

        // Validate the dates
        QDate startDate = QDate::fromString(startDateStr, "yyyy-MM-dd");
        QDate endDate = QDate::fromString(endDateStr, "yyyy-MM-dd");

        if (!startDate.isValid() || !endDate.isValid()) {
            sendResponse(socket, "Invalid date format. Use yyyy-MM-dd.", 400);
            return;
        }

        // Query database for records between the given dates
        QList<QVariantMap> records = DatabaseManager::instance().getRecordsByDateRange(startDate, endDate);

        QJsonArray jsonArray;
        for (const auto& record : records) {
            QJsonObject jsonObject;
            for (const auto& key : record.keys()) {
                jsonObject[key] = QJsonValue::fromVariant(record[key]); // 모든 필드 자동 추가
            }
            jsonArray.append(jsonObject);
        }

        QJsonDocument jsonDoc(jsonArray);
        sendResponse(socket, jsonDoc.toJson(), 200);
    }

    /*
    if (method == "GET" && path == "/records") {
        QList<QVariantMap> records = DatabaseManager::instance().getAllRecords(); // 데이터 가져오기
        QJsonArray jsonArray;

        for (const auto& record : records) {
            QJsonObject jsonObject;
            jsonObject["ID"] = record["ID"].toInt();
            jsonObject["EntryTime"] = record["EntryTime"].toString();
            jsonObject["PlateNumber"] = record["PlateNumber"].toString();
            jsonObject["GateNumber"] = record["GateNumber"].toInt();
            jsonArray.append(jsonObject);
        }

        QJsonDocument jsonDoc(jsonArray);
        sendResponse(socket, jsonDoc.toJson(), 200);
    } else if (method == "GET" && path == "/gates") {
        // GATELIST 테이블에서 데이터 가져오기
        QList<QVariantMap> gates = DatabaseManager::instance().getAllGates(); // GATELIST 데이터 가져오기
        QJsonArray jsonArray;

        for (const auto& gate : gates) {
            QJsonObject jsonObject;
            jsonObject["GateNumber"] = gate["GateNumber"].toInt();
            jsonObject["GateName"] = gate["GateName"].toString();
            jsonObject["isEnterGate"] = gate["isEnterGate"].toBool();
            jsonObject["isExitGate"] = gate["isExitGate"].toBool();
            jsonArray.append(jsonObject);
        }

        QJsonDocument jsonDoc(jsonArray);
        sendResponse(socket, jsonDoc.toJson(), 200);
    } else if (method == "POST" && path == "/records") {
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
    }*/

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
