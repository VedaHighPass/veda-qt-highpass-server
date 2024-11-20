#ifndef PLATERECORDINTERFACE_H
#define PLATERECORDINTERFACE_H

#include "databasemanager.h"
#include <QStringList>

class PlateRecordInterface {
public:
    PlateRecordInterface(DatabaseManager& dbManager) : dbManager(dbManager) {}

    QStringList getAllRecords() {
        QStringList records;
        QString queryStr = "SELECT * FROM plateRecord";
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
