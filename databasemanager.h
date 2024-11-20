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
    static DatabaseManager& instance() {
        static DatabaseManager instance;
        return instance;
    }
    bool insertHighPassRecord(const QString& entryTime, const QString& plateNumber, int gateNumber) {
        QSqlQuery query;
        query.prepare("INSERT INTO HIGHPASS_RECORD (EntryTime, PlateNumber, GateNumber) "
                      "VALUES (:entryTime, :plateNumber, :gateNumber)");
        query.bindValue(":entryTime", entryTime);
        query.bindValue(":plateNumber", plateNumber);
        query.bindValue(":gateNumber", gateNumber);

        if (!query.exec()) {
            qDebug() << "Insert error:" << query.lastError().text();
            return false;
        }

        qDebug() << "Record inserted successfully.";
        return true;
    }

    bool connectToDatabase(const QString& dbName) {
        if (!QFile::exists("/home/server/veda-qt-highpass-server/gotomars.db")) {
            qDebug() << "Database file does not exist!";
            return false; // 또는 적절한 오류 처리
        }

        if (db.isOpen()) {
            return true; // 이미 연결됨
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

    bool executeQuery(const QString& queryStr) {
        QSqlQuery query;
        if (!query.exec(queryStr)) {
            qDebug() << "Query execution error:" << query.lastError().text();
            return false;
        }
        return true;
    }

    QSqlQuery executeSelectQuery(const QString& queryStr) {
        QSqlQuery query;
        if (!query.exec(queryStr)) {
            qDebug() << "Query execution error:" << query.lastError().text();
        }
        return query;
    }

private:
    QSqlDatabase db;

    DatabaseManager() {}
    ~DatabaseManager() {
        if (db.isOpen()) {
            db.close();
        }
    }

    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;
};

#endif // DATABASEMANAGER_H
