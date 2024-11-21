#ifndef DBCONTROLINTERFACE_H
#define DBCONTROLINTERFACE_H

#include "DatabaseManager.h"
#include <QStringList>

class DBControlInterface {
public:
    DBControlInterface(DatabaseManager& dbManager) : dbManager(dbManager) {}

    bool addHighPassRecord(const QString& entryTime, const QString& plateNumber, int gateNumber) {
        return dbManager.insertHighPassRecord(entryTime, plateNumber, gateNumber);
    }

    QList<QVariantMap> getAllRecords() {
        QList<QVariantMap> records;
        QString queryStr = "SELECT * FROM HIGHPASS_RECORD";
        QSqlQuery query = dbManager.executeSelectQuery(queryStr);

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

    QList<QVariantMap> getAllGates() {
        QList<QVariantMap> gateList;
        QString queryStr = "SELECT * FROM GATELIST";
        QSqlQuery query = dbManager.executeSelectQuery(queryStr);

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

private:
    DatabaseManager& dbManager;
};

#endif // DBCONTROLINTERFACE_H
