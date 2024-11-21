#include "DatabaseManager.h"

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

bool DatabaseManager::executeQuery(const QString& queryStr) {
    QSqlQuery query;
    if (!query.exec(queryStr)) {
        qDebug() << "Query execution error:" << query.lastError().text();
        return false;
    }
    return true;
}

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
