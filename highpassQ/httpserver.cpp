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
        //sslSocket->setPeerVerifyMode(QSslSocket::VerifyNone); // �겢�씪�씠�뼵�듃 �씤利앹꽌 寃�利� �븞 �븿

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
        //qDebug() << method << path;

        // 본문 데이터만 추출
        QJsonDocument jsonDoc;
        if(findContentLength){
            body = requestData.mid(headerEndIndex + 4, contentLength);

            // JSON 파싱 및 처리
            QJsonParseError parseError;
            jsonDoc = QJsonDocument::fromJson(body, &parseError);
            if (parseError.error != QJsonParseError::NoError) {
                //qDebug() << "JSON Parse Error:" << parseError.errorString();
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
        /**
         * @brief "/hello" 경로에 대한 간단한 GET 요청을 처리합니다.
         *
         * 이 함수는 "/hello" 경로로 들어오는 GET 요청을 처리하며,
         * 클라이언트에게 간단한 텍스트 응답("Hello, SSL World!")과 함께 HTTP 상태 코드 200을 반환합니다.
         *
         * @details
         * - 요청 경로가 "/hello"로 시작하는지 확인합니다.
         * - 요청이 유효한 경우 응답으로 "Hello, SSL World!"를 반환합니다.
         *
         * 요청 예제:
         * @code
         * GET http://127.0.0.1:8080/hello
         * @endcode
         *
         * 응답 예제:
         * @code
         * HTTP/1.1 200 OK
         * Content-Type: text/plain
         *
         * Hello, SSL World!
         * @endcode
         *
         * @param sslSocket 응답을 전송하는 데 사용되는 SSL 소켓의 포인터.
         *
         * @return void
         */
        if (method == "GET" && path.startsWith("/hello")) {
            sendResponse(sslSocket, "Hello, SSL World!", 200);
        }
        /**
         * @brief 지정된 필터를 기반으로 기록을 가져오는 GET 요청을 처리합니다.
         *
         * 이 함수는 "/records"로 시작하는 경로의 GET 요청을 처리합니다.
         * 쿼리 매개변수를 추출하여 필터링 조건을 설정하고, 입력값을 검증한 후,
         * 데이터베이스에서 데이터를 조회하여 JSON 형식으로 결과를 반환합니다.
         *
         * @details
         * 지원하는 쿼리 매개변수:
         * - `startDate` (필수): 기록을 필터링할 시작 날짜 (형식: yyyy-MM-dd).
         * - `endDate` (필수): 기록을 필터링할 종료 날짜 (형식: yyyy-MM-dd).
         * - `plateNumber` (선택): 차량 번호를 기준으로 기록을 필터링.
         * - `entryGate` (선택): 출입 게이트 번호의 쉼표로 구분된 목록 (예: "1,2,3").
         * - `exitGate` (선택): 출구 게이트 번호의 쉼표로 구분된 목록 (예: "4,5").
         * - `pageSize` (선택): 페이지당 반환할 기록 수 (기본값: 10).
         * - `page` (선택): 가져올 페이지 번호 (기본값: 1).
         *
         * 동작 과정:
         * 1. URL을 파싱하여 쿼리 매개변수를 추출합니다.
         * 2. `startDate`와 `endDate`의 날짜 형식을 검증합니다.
         * 3. `entryGate`와 `exitGate` 값을 정수 리스트로 변환합니다.
         * 4. `DatabaseManager`를 사용해 필터 조건에 따라 데이터를 조회합니다.
         * 5. 조회된 기록과 총 기록 수를 포함한 JSON 응답을 생성합니다.
         *
         * 요청 예제:
         * @code
         * GET http://127.0.0.1:8080/records?startDate=2024-11-13&endDate=2024-11-23&pageSize=10&page=1
         * @endcode
         *
         * JSON 응답 예제:
         * @code
         * {
         *     "data": [
         *         {
         *             "plateNumber": "ABC123",
         *             "entryGate": 1,
         *             "exitGate": 4,
         *             "entryTime": "2024-11-13T08:30:00",
         *             "exitTime": "2024-11-13T09:00:00"
         *         },
         *         {
         *             "plateNumber": "XYZ789",
         *             "entryGate": 2,
         *             "exitGate": 5,
         *             "entryTime": "2024-11-13T10:00:00",
         *             "exitTime": "2024-11-13T10:30:00"
         *         }
         *     ],
         *     "totalRecords": 2
         * }
         * @endcode
         *
         * @param sslSocket 응답을 전송하는 데 사용되는 SSL 소켓의 포인터.
         *
         * @throws std::runtime_error 데이터베이스 작업 또는 JSON 생성에 문제가 발생한 경우 예외를 던집니다.
         * @return void
         */
        else if (method == "GET" && path.startsWith("/records")) {
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

            // JSON
            QJsonArray jsonArray;
            for (const auto& record : result.records) {
                QJsonObject jsonObject;
                for (const auto& key : record.keys()) {
                    jsonObject[key] = QJsonValue::fromVariant(record[key]);
                }
                if( jsonObject["PlateNumber"] != "UNKNOWN"){
                    jsonArray.append(jsonObject);
                }
            }

            QJsonObject response;
            response["data"] = jsonArray;
            response["totalRecords"] = result.totalRecords;

            QJsonDocument jsonDoc(response);
            sendResponse(sslSocket, jsonDoc.toJson(), 200);
        }
        /**
         * @brief "/cameras" 경로에 대한 GET 요청을 처리합니다.
         *
         * 이 함수는 "/cameras" 경로로 들어오는 GET 요청을 처리하며,
         * 데이터베이스에서 모든 카메라 정보를 가져와 JSON 배열로 변환한 후 클라이언트에게 반환합니다.
         *
         * @details
         * - 요청 경로가 "/cameras"로 시작하는지 확인합니다.
         * - 데이터베이스에서 카메라 정보를 조회합니다.
         * - 각 카메라 정보를 JSON 형식으로 변환하여 JSON 배열에 추가합니다.
         * - 생성된 JSON 데이터를 클라이언트에 HTTP 200 상태 코드와 함께 전송합니다.
         *
         * JSON 응답 예제:
         * @code
         * HTTP/1.1 200 OK
         * Content-Type: application/json
         *
         * [
         *     {
         *         "Camera_ID": 1,
         *         "Camera_Name": "Front Gate",
         *         "Camera_RTSP_URL": "rtsp://192.168.1.10:554/stream1"
         *     },
         *     {
         *         "Camera_ID": 2,
         *         "Camera_Name": "Parking Lot",
         *         "Camera_RTSP_URL": "rtsp://192.168.1.11:554/stream2"
         *     }
         * ]
         * @endcode
         *
         * @param sslSocket 응답을 전송하는 데 사용되는 SSL 소켓의 포인터.
         *
         * @throws std::runtime_error 데이터베이스 작업 또는 JSON 생성에 문제가 발생한 경우 예외를 던질 수 있습니다.
         * @return void
         */
        else if (method == "GET" && path.startsWith("/cameras")) {
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
        }
        /**
         * @brief "/images/" 경로에 대한 GET 요청을 처리합니다.
         *
         * 이 함수는 "/images/" 경로로 시작하는 GET 요청을 처리하며,
         * 요청된 이미지 파일을 읽어 클라이언트에게 HTTP 응답으로 반환합니다.
         *
         * @details
         * - 요청 경로에서 "/images/" 이후의 파일 경로를 추출합니다.
         * - 서버의 이미지 디렉터리(`/home/server/veda-qt-highpass-server/images`)에서 파일 경로를 구성합니다.
         * - 파일이 존재하지 않을 경우 HTTP 404 상태 코드와 함께 "Image not found" 메시지를 반환합니다.
         * - 파일 열기에 실패하면 HTTP 500 상태 코드와 함께 "Failed to open image" 메시지를 반환합니다.
         * - 성공적으로 파일을 읽으면, 파일 데이터를 HTTP 응답 본문에 포함하여 반환합니다.
         *
         * 요청 예제:
         * @code
         * GET http://127.0.0.1:8080/images/example.jpg
         * @endcode
         *
         * 응답 예제:
         * @code
         * HTTP/1.1 200 OK
         * Content-Type: image/jpeg
         * Content-Length: 123456
         *
         * [이미지 바이너리 데이터]
         * @endcode
         *
         * 오류 응답 예제 (파일 없음):
         * @code
         * HTTP/1.1 404 Not Found
         * Content-Type: text/plain
         *
         * Image not found
         * @endcode
         *
         * 오류 응답 예제 (파일 열기 실패):
         * @code
         * HTTP/1.1 500 Internal Server Error
         * Content-Type: text/plain
         *
         * Failed to open image
         * @endcode
         *
         * @param sslSocket 응답을 전송하는 데 사용되는 SSL 소켓의 포인터.
         *
         * @throws std::runtime_error 파일 읽기 실패 또는 응답 전송 중 문제가 발생할 경우 예외를 던질 수 있습니다.
         * @return void
         */
        else if (method == "GET" && path.startsWith("/images/")) {
            QString filePath = QString("/home/server/veda-qt-highpass-server/images") + path.mid(7); // "/images/" �씠�썑 寃쎈줈 異붽��
            QFile file(filePath);

            if (!file.exists()) {
                sendResponse(sslSocket, "Image not found", 404);
                return;
            }

            if (!file.open(QIODevice::ReadOnly)) {
                sendResponse(sslSocket, "Failed to open image", 500);
                return;
            }

            QByteArray imageData = file.readAll();
            file.close();

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
        }
        /**
         * @brief "/gatefees" 경로에 대한 GET 요청을 처리합니다.
         *
         * 이 함수는 "/gatefees" 경로로 들어오는 GET 요청을 처리하며,
         * 데이터베이스에서 게이트 번호와 관련 요금을 조회하여 JSON 객체로 클라이언트에게 반환합니다.
         *
         * @details
         * - GATELIST 테이블에서 GateNumber와 GateFee 열을 조회합니다.
         * - 각 GateNumber를 키로, GateFee를 값으로 하는 JSON 객체를 생성합니다.
         * - 생성된 JSON 데이터를 HTTP 200 상태 코드와 함께 클라이언트에 반환합니다.
         *
         * JSON 응답 예제:
         * @code
         * HTTP/1.1 200 OK
         * Content-Type: application/json
         *
         * {
         *     "1": 500,
         *     "2": 300,
         *     "3": 700
         * }
         * @endcode
         *
         * 요청 예제:
         * @code
         * GET http://127.0.0.1:8080/gatefees
         * @endcode
         *
         * @param sslSocket 응답을 전송하는 데 사용되는 SSL 소켓의 포인터.
         *
         * @throws std::runtime_error 데이터베이스 쿼리 실패 또는 JSON 생성 중 문제가 발생할 경우 예외를 던질 수 있습니다.
         * @return void
         */
        else if (method == "GET" && path.startsWith("/gatefees")) {
            QSqlQuery query("SELECT GateNumber, GateFee FROM GATELIST");
            QJsonObject response;
            while (query.next()) {
                int gateNumber = query.value("GateNumber").toInt();
                int gateFee = query.value("GateFee").toInt();
                response[QString::number(gateNumber)] = gateFee;
            }

            QJsonDocument jsonDoc(response);
            sendResponse(sslSocket, jsonDoc.toJson(), 200);
        }
        /**
         * @brief "/records" 경로에 대한 POST 요청을 처리합니다.
         *
         * 이 함수는 "/records" 경로로 들어오는 POST 요청을 처리하며,
         * JSON 형식으로 제공된 데이터를 기반으로 HIGHPASS_RECORD 테이블에 새 기록을 추가합니다.
         * 기록 추가 후, BILL 테이블에 관련 데이터를 삽입합니다.
         *
         * @details
         * - 요청 본문에서 JSON 데이터를 추출하여 `EntryTime`, `PlateNumber`, `GateNumber`를 읽어옵니다.
         * - HIGHPASS_RECORD 테이블에 데이터를 추가하고, 성공 시 새로운 레코드 ID를 반환받습니다.
         * - 게이트 번호에 따라 BILL 테이블에 해당 데이터를 삽입합니다.
         *   - 진입 게이트인 경우 `insertEnterStepBill`을 호출합니다.
         *   - 출구 게이트인 경우 `insertExitStepBill`을 호출합니다.
         * - 데이터 삽입이 성공하면 HTTP 200 상태 코드와 함께 "Record added successfully" 메시지를 반환합니다.
         * - 실패하면 HTTP 500 상태 코드와 함께 "Failed to add record" 메시지를 반환합니다.
         *
         * 요청 예제:
         * @code
         * POST http://127.0.0.1:8080/records
         * Content-Type: application/json
         *
         * {
         *     "EntryTime": "2024-12-12T08:30:00",
         *     "PlateNumber": "ABC123",
         *     "GateNumber": 1
         * }
         * @endcode
         *
         * 성공 응답 예제:
         * @code
         * HTTP/1.1 200 OK
         * Content-Type: text/plain
         *
         * Record added successfully
         * @endcode
         *
         * 실패 응답 예제:
         * @code
         * HTTP/1.1 500 Internal Server Error
         * Content-Type: text/plain
         *
         * Failed to add record
         * @endcode
         *
         * @param sslSocket 응답을 전송하는 데 사용되는 SSL 소켓의 포인터.
         *
         * @throws std::runtime_error 데이터베이스 작업 또는 JSON 파싱 실패 시 예외를 던질 수 있습니다.
         * @return void
         */
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
            requestData.clear();
        }
        /**
         * @brief "/upload" 경로에 대한 POST 요청을 처리합니다.
         *
         * 이 함수는 "/upload" 경로로 들어오는 POST 요청을 처리하며,
         * JSON 형식으로 제공된 데이터를 기반으로 차량 번호판 정보와 이미지를 저장하고,
         * 해당 데이터를 데이터베이스에 기록합니다.
         *
         * @details
         * - 요청 본문에서 JSON 데이터를 추출하고 파싱합니다.
         * - JSON 데이터에서 `GateNum`, `PlateNum`, `time`, `imageWidth`, `imageHeight`, `imageRaw` 값을 추출합니다.
         * - `PlateNum`이 "UNKNOWN"이면 작업을 중단합니다.
         * - 차량 번호판 디렉터리가 존재하지 않으면 생성합니다.
         * - `time` 값을 기반으로 이미지 파일명을 생성하고, Base64로 인코딩된 이미지 데이터를 디코딩하여 저장합니다.
         * - 이미지 저장 성공 시 HTTP 200 상태 코드와 함께 성공 메시지를 반환합니다.
         * - 이미지 저장 실패 시 HTTP 400 상태 코드와 함께 실패 메시지를 반환합니다.
         * - 저장된 데이터를 `DatabaseManager`를 통해 데이터베이스에 기록합니다.
         *
         * 요청 예제:
         * @code
         * POST http://127.0.0.1:8080/upload
         * Content-Type: application/json
         *
         * {
         *     "GateNum": 1,
         *     "PlateNum": "ABC123",
         *     "time": "2024-12-12T08:30:00",
         *     "imageWidth": 640,
         *     "imageHeight": 480,
         *     "imageRaw": "Base64EncodedImageData"
         * }
         * @endcode
         *
         * 성공 응답 예제:
         * @code
         * HTTP/1.1 200 OK
         * Content-Type: text/plain
         *
         * Image saved successfully.
         * @endcode
         *
         * 실패 응답 예제 (이미지 저장 실패):
         * @code
         * HTTP/1.1 400 Bad Request
         * Content-Type: text/plain
         *
         * Image saved fail.
         * @endcode
         *
         * @param sslSocket 응답을 전송하는 데 사용되는 SSL 소켓의 포인터.
         *
         * @throws std::runtime_error JSON 파싱 실패, 파일 저장 실패, 또는 데이터베이스 작업 실패 시 예외를 던질 수 있습니다.
         * @return void
         */
        else if (method == "POST" && path == "/upload") {
            QString body = request.split("\r\n\r\n").last();
            //qDebug() << "body:" << body;

            // JSON �뙆�떛 �삤瑜� 寃��궗
            QJsonParseError parseError;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(body.toUtf8(), &parseError);

            if (parseError.error != QJsonParseError::NoError) {
                //qDebug() << "JSON Parse Error:" << parseError.errorString();
                requestData.clear();
                return;
            }

            QJsonObject jsonObject = jsonDoc.object();
            //qDebug() << "Parsed JSON Object:" << jsonObject;

            // JSON �뜲�씠�꽣 異붿텧
            int gateNumber = jsonObject.value("GateNum").toInt();
            QString plateNumber = jsonObject.value("PlateNum").toString().remove(' ');
            //if (plateNumber == "UNKNOWN") {
            //    return;
            //}
            QString time = jsonObject.value("time").toString();
            int imageWidth = jsonObject.value("imageWidth").toInt();
            int imageHeight = jsonObject.value("imageHeight").toInt();
            QString imageRawString = jsonObject.value("imageRaw").toString();

            //qDebug() << "imageWidth:" << imageWidth << "imageHeight:" << imageHeight << "plateNumber:" << plateNumber << "time:" << time;
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
                //qDebug() << "DB saved successfully.";
                requestData.clear();
            }else {
                //qDebug() << "DB saved fail.";
                requestData.clear();
            }
        }
        /**
         * @brief "/camera" 경로에 대한 POST 요청을 처리합니다.
         *
         * 이 함수는 "/camera" 경로로 들어오는 POST 요청을 처리하며,
         * JSON 형식으로 제공된 카메라 이름과 RTSP URL 정보를 데이터베이스에 저장합니다.
         *
         * @details
         * - 요청 본문에서 JSON 데이터를 추출하고 파싱합니다.
         * - JSON 데이터에서 `Camera_Name`과 `Camera_RTSP_URL` 값을 추출합니다.
         * - 값이 비어 있는 경우 HTTP 400 상태 코드와 함께 오류 메시지를 반환합니다.
         * - 데이터를 `DatabaseManager`를 통해 데이터베이스에 저장하고, 새로 추가된 카메라 ID를 반환받습니다.
         * - 카메라 추가에 성공하면 HTTP 200 상태 코드와 함께 성공 메시지와 새 카메라 ID를 JSON 형식으로 반환합니다.
         * - 카메라 추가에 실패하면 HTTP 500 상태 코드와 함께 오류 메시지를 반환합니다.
         *
         * 요청 예제:
         * @code
         * POST http://127.0.0.1:8080/camera
         * Content-Type: application/json
         *
         * {
         *     "Camera_Name": "Main Entrance",
         *     "Camera_RTSP_URL": "rtsp://192.168.1.100:554/stream1"
         * }
         * @endcode
         *
         * 성공 응답 예제:
         * @code
         * HTTP/1.1 200 OK
         * Content-Type: application/json
         *
         * {
         *     "message": "Camera added successfully",
         *     "camera_ID": 123
         * }
         * @endcode
         *
         * 실패 응답 예제:
         * @code
         * HTTP/1.1 500 Internal Server Error
         * Content-Type: text/plain
         *
         * Failed to add camera to the database
         * @endcode
         *
         * 오류 응답 예제 (필수 값 누락):
         * @code
         * HTTP/1.1 400 Bad Request
         * Content-Type: text/plain
         *
         * Missing Camera_Name or Camera_RTSP_URL
         * @endcode
         *
         * @param sslSocket 응답을 전송하는 데 사용되는 SSL 소켓의 포인터.
         *
         * @throws std::runtime_error JSON 파싱 실패 또는 데이터베이스 작업 실패 시 예외를 던질 수 있습니다.
         * @return void
         */
        else if (method == "POST" && path == "/camera") {
            //qDebug() << "Received :" << request;
            QString body = request.mid(request.indexOf("\r\n\r\n") + 4).trimmed(); // JSON 蹂몃Ц 異붿텧
            //qDebug() << "Received body:" << body;

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
            //qDebug() << "Camera Name:" << cameraName;
            //qDebug() << "RTSP URL:" << rtspUrl;

            int newCameraID = DatabaseManager::instance().addCamera(cameraName, rtspUrl);
            requestData.clear();

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
        }
        /**
         * @brief "/emails" 경로에 대한 POST 요청을 처리합니다.
         *
         * 이 함수는 "/emails" 경로로 들어오는 POST 요청을 처리하며,
         * JSON 형식으로 제공된 차량 번호와 이메일 정보를 데이터베이스에 추가하거나 업데이트합니다.
         *
         * @details
         * - 요청 본문에서 JSON 데이터를 추출하고 파싱합니다.
         * - JSON 데이터에서 `PlateNumber`와 `Email` 값을 추출합니다.
         * - 값이 비어 있는 경우 HTTP 400 상태 코드와 함께 오류 메시지를 반환합니다.
         * - `DatabaseManager`의 `addOrUpdateEmail` 메서드를 호출하여 데이터를 추가하거나 업데이트합니다.
         * - 데이터 업데이트에 성공하면 HTTP 200 상태 코드와 함께 성공 메시지를 반환합니다.
         * - 데이터 업데이트에 실패하면 HTTP 500 상태 코드와 함께 오류 메시지를 반환합니다.
         *
         * 요청 예제:
         * @code
         * POST http://127.0.0.1:8080/emails
         * Content-Type: application/json
         *
         * {
         *     "PlateNumber": "ABC123",
         *     "Email": "user@example.com"
         * }
         * @endcode
         *
         * 성공 응답 예제:
         * @code
         * HTTP/1.1 200 OK
         * Content-Type: text/plain
         *
         * Email information updated successfully
         * @endcode
         *
         * 실패 응답 예제:
         * @code
         * HTTP/1.1 500 Internal Server Error
         * Content-Type: text/plain
         *
         * Failed to update email information
         * @endcode
         *
         * 오류 응답 예제 (필수 값 누락):
         * @code
         * HTTP/1.1 400 Bad Request
         * Content-Type: text/plain
         *
         * Missing PlateNumber or Email
         * @endcode
         *
         * @param sslSocket 응답을 전송하는 데 사용되는 SSL 소켓의 포인터.
         *
         * @throws std::runtime_error JSON 파싱 실패 또는 데이터베이스 작업 실패 시 예외를 던질 수 있습니다.
         * @return void
         */
        else if (method == "POST" && path == "/emails") {
            QString body = request.mid(request.indexOf("\r\n\r\n") + 4).trimmed(); // JSON 蹂몃Ц 異붿텧
            QJsonDocument jsonDoc = QJsonDocument::fromJson(body.toUtf8());

            if (jsonDoc.isNull() || !jsonDoc.isObject()) {
                sendResponse(sslSocket, "Invalid JSON format", 400);
                //qDebug() << "Invalid JSON format1";
                requestData.clear();
                return;
            }

            QJsonObject jsonObject = jsonDoc.object();
            QString plateNumber = jsonObject.value("PlateNumber").toString();
            QString email = jsonObject.value("Email").toString();

            if (plateNumber.isEmpty() || email.isEmpty()) {
                sendResponse(sslSocket, "Missing PlateNumber or Email", 400);
                //qDebug() << "Missing PlateNumber or Email";
                requestData.clear();
                return;
            }

            bool success = DatabaseManager::instance().addOrUpdateEmail(plateNumber, email);
            if (success) {
                sendResponse(sslSocket, "Email information updated successfully", 200);
                //qDebug() << "Email information updated successfully";
            } else {
                sendResponse(sslSocket, "Failed to update email information", 500);
                //qDebug() << "Failed to update email information";
            }
            requestData.clear();
        }
        /**
         * @brief "/highPassRecord" 경로에 대한 POST 요청을 처리합니다.
         *
         * 이 함수는 "/highPassRecord" 경로로 들어오는 POST 요청을 처리하며,
         * JSON 형식으로 제공된 차량 번호판, 게이트 번호, 시간 정보를 처리하여 고속 통행 기록을 업데이트합니다.
         *
         * @details
         * - 요청 본문에서 JSON 데이터를 추출하고 파싱합니다.
         * - JSON 데이터에서 `PlateNumber`, `GateNumber`, `Time` 값을 추출합니다.
         * - 값이 비어 있거나 형식이 잘못된 경우 HTTP 400 상태 코드와 함께 오류 메시지를 반환합니다.
         * - `Time` 값은 "yyyy-MM-dd HH:mm:ss" 형식이어야 하며, 유효하지 않으면 오류를 반환합니다.
         * - `DatabaseManager`의 `processHighPassRecord` 메서드를 호출하여 고속 통행 기록을 처리합니다.
         * - 데이터 처리에 성공하면 HTTP 200 상태 코드와 함께 성공 메시지를 반환합니다.
         * - 데이터 처리에 실패하면 HTTP 500 상태 코드와 함께 오류 메시지를 반환합니다.
         *
         * 요청 예제:
         * @code
         * POST http://127.0.0.1:8080/highPassRecord
         * Content-Type: application/json
         *
         * {
         *     "PlateNumber": "ABC123",
         *     "GateNumber": 1,
         *     "Time": "2024-12-12 08:30:00"
         * }
         * @endcode
         *
         * 성공 응답 예제:
         * @code
         * HTTP/1.1 200 OK
         * Content-Type: text/plain
         *
         * Path updated successfully
         * @endcode
         *
         * 실패 응답 예제:
         * @code
         * HTTP/1.1 500 Internal Server Error
         * Content-Type: text/plain
         *
         * Failed to update Path
         * @endcode
         *
         * 오류 응답 예제 (필수 값 누락):
         * @code
         * HTTP/1.1 400 Bad Request
         * Content-Type: text/plain
         *
         * Missing PlateNumber, GateNumber, or Time
         * @endcode
         *
         * 오류 응답 예제 (시간 형식 오류):
         * @code
         * HTTP/1.1 400 Bad Request
         * Content-Type: text/plain
         *
         * Invalid time format. Use yyyy-MM-dd HH:mm:ss
         * @endcode
         *
         * @param sslSocket 응답을 전송하는 데 사용되는 SSL 소켓의 포인터.
         *
         * @throws std::runtime_error JSON 파싱 실패, 시간 형식 오류, 또는 데이터베이스 작업 실패 시 예외를 던질 수 있습니다.
         * @return void
         */
        else if (method == "POST" && path == "/highPassRecord") {
            QString body = request.mid(request.indexOf("\r\n\r\n") + 4).trimmed();
            QJsonDocument jsonDoc = QJsonDocument::fromJson(body.toUtf8());
            if (jsonDoc.isNull() || !jsonDoc.isObject()) {
                sendResponse(sslSocket, "Invalid JSON format", 400);
                requestData.clear();
                return;
            }

            QJsonObject jsonObject = jsonDoc.object();
            QString plateNumber = jsonObject.value("PlateNumber").toString();
            int gateNumber = jsonObject.value("GateNumber").toInt();
            QString timeStr = jsonObject.value("Time").toString();

            if (plateNumber.isEmpty() || gateNumber <= 0 || timeStr.isEmpty()) {
                sendResponse(sslSocket, "Missing PlateNumber, GateNumber, or Time", 400);
                requestData.clear();
                return;
            }

            // JSON�뿉�꽌 Time�쓣 媛��졇��� QDateTime�쑝濡� 蹂��솚
            QDateTime currentTime = QDateTime::fromString(timeStr, "yyyy-MM-dd HH:mm:ss");
            if (!currentTime.isValid()) {
                sendResponse(sslSocket, "Invalid time format. Use yyyy-MM-dd HH:mm:ss", 400);
                requestData.clear();
                return;
            }

            if (DatabaseManager::instance().processHighPassRecord(plateNumber, gateNumber, currentTime.toString("yyyy-MM-dd HH:mm:ss"))) {
                sendResponse(sslSocket, "Path updated successfully", 200);
            } else {
                sendResponse(sslSocket, "Failed to update Path", 500);
            }
            requestData.clear();
        } else {
            sendResponse(sslSocket, "Not Found", 404);
        }

        //sslSocket->flush();
        //sslSocket->disconnectFromHost();
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

