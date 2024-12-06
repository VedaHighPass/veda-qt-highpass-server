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
#include <opencv2/opencv.hpp>
#include <QDebug>
#include <base64.h>

HttpServer::HttpServer(DatabaseManager& dbManager, QObject* parent)
    : QTcpServer(parent), dbManager(dbManager) {}

void HttpServer::startServer(quint16 port) {
    if (!this->listen(QHostAddress::Any, port)) {
        qCritical() << "Failed to start server on port" << port;
    } else {
        qDebug() << "Server started on port" << port;
    }
}

// Function to decode the Base64 string and save it as a .jpg file
void decodeBase64AndSaveToFile(const std::string& encoded, const std::string& filename) {
    // Decode the Base64 string into raw byte data
    std::string decodedData = base64_decode(encoded, true); // Remove line breaks if present

    // Convert the decoded string to a std::vector<uchar> (byte buffer)
    std::vector<uchar> decodedVec(decodedData.begin(), decodedData.end());

    // Create an InputArray from the decoded byte buffer
    cv::InputArray inputArray(decodedVec);

    // Decode the byte buffer into a cv::Mat image
    cv::Mat img = cv::imdecode(inputArray, cv::IMREAD_COLOR); // Use IMREAD_COLOR to load a color image

    // Check if the decoding was successful
    if (img.empty()) {
        std::cerr << "Failed to decode Base64 to image." << std::endl;
        return;
    }

    // Save the decoded image to a file (e.g., "output.jpg")
    if (cv::imwrite(filename, img)) {
        std::cout << "Image saved successfully to " << filename << std::endl;
    } else {
        std::cerr << "Failed to save image to file." << std::endl;
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

QList<QByteArray> customSplit(const QByteArray& input, const QByteArray& delimiter) {
    QList<QByteArray> result;
    int startIndex = 0;

    while (true) {
        int delimiterIndex = input.indexOf(delimiter, startIndex);
        if (delimiterIndex == -1) {
            result.append(input.mid(startIndex)); // Add the remaining part
            break;
        }
        result.append(input.mid(startIndex, delimiterIndex - startIndex));
        startIndex = delimiterIndex + delimiter.size();
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

    qDebug() << "server ===== " << method <<"-----"<< path;

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
    } else if (method == "POST" && path == "/upload") {
        qDebug() << "server upload start";
        QString body = request.split("\r\n\r\n").last();
        //qDebug() << "body: " << body;

        QJsonDocument jsonDoc = QJsonDocument::fromJson(body.toUtf8());
        QJsonObject jsonObject = jsonDoc.object();

        // JSON에서 데이터 추출
        int gateNumber = jsonObject.value("GateNum").toInt();
        QString plateNumber = jsonObject.value("PlateNum").toString();
        QString time = jsonObject.value("time").toString();
        QString imageRawString = jsonObject.value("imageRaw").toString();
        int imageWidth = jsonObject.value("imageWidth").toInt();
        int imageHeight = jsonObject.value("imageHeight").toInt();

        qDebug() << "imageWidth: " << imageWidth;
        qDebug() << "imageHeight: " << imageHeight;
        qDebug() << "plateNumber: " << plateNumber;

        // Raw 데이터를 QByteArray로 변환
    //    QByteArray imageRawData = imageRawString.toUtf8();

        // cv::Mat로 복원
    //    cv::Mat rawImage(imageHeight, imageWidth, CV_8UC3, reinterpret_cast<void*>(imageRawData.data()));


        qDebug() << "test-1";

        // Create directory "AAA" if it doesn't exist
        QDir dir("AAA");
        if (!dir.exists()) {
            dir.mkpath(".");
        }

        // Generate filename: "YYYYMMDD_ttmm.jpg"
        QString date = QDate::currentDate().toString("yyyyMMdd");
        QString timePart = QTime::currentTime().toString("hhmm");
        QString fileName = QString("AAA/%1_%2.jpg").arg(date).arg(timePart);

        decodeBase64AndSaveToFile(imageRawString.toStdString(), fileName.toStdString());

        sendResponse(socket, "Upload successful", 200);
        qDebug() << "Upload Successful";
    }  else if (method == "POST" && path == "/camera") {
        // 요청 본문에서 JSON 데이터 추출
        qDebug() << "Received 원본:" << request;
        QString body = request.mid(request.indexOf("\r\n\r\n") + 4).trimmed(); // JSON 본문 추출
        qDebug() << "Received body:" << body;

        // JSON 파싱
        QJsonDocument jsonDoc = QJsonDocument::fromJson(body.toUtf8());
        if (jsonDoc.isNull() || !jsonDoc.isObject()) {
            sendResponse(socket, "Invalid JSON format", 400);
            return;
        }

        QJsonObject jsonObject = jsonDoc.object();
        QString cameraName = jsonObject.value("Camera_Name").toString();
        QString rtspUrl = jsonObject.value("Camera_RTSP_URL").toString();

        if (cameraName.isEmpty() || rtspUrl.isEmpty()) {
            sendResponse(socket, "Missing Camera_Name or Camera_RTSP_URL", 400);
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
            sendResponse(socket, responseDoc.toJson(), 200);
        } else {
            sendResponse(socket, "Failed to add camera to the database", 500);
        }
    } else if (method == "GET" && path == "/cameras") {
        // 모든 카메라 리스트 가져오기
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
        sendResponse(socket, jsonDoc.toJson(), 200);
    } else {
        sendResponse(socket, "Not Found", 404);
    }
}

void HttpServer::sendResponse(QTcpSocket* socket, const QByteArray& body, int statusCode) {
    // Prepare headers
    QByteArray responseHeaders;

    // Basic HTTP status line (e.g., "HTTP/1.1 200 OK")
    responseHeaders.append("HTTP/1.1 " + QByteArray::number(statusCode) + " OK\r\n");

    // Content-Type header (for JSON response)
    responseHeaders.append("Content-Type: application/json\r\n");

    // Content-Length header
    responseHeaders.append("Content-Length: " + QByteArray::number(body.size()) + "\r\n");

    // End of headers (empty line)
    responseHeaders.append("\r\n");

    // Send headers and body
    socket->write(responseHeaders);
    socket->write(body);
    socket->flush();
}
