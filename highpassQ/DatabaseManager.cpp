#include "DatabaseManager.h"
#include <QDate>
#include <QSqlRecord>

DatabaseManager& DatabaseManager::instance() {
    static DatabaseManager instance;
    return instance;
}

DatabaseManager::DatabaseManager() {
    // 생성자에서 특별한 초기화 작업은 없지만, 필요하면 추가 가능합니다.
}

DatabaseManager::~DatabaseManager() {
    if (db.isOpen()) {
        db.close();
    }
}

int DatabaseManager::addCamera(const QString& cameraName, const QString& rtspUrl) {
    QSqlQuery query;
    query.prepare(
        "INSERT INTO \"CAMERA\" (\"Camera_Name\", \"Camera_RTSP_URL\") VALUES (:name, :rtspUrl)"
    );
    query.bindValue(":name", cameraName);   // TEXT 타입 컬럼에 안전하게 바인딩
    query.bindValue(":rtspUrl", rtspUrl);  // TEXT 타입 컬럼에 안전하게 바인딩

    if (query.exec()) {
        return query.lastInsertId().toInt();  // 새 카메라 ID 반환
    } else {
        qDebug() << "Database error:" << query.lastError().text();  // 디버그 메시지 출력
        return -1;  // 오류 발생 시 -1 반환
    }
}

QList<QVariantMap> DatabaseManager::getAllCameras() {
    QList<QVariantMap> cameras;
    QSqlQuery query("SELECT * FROM CAMERA");

    while (query.next()) {
        QVariantMap camera;
        camera["camera_ID"] = query.value("camera_ID").toInt();
        camera["Camera_Name"] = query.value("Camera_Name").toString();
        camera["Camera_RTSP_URL"] = query.value("Camera_RTSP_URL").toString();
        cameras.append(camera);
    }

    return cameras;
}

bool DatabaseManager::connectToDatabase(const QString& dbName) {
    if (!QFile::exists(dbName)) {
        qDebug() << "Database file does not exist!";
        return false;
    }

    if (db.isOpen()) {
        return true;
    }

    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(dbName);

    if (!db.open()) {
        qDebug() << "Database connection error:" << db.lastError().text();
        return false;
    }

    qDebug() << "Connected to database:" << dbName;
    return true;
}

// INSERT, UPDATE, DELETE
bool DatabaseManager::executeQuery(const QString& queryStr) {
    QSqlQuery query;
    if (!query.exec(queryStr)) {
        qDebug() << "Query execution error:" << query.lastError().text();
        return false;
    }
    return true;
}

// SELECT
QSqlQuery DatabaseManager::executeSelectQuery(const QString& queryStr) {
    QSqlQuery query;
    if (!query.exec(queryStr)) {
        qDebug() << "Query execution error:" << query.lastError().text();
    }
    return query;
}

QList<QVariantMap> DatabaseManager::getAllRecords() {
    QList<QVariantMap> records;
    QString queryStr = "SELECT * FROM HIGHPASS_RECORD";
    QSqlQuery query = executeSelectQuery(queryStr);

    while (query.next()) {
        QVariantMap record;
        record["ID"] = query.value(0).toInt();          // ID 컬럼
        record["EntryTime"] = query.value(1).toString(); // EntryTime 컬럼
        record["PlateNumber"] = query.value(2).toString(); // PlateNumber 컬럼
        record["GateNumber"] = query.value(3).toInt();   // GateNumber 컬럼
        records.append(record);
    }

    return records;
}

QList<QVariantMap> DatabaseManager::getAllGates() {
    QList<QVariantMap> gateList;
    QString queryStr = "SELECT * FROM GATELIST";
    QSqlQuery query = executeSelectQuery(queryStr);

    while (query.next()) {
        QVariantMap gateRecord;
        gateRecord["GateNumber"] = query.value(0).toInt();
        gateRecord["GateName"] = query.value(1).toString();
        gateRecord["isEnterGate"] = query.value(2).toBool();
        gateRecord["isExitGate"] = query.value(3).toBool();
        gateList.append(gateRecord);
    }

    return gateList;
}

//GATELIST에서 gaetNumber가 출구번호면 true, 아니면 false
bool DatabaseManager::checkIsEnterGate(int gateNumber) {
    QSqlQuery query;
    query.prepare("SELECT isEnterGate, isExitGate FROM GATELIST WHERE GateNumber = :GateNumber");
    query.bindValue(":GateNumber", gateNumber);

    if (query.exec() && query.next()) {
        bool isEnterGate = query.value("isEnterGate").toBool();
        bool isExitGate = query.value("isExitGate").toBool();

        if (isEnterGate) {
            return true; // Enter Gate
        } else if (isExitGate) {
            return false; // Exit Gate
        } else {
            qWarning() << "GateNumber " << gateNumber << " is neither an Enter nor Exit Gate.";
            return false; // 유효하지 않은 게이트
        }
    } else {
        qWarning() << "Failed to retrieve gate information for GateNumber:" << gateNumber;
        return false; // GateNumber에 해당하는 레코드가 없거나 쿼리 오류
    }
}

int DatabaseManager::addHighPassRecord(const QString& entryTime, const QString& plateNumber, int gateNumber) {
    QSqlQuery query;
    db.transaction(); // 트랜잭션 시작

    query.prepare("INSERT INTO HIGHPASS_RECORD (EntryTime, PlateNumber, GateNumber) "
                  "VALUES (:entryTime, :plateNumber, :gateNumber)");
    query.bindValue(":entryTime", entryTime);
    query.bindValue(":plateNumber", plateNumber);
    query.bindValue(":gateNumber", gateNumber);

    if (!query.exec()) {
        qDebug() << "Insert error:" << query.lastError().text();
        db.rollback(); // 실패 시 롤백
        return -1;
    }

    int lastInsertedId = query.lastInsertId().toInt(); // 자동 생성된 ID를 가져옴
    db.commit(); // 트랜잭션 커밋
    return lastInsertedId;
}

bool DatabaseManager::insertEnterStepBill(const QString& plateNumber, int enterGateID, int enterGateRecordID) {
    QSqlQuery query;
    query.prepare("INSERT INTO BILL (PlateNumber, EnterGateID, EnterGateRecordID) "
                  "VALUES (:PlateNumber, :EnterGateID, :EnterGateRecordID)");
    query.bindValue(":PlateNumber", plateNumber);
    query.bindValue(":EnterGateID", enterGateID);
    query.bindValue(":EnterGateRecordID", enterGateRecordID);

    if (query.exec()) {
        return true; // 성공적으로 삽입
    } else {
        qWarning() << "Error inserting first step bill:" << query.lastError().text();
        return false; // 오류 발생
    }
}

bool DatabaseManager::insertExitStepBill(const QString& plateNumber, int exitGateID, int exitGateRecordID) {
    QSqlQuery query;
    query.prepare("SELECT BillID FROM BILL WHERE PlateNumber = :PlateNumber AND ExitGateID IS NULL");
    query.bindValue(":PlateNumber", plateNumber);

    if (query.exec() && query.next()) {
        int billID = query.value(0).toInt();

        query.prepare("UPDATE BILL SET ExitGateID = :ExitGateID, ExitGateRecordID = :ExitGateRecordID WHERE BillID = :BillID");
        query.bindValue(":ExitGateID", exitGateID);
        query.bindValue(":ExitGateRecordID", exitGateRecordID);
        query.bindValue(":BillID", billID);

        if (query.exec()) {
            return true; // 성공적으로 업데이트
        } else {
            qWarning() << "Error updating second step bill:" << query.lastError().text();
            return false; // 오류 발생
        }
    } else {
        qWarning() << "No record found for PlateNumber:" << plateNumber;
        return false; // 해당 PlateNumber가 없거나 첫 번째 입력이 없는 경우
    }
}


DatabaseResult DatabaseManager::getRecordsByFilters(
    const QDate& startDate,
    const QDate& endDate,
    const QString& plateNumber,
    const QList<int>& entryGates,
    const QList<int>& exitGates,
    int pageSize, int page
    ) {
    DatabaseResult result;

    // Base SQL query with JOIN to include Email
    QString baseQuery = R"(
        FROM HIGHPASS_RECORD hr
        LEFT JOIN EMAILS e ON hr.PlateNumber = e.PlateNumber
        WHERE DATE(hr.EntryTime) BETWEEN :startDate AND :endDate
    )";

    // Add plate number condition
    if (!plateNumber.isEmpty()) {
        baseQuery += " AND hr.PlateNumber = :plateNumber";
    }

    // Add entry gates condition
    if (!entryGates.isEmpty()) {
        QStringList entryPlaceholders;
        for (int i = 0; i < entryGates.size(); ++i) {
            entryPlaceholders.append(QString(":entryGate%1").arg(i));
        }
        baseQuery += QString(" AND hr.EntryGateNumber IN (%1)").arg(entryPlaceholders.join(","));
    }

    // Add exit gates condition
    if (!exitGates.isEmpty()) {
        QStringList exitPlaceholders;
        for (int i = 0; i < exitGates.size(); ++i) {
            exitPlaceholders.append(QString(":exitGate%1").arg(i));
        }
        baseQuery += QString(" AND hr.ExitGateNumber IN (%1)").arg(exitPlaceholders.join(","));
    }

    // Query to calculate total record count
    QString countQueryStr = "SELECT COUNT(*) " + baseQuery;
    QSqlQuery countQuery;
    countQuery.prepare(countQueryStr);
    countQuery.bindValue(":startDate", startDate.toString("yyyy-MM-dd"));
    countQuery.bindValue(":endDate", endDate.toString("yyyy-MM-dd"));
    if (!plateNumber.isEmpty()) {
        countQuery.bindValue(":plateNumber", plateNumber);
    }
    for (int i = 0; i < entryGates.size(); ++i) {
        countQuery.bindValue(QString(":entryGate%1").arg(i), entryGates[i]);
    }
    for (int i = 0; i < exitGates.size(); ++i) {
        countQuery.bindValue(QString(":exitGate%1").arg(i), exitGates[i]);
    }

    if (!countQuery.exec()) {
        qWarning() << "Error executing count query:" << countQuery.lastError().text();
        return result;
    }

    if (countQuery.next()) {
        result.totalRecords = countQuery.value(0).toInt();
    }

    // Query to fetch paginated records
    QString dataQueryStr = R"(
        SELECT hr.*, e.Email
    )" + baseQuery + " LIMIT :limit OFFSET :offset";

    QSqlQuery dataQuery;
    dataQuery.prepare(dataQueryStr);
    dataQuery.bindValue(":startDate", startDate.toString("yyyy-MM-dd"));
    dataQuery.bindValue(":endDate", endDate.toString("yyyy-MM-dd"));
    if (!plateNumber.isEmpty()) {
        dataQuery.bindValue(":plateNumber", plateNumber);
    }
    for (int i = 0; i < entryGates.size(); ++i) {
        dataQuery.bindValue(QString(":entryGate%1").arg(i), entryGates[i]);
    }
    for (int i = 0; i < exitGates.size(); ++i) {
        dataQuery.bindValue(QString(":exitGate%1").arg(i), exitGates[i]);
    }

    // Calculate LIMIT and OFFSET for pagination
    int offset = (page - 1) * pageSize;
    dataQuery.bindValue(":limit", pageSize);
    dataQuery.bindValue(":offset", offset);

    if (!dataQuery.exec()) {
        qWarning() << "Error executing data query:" << dataQuery.lastError().text();
        return result;
    }

    // Process query results
    while (dataQuery.next()) {
        QVariantMap record;
        record["ID"] = dataQuery.value("ID").toInt();
        record["PlateNumber"] = dataQuery.value("PlateNumber").toString();
        record["EntryTime"] = dataQuery.value("EntryTime").toString();
        record["EntryGateNumber"] = dataQuery.value("EntryGateNumber").toInt();
        record["ExitTime"] = dataQuery.value("ExitTime").toString();
        record["ExitGateNumber"] = dataQuery.value("ExitGateNumber").toInt();
        record["Path"] = dataQuery.value("Path").toString();
        record["Email"] = dataQuery.value("Email").toString();  // Add email
        result.records.append(record);
    }

    return result;
}

bool DatabaseManager::addOrUpdateEmail(const QString& plateNumber, const QString& email) {
    QSqlQuery query;
    query.prepare("INSERT INTO Emails (PlateNumber, Email) VALUES (:plateNumber, :email) "
                  "ON CONFLICT(PlateNumber) DO UPDATE SET Email = :email;");
    query.bindValue(":plateNumber", plateNumber);
    query.bindValue(":email", email);

    if (!query.exec()) {
        qDebug() << "Failed to update email:" << query.lastError().text();
        return false;
    }
    return true;
}
