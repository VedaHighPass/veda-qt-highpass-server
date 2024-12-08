#include "httpserver.h"
#include <QDebug>
#include <QJsonArray>
#include <QFile>
#include <QString>
#include <QStringList>
#include <QTcpServer>
#include <QFile>
#include <QDir>
#include <QByteArray>
#include <QUrlQuery>
#include <opencv2/opencv.hpp>
#include <QDebug>
#include <base64.h>
#include <QSslSocket>
#include <QSslKey>
#include <QSslCertificate>


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
    // SSL 소켓 생성
    QSslSocket* sslSocket = new QSslSocket(this);

    // 소켓 디스크립터 설정
    if (!sslSocket->setSocketDescriptor(socketDescriptor)) {
        qDebug() << "Failed to set socket descriptor";
        sslSocket->deleteLater();
        return;
    }

    // 인증서 및 키 설정
    QSslCertificate certificate;
    QSslKey privateKey;

    // 인증서 및 키 파일 로드
    QFile certFile("../server.crt");
    QFile keyFile("../server.key");

    if (certFile.open(QIODevice::ReadOnly) && keyFile.open(QIODevice::ReadOnly)) {
        certificate = QSslCertificate(certFile.readAll());
        privateKey = QSslKey(keyFile.readAll(), QSsl::Rsa);

        sslSocket->setLocalCertificate(certificate);
        sslSocket->setPrivateKey(privateKey);
        sslSocket->setPeerVerifyMode(QSslSocket::VerifyNone); // 클라이언트 인증서 검증 안 함

        connect(sslSocket, &QSslSocket::encrypted, this, &HttpServer::onEncrypted);
        connect(sslSocket, &QSslSocket::readyRead, this, &HttpServer::onReadyRead);

        // SSL 핸드셰이크 시작
        sslSocket->startServerEncryption();
    } else {
        qCritical() << "Failed to load SSL certificate or private key.";
        sslSocket->deleteLater();
        return;
    }

//    qDebug() << "sslSocket : "<<sslSocket;
//    // SSL 암호화 활성화
//    connect(sslSocket, &QSslSocket::encrypted, this, [this, sslSocket]() {
//        handleRequest(sslSocket); // 요청 처리
//    });

//    connect(sslSocket, &QSslSocket::disconnected, sslSocket, &QSslSocket::deleteLater);

//    // SSL 핸드셰이크 시작
//    sslSocket->startServerEncryption();
}

QList<QVariantMap> convertToQVariantMapList(const QStringList& stringList) {
    QList<QVariantMap> result;

    for (const QString& str : stringList) {
        QStringList fields = str.split(","); // Assuming fields are comma-separate
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


void HttpServer::onReadyRead() {
    QSslSocket *sslSocket = qobject_cast<QSslSocket *>(sender());
    if (sslSocket) {
        QByteArray requestData = sslSocket->readAll();
        //qDebug() << "Received data:" << requestData;
        QString request = QString::fromUtf8(requestData);
        QString method, path;
        QTextStream stream(&request);
        stream >> method >> path;
        qDebug() << method << path;

        // 간단한 RESTful API 응답 처리
        if (method == "GET" && path.startsWith("/hello")) {
            sendResponse(sslSocket, "Hello, SSL World!", 200);
        } else if (method == "GET" && path.startsWith("/records")) {
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
                sendResponse(sslSocket, "Invalid date format. Use yyyy-MM-dd.", 400);
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
            sendResponse(sslSocket, jsonDoc.toJson(), 200);
        } else if (method == "GET" && path.startsWith("/cameras")) {
            QList<QVariantMap> cameras = DatabaseManager::instance().getAllCameras(); // 카메라 데이터 가져오기
            QJsonArray jsonArray;

            for (const auto& camera : cameras) {
                QJsonObject jsonObject;
                jsonObject["Camera_ID"] = camera["camera_ID"].toInt();
                jsonObject["Camera_Name"] = camera["Camera_Name"].toString();
                jsonObject["Camera_RTSP_URL"] = camera["Camera_RTSP_URL"].toString();
                jsonArray.append(jsonObject);
            }
            QJsonDocument jsonDoc(jsonArray);
            sendResponse(sslSocket, jsonDoc.toJson(), 200);
        } else if (method == "GET" && path.startsWith("/images/")) {
            // 요청된 경로를 실제 파일 시스템 경로로 변환
            QString filePath = QString("C:/Users/3kati/Desktop/db_qt/images") + path.mid(7); // "/images/" 이후 경로 추가
            QFile file(filePath);

            // 파일 존재 여부 확인
            if (!file.exists()) {
                sendResponse(sslSocket, "Image not found", 404);
                return;
            }

            // 파일 열기
            if (!file.open(QIODevice::ReadOnly)) {
                sendResponse(sslSocket, "Failed to open image", 500);
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
            sslSocket->write(httpResponse);
            sslSocket->flush();
            sslSocket->disconnectFromHost();
            return;
        } else if (method == "GET" && path.startsWith("/gatefees")) {
            QSqlQuery query("SELECT GateNumber, GateFee FROM GATELIST");
            QJsonObject response;
            while (query.next()) {
                int gateNumber = query.value("GateNumber").toInt();
                int gateFee = query.value("GateFee").toInt();
                response[QString::number(gateNumber)] = gateFee;
            }

            QJsonDocument jsonDoc(response);
            sendResponse(sslSocket, jsonDoc.toJson(), 200);
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
                sendResponse(sslSocket, "Record added successfully", 200);

                //insert data in BILL table.
                if(dbManager.checkIsEnterGate(gateNumber)){
                    dbManager.insertEnterStepBill(plateNumber, gateNumber, newRecordID);
                }else {
                    dbManager.insertExitStepBill(plateNumber, gateNumber, newRecordID);
                }

            } else {
                sendResponse(sslSocket, "Failed to add record", 500);
            }
        } else if (method == "POST" && path == "/upload") {
            QString body = request.split("\r\n\r\n").last();
            qDebug() << "body:" << body;

            // JSON 파싱 오류 검사
            QJsonParseError parseError;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(body.toUtf8(), &parseError);

            if (parseError.error != QJsonParseError::NoError) {
                qDebug() << "JSON Parse Error:" << parseError.errorString();
                return;
            }

            QJsonObject jsonObject = jsonDoc.object();
            qDebug() << "Parsed JSON Object:" << jsonObject;

            // JSON 데이터 추출
            int gateNumber = jsonObject.value("GateNum").toInt();
            QString plateNumber = jsonObject.value("PlateNum").toString();
            QString time = jsonObject.value("time").toString();
            QString imageRawString = jsonObject.value("imageRaw").toString();
            int imageWidth = jsonObject.value("imageWidth").toInt();
            int imageHeight = jsonObject.value("imageHeight").toInt();

            qDebug() << "imageWidth:" << imageWidth << "imageHeight:" << imageHeight << "plateNumber:" << plateNumber;
            // Create directory [PlateNumber] if it doesn't exist
            QDir dir(plateNumber);
            if (!dir.exists()) {
                dir.mkpath(".");
            }
            // Generate filename: "YYYYMMDD_ttmmss.jpg"
            QString date = QDate::currentDate().toString("yyyyMMdd");
            QString timePart = QTime::currentTime().toString("hhmmss");
            QString fileName = QString("%1/%2_%3.jpg").arg(plateNumber).arg(date).arg(timePart);

            // Base64 디코딩 및 파일 저장 테스트
            if (!imageRawString.isEmpty()) {
                decodeBase64AndSaveToFile(imageRawString.toStdString(), fileName.toStdString());
                qDebug() << "Image saved successfully.";
                sendResponse(sslSocket, "Image saved successfully.", 200);
            }else{
                sendResponse(sslSocket, "Image saved fail.", 400);
            }
        } else if (method == "POST" && path == "/camera") {
            // 요청 본문에서 JSON 데이터 추출
            qDebug() << "Received 원본:" << request;
            QString body = request.mid(request.indexOf("\r\n\r\n") + 4).trimmed(); // JSON 본문 추출
            qDebug() << "Received body:" << body;

            // JSON 파싱
            QJsonDocument jsonDoc = QJsonDocument::fromJson(body.toUtf8());
            if (jsonDoc.isNull() || !jsonDoc.isObject()) {
                sendResponse(sslSocket, "Invalid JSON format", 400);
                return;
            }

            QJsonObject jsonObject = jsonDoc.object();
            QString cameraName = jsonObject.value("Camera_Name").toString();
            QString rtspUrl = jsonObject.value("Camera_RTSP_URL").toString();

            if (cameraName.isEmpty() || rtspUrl.isEmpty()) {
                sendResponse(sslSocket, "Missing Camera_Name or Camera_RTSP_URL", 400);
                return;
            }
            qDebug() << "Camera Name:" << cameraName;
            qDebug() << "RTSP URL:" << rtspUrl;

            // 데이터베이스에 카메라 추가
            int newCameraID = DatabaseManager::instance().addCamera(cameraName, rtspUrl);
            if (newCameraID != -1) {
                // 성공 응답 생성
                QJsonObject responseObj;
                responseObj["message"] = "Camera added successfully";
                responseObj["camera_ID"] = newCameraID;

                QJsonDocument responseDoc(responseObj);
                sendResponse(sslSocket, responseDoc.toJson(), 200);
            } else {
                sendResponse(sslSocket, "Failed to add camera to the database", 500);
            }
        } else {
            sendResponse(sslSocket, "Not Found", 404);
        }
        sslSocket->flush();
        sslSocket->disconnectFromHost();
    }
}

void HttpServer::sendResponse(QSslSocket* socket, const QByteArray& body, int statusCode) {
    QByteArray responseHeaders;
    responseHeaders.append("HTTP/1.1 " + QByteArray::number(statusCode) + " OK\r\n");
    responseHeaders.append("Content-Type: application/json\r\n");
    responseHeaders.append("Content-Length: " + QByteArray::number(body.size()) + "\r\n");
    responseHeaders.append("\r\n");

    socket->write(responseHeaders);
    socket->write(body);
    socket->flush();
}

// Function to decode the Base64 string and save it as a .jpg file
void HttpServer::decodeBase64AndSaveToFile(const std::string& encoded, const std::string& filename) {
    std::string decodedData = base64_decode(encoded, true);
    std::vector<uchar> decodedVec(decodedData.begin(), decodedData.end());
    cv::InputArray inputArray(decodedVec);

    // Decode the byte buffer into a cv::Mat image
    cv::Mat img = cv::imdecode(inputArray, cv::IMREAD_COLOR);
    if (img.empty()) {
        std::cerr << "Failed to decode Base64 to image." << std::endl;
        return;
    }

    if (cv::imwrite(filename, img)) {
        std::cout << "Image saved successfully to " << filename << std::endl;
    } else {
        std::cerr << "Failed to save image to file." << std::endl;
    }
}
