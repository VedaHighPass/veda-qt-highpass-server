#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QString>
#include <QDebug>
#include <QFile>

class DatabaseManager {
public:
    static DatabaseManager& instance();
    bool connectToDatabase(const QString& dbName);
    bool executeQuery(const QString& queryStr);
    QSqlQuery executeSelectQuery(const QString& queryStr);
    QList<QVariantMap> getAllRecords();
    QList<QVariantMap> getAllGates();
    bool checkIsEnterGate(int gateNumber);
    int addHighPassRecord(const QString& entryTime, const QString& plateNumber, int gateNumber);
    bool insertEnterStepBill(const QString& plateNumber, int enterGateID, int enterGateRecordID);
    bool insertExitStepBill(const QString& plateNumber, int exitGateID, int exitGateRecordID);

private:
    QSqlDatabase db;
    DatabaseManager();
    ~DatabaseManager();
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;
};

#endif // DATABASEMANAGER_H
