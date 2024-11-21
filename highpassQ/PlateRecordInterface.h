#ifndef PLATERECORDINTERFACE_H
#define PLATERECORDINTERFACE_H

#include "DatabaseManager.h".h"
#include <QStringList>

class PlateRecordInterface {
public:
    PlateRecordInterface(DatabaseManager& dbManager) : dbManager(dbManager) {}

    bool addHighPassRecord(const QString& entryTime, const QString& plateNumber, int gateNumber) {
        return dbManager.insertHighPassRecord(entryTime, plateNumber, gateNumber);
    }

    QStringList getAllRecords() {
        QStringList records;
        QString queryStr = "SELECT * FROM HIGHPASS_RECORD";
        QSqlQuery query = dbManager.executeSelectQuery(queryStr);

        while (query.next()) {
            QString record = query.value(0).toString() + ", " + query.value(1).toString() + ", " + query.value(2).toString();
            records.append(record);
        }

        return records;
    }

private:
    DatabaseManager& dbManager;
};

#endif // PLATERECORDINTERFACE_H
