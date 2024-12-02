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

void HttpServer::handleRequest(QTcpSocket* socket) {
    QByteArray requestData = socket->readAll();
    QString request = QString::fromUtf8(requestData);

    // Parse HTTP Request (simple parser for GET and POST)
    QString method, path;
    QTextStream stream(&request);
    stream >> method >> path;

    if (method == "GET" && path.startsWith("/records")) {
        // http://127.0.0.1:8080/records?startDate=2024-11-13&endDate=2024-11-23&pageSize=10&page=1
        QUrl url(path);
        QUrlQuery queryParams(url.query());

        // Query Parameters
        QString startDateStr = queryParams.queryItemValue("startDate");
        QString endDateStr = queryParams.queryItemValue("endDate");
        QString plateNumber = queryParams.queryItemValue("plateNumber");
        QString entryGateStr = queryParams.queryItemValue("entryGate");
        QString exitGateStr = queryParams.queryItemValue("exitGate");
        int pageSize = queryParams.queryItemValue("pageSize").toInt();
        int page = queryParams.queryItemValue("page").toInt();

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

        // Fetch records using the updated DatabaseResult structure
        DatabaseResult result = DatabaseManager::instance().getRecordsByFilters(
            startDate, endDate, plateNumber, entryGates, exitGates, pageSize, page
            );

        // JSON 변환 및 응답 생성
        QJsonArray jsonArray;
        for (const auto& record : result.records) {
            QJsonObject jsonObject;
            for (const auto& key : record.keys()) {
                jsonObject[key] = QJsonValue::fromVariant(record[key]);
            }
            jsonArray.append(jsonObject);
        }

        // 전체 레코드 수와 데이터를 포함한 응답 생성
        QJsonObject response;
        response["data"] = jsonArray;
        response["totalRecords"] = result.totalRecords;

        QJsonDocument jsonDoc(response);
        sendResponse(socket, jsonDoc.toJson(), 200);
    } else if (method == "GET" && path.startsWith("/images/")){
        // 요청된 경로를 실제 파일 시스템 경로로 변환
        QString filePath = QString("C:/Users/3kati/Desktop/db_qt/images") + path.mid(7); // "/images/" 이후 경로 추가
        QFile file(filePath);

        // 파일 존재 여부 확인
        if (!file.exists()) {
            sendResponse(socket, "Image not found", 404);
            return;
        }

        // 파일 열기
        if (!file.open(QIODevice::ReadOnly)) {
            sendResponse(socket, "Failed to open image", 500);
            return;
        }

        QByteArray imageData = file.readAll();
        file.close();

        // 이미지 데이터를 반환
        QByteArray httpResponse = QString("HTTP/1.1 200 OK\r\n"
                                          "Content-Type: image/jpeg\r\n"
                                          "Content-Length: %1\r\n\r\n")
                                      .arg(imageData.size())
                                      .toUtf8();
        httpResponse.append(imageData);
        socket->write(httpResponse);
        socket->flush();
        socket->disconnectFromHost();
        return;
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
