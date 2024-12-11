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
    // SSL �냼耳� �깮�꽦
    QSslSocket* sslSocket = new QSslSocket(this);

    // �냼耳� �뵒�뒪�겕由쏀꽣 �꽕�젙
    if (!sslSocket->setSocketDescriptor(socketDescriptor)) {
        qDebug() << "Failed to set socket descriptor";
        sslSocket->deleteLater();
        return;
    }

    // �씤利앹꽌 諛� �궎 �꽕�젙
    QSslCertificate certificate;
    QSslKey privateKey;

    // �씤利앹꽌 諛� �궎 �뙆�씪 濡쒕뱶
    QFile certFile("../server.crt");
    QFile keyFile("../server.key");

    if (certFile.open(QIODevice::ReadOnly) && keyFile.open(QIODevice::ReadOnly)) {
        certificate = QSslCertificate(certFile.readAll());
        privateKey = QSslKey(keyFile.readAll(), QSsl::Rsa);

        sslSocket->setLocalCertificate(certificate);
        sslSocket->setPrivateKey(privateKey);
        sslSocket->setPeerVerifyMode(QSslSocket::VerifyNone); // �겢�씪�씠�뼵�듃 �씤利앹꽌 寃�利� �븞 �븿

        connect(sslSocket, &QSslSocket::encrypted, this, &HttpServer::onEncrypted);
        connect(sslSocket, &QSslSocket::readyRead, this, &HttpServer::onReadyRead);

        // SSL �빖�뱶�뀺�씠�겕 �떆�옉
        sslSocket->startServerEncryption();
    } else {
        qCritical() << "Failed to load SSL certificate or private key.";
        sslSocket->deleteLater();
        return;
    }

//    qDebug() << "sslSocket : "<<sslSocket;
//    // SSL �븫�샇�솕 �솢�꽦�솕
//    connect(sslSocket, &QSslSocket::encrypted, this, [this, sslSocket]() {
//        handleRequest(sslSocket); // �슂泥� 泥섎━
//    });

//    connect(sslSocket, &QSslSocket::disconnected, sslSocket, &QSslSocket::deleteLater);

//    // SSL �빖�뱶�뀺�씠�겕 �떆�옉
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
        static QByteArray requestData;
        requestData.append(sslSocket->readAll());
        // HTTP 헤더 끝을 찾아 Content-Length를 확인
        int headerEndIndex = requestData.indexOf("\r\n\r\n");
        if (headerEndIndex == -1) {
            // 헤더가 아직 도착하지 않은 경우, 더 읽기를 대기
            return;
        }


        // Content-Length 값 확인
        QByteArray header = requestData.left(headerEndIndex);
        QByteArray body = requestData.mid(headerEndIndex + 4);
        int contentLength = 0;
        bool findContentLength = false;

        QList<QByteArray> headerLines = header.split('\n');
        for (const QByteArray &line : headerLines) {
            // "Content-Length:"가 문자열의 시작 부분인지 확인
            if (line.left(15) == "Content-Length:") {
                findContentLength = true;
                // ':' 이후 값을 잘라내어 Content-Length를 추출
                contentLength = line.mid(15).trimmed().toInt();
                break;
            }
        }

        //qDebug() << requestData;
        //qDebug() << "contentLength:" << contentLength;


        if (contentLength > 0 && body.size() < contentLength) {
            // 본문이 아직 완전히 도착하지 않았으면 더 읽기를 대기
            return;
        }

        // 요청이 완전한 경우 처리 시작
        QString request = QString::fromUtf8(requestData);
        QString method, path;
        QTextStream stream(&request);
        stream >> method >> path;
        qDebug() << method << path;

        // 본문 데이터만 추출
        QJsonDocument jsonDoc;
        if(findContentLength){
            body = requestData.mid(headerEndIndex + 4, contentLength);

            // JSON 파싱 및 처리
            QJsonParseError parseError;
            jsonDoc = QJsonDocument::fromJson(body, &parseError);
            if (parseError.error != QJsonParseError::NoError) {
                qDebug() << "JSON Parse Error:" << parseError.errorString();
                sendResponse(sslSocket, "Invalid JSON format", 400);
                requestData.clear();
                return;
            }
        }else{
            body = requestData;
            requestData.clear();
        }
        //qDebug() << "Complete body:" << body;



        QJsonObject jsonObject = jsonDoc.object();

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

            // JSON 蹂��솚 諛� �쓳�떟 �깮�꽦
            QJsonArray jsonArray;
            for (const auto& record : result.records) {
                QJsonObject jsonObject;
                for (const auto& key : record.keys()) {
                    jsonObject[key] = QJsonValue::fromVariant(record[key]);
                }
                jsonArray.append(jsonObject);
            }

            // �쟾泥� �젅肄붾뱶 �닔��� �뜲�씠�꽣瑜� �룷�븿�븳 �쓳�떟 �깮�꽦
            QJsonObject response;
            response["data"] = jsonArray;
            response["totalRecords"] = result.totalRecords;

            QJsonDocument jsonDoc(response);
            sendResponse(sslSocket, jsonDoc.toJson(), 200);
        } else if (method == "GET" && path.startsWith("/cameras")) {
            QList<QVariantMap> cameras = DatabaseManager::instance().getAllCameras(); // 移대찓�씪 �뜲�씠�꽣 媛��졇�삤湲�
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
            // �슂泥��맂 寃쎈줈瑜� �떎�젣 �뙆�씪 �떆�뒪�뀥 寃쎈줈濡� 蹂��솚
            QString filePath = QString("/home/server/veda-qt-highpass-server/images") + path.mid(7); // "/images/" �씠�썑 寃쎈줈 異붽��
            QFile file(filePath);

            // �뙆�씪 議댁옱 �뿬遺� �솗�씤
            if (!file.exists()) {
                sendResponse(sslSocket, "Image not found", 404);
                return;
            }

            // �뙆�씪 �뿴湲�
            if (!file.open(QIODevice::ReadOnly)) {
                sendResponse(sslSocket, "Failed to open image", 500);
                return;
            }

            QByteArray imageData = file.readAll();
            file.close();

            // �씠誘몄�� �뜲�씠�꽣瑜� 諛섑솚
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

            // JSON �뙆�떛 �삤瑜� 寃��궗
            QJsonParseError parseError;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(body.toUtf8(), &parseError);

            if (parseError.error != QJsonParseError::NoError) {
                qDebug() << "JSON Parse Error:" << parseError.errorString();
                return;
            }

            QJsonObject jsonObject = jsonDoc.object();
            qDebug() << "Parsed JSON Object:" << jsonObject;

            // JSON �뜲�씠�꽣 異붿텧
            int gateNumber = jsonObject.value("GateNum").toInt();
            QString plateNumber = jsonObject.value("PlateNum").toString();
            QString time = jsonObject.value("time").toString();
            int imageWidth = jsonObject.value("imageWidth").toInt();
            int imageHeight = jsonObject.value("imageHeight").toInt();
            QString imageRawString = jsonObject.value("imageRaw").toString();

            qDebug() << "imageWidth:" << imageWidth << "imageHeight:" << imageHeight << "plateNumber:" << plateNumber << "time:" << time;
            // Create directory [PlateNumber] if it doesn't exist
            QDir dir("../images/"+plateNumber);
            if (!dir.exists()) {
                dir.mkpath(".");
            }
            // Generate filename: "YYYYMMDD_ttmmss.jpg"
            QDateTime currentTime = QDateTime::fromString(time, "yyyy-MM-ddTHH:mm:ss");

            QString date = currentTime.toString("yyyyMMdd");
            QString timePart = currentTime.toString("hhmmss");
            QString fileName = QString("../images/%1/%2_%3.jpg").arg(plateNumber).arg(date).arg(timePart);

            // Base64 �뵒肄붾뵫 諛� �뙆�씪 ����옣 �뀒�뒪�듃

            if (!imageRawString.isEmpty()) {
                decodeBase64AndSaveToFile(imageRawString.toStdString(), fileName.toStdString());
                sendResponse(sslSocket, "Image saved successfully.", 200);
            }else{
                sendResponse(sslSocket, "Image saved fail.", 400);
            }

            QString formattedTime = currentTime.toString("yyyy-MM-dd HH:mm:ss");
            if (DatabaseManager::instance().processHighPassRecord(plateNumber, gateNumber, formattedTime)) {
                qDebug() << "DB saved successfully.";
            }else {
                qDebug() << "DB saved fail.";
            }
        } else if (method == "POST" && path == "/camera") {
            // �슂泥� 蹂몃Ц�뿉�꽌 JSON �뜲�씠�꽣 異붿텧
            qDebug() << "Received �썝蹂�:" << request;
            QString body = request.mid(request.indexOf("\r\n\r\n") + 4).trimmed(); // JSON 蹂몃Ц 異붿텧
            qDebug() << "Received body:" << body;

            // JSON �뙆�떛
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

            // �뜲�씠�꽣踰좎씠�뒪�뿉 移대찓�씪 異붽��
            int newCameraID = DatabaseManager::instance().addCamera(cameraName, rtspUrl);
            if (newCameraID != -1) {
                // �꽦怨� �쓳�떟 �깮�꽦
                QJsonObject responseObj;
                responseObj["message"] = "Camera added successfully";
                responseObj["camera_ID"] = newCameraID;

                QJsonDocument responseDoc(responseObj);
                sendResponse(sslSocket, responseDoc.toJson(), 200);
            } else {
                sendResponse(sslSocket, "Failed to add camera to the database", 500);
            }
        } else if (method == "POST" && path == "/emails") { // �슂泥� 蹂몃Ц�뿉�꽌 JSON �뜲�씠�꽣 異붿텧
            QString body = request.mid(request.indexOf("\r\n\r\n") + 4).trimmed(); // JSON 蹂몃Ц 異붿텧
            QJsonDocument jsonDoc = QJsonDocument::fromJson(body.toUtf8());

            if (jsonDoc.isNull() || !jsonDoc.isObject()) {
                sendResponse(sslSocket, "Invalid JSON format", 400);
            return;
            }

            QJsonObject jsonObject = jsonDoc.object();
            QString plateNumber = jsonObject.value("PlateNumber").toString();
            QString email = jsonObject.value("Email").toString();

            // �쑀�슚�꽦 寃��궗
            if (plateNumber.isEmpty() || email.isEmpty()) {
                sendResponse(sslSocket, "Missing PlateNumber or Email", 400);
                return;
            }

            // Emails �뀒�씠釉붿뿉 �뜲�씠�꽣 異붽�� �삉�뒗 �뾽�뜲�씠�듃
            bool success = DatabaseManager::instance().addOrUpdateEmail(plateNumber, email);
            if (success) {
                sendResponse(sslSocket, "Email information updated successfully", 200);
            } else {
                sendResponse(sslSocket, "Failed to update email information", 500);
            }
        } else if (method == "POST" && path == "/highPassRecord") {
            QString body = request.mid(request.indexOf("\r\n\r\n") + 4).trimmed();
            QJsonDocument jsonDoc = QJsonDocument::fromJson(body.toUtf8());
            if (jsonDoc.isNull() || !jsonDoc.isObject()) {
                sendResponse(sslSocket, "Invalid JSON format", 400);
                return;
            }

            QJsonObject jsonObject = jsonDoc.object();
            QString plateNumber = jsonObject.value("PlateNumber").toString();
            int gateNumber = jsonObject.value("GateNumber").toInt();
            QString timeStr = jsonObject.value("Time").toString();

            if (plateNumber.isEmpty() || gateNumber <= 0 || timeStr.isEmpty()) {
                sendResponse(sslSocket, "Missing PlateNumber, GateNumber, or Time", 400);
                return;
            }

            // JSON�뿉�꽌 Time�쓣 媛��졇��� QDateTime�쑝濡� 蹂��솚
            QDateTime currentTime = QDateTime::fromString(timeStr, "yyyy-MM-dd HH:mm:ss");
            if (!currentTime.isValid()) {
                sendResponse(sslSocket, "Invalid time format. Use yyyy-MM-dd HH:mm:ss", 400);
                return;
            }

            if (DatabaseManager::instance().processHighPassRecord(plateNumber, gateNumber, currentTime.toString("yyyy-MM-dd HH:mm:ss"))) {
                sendResponse(sslSocket, "Path updated successfully", 200);
            } else {
                sendResponse(sslSocket, "Failed to update Path", 500);
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

