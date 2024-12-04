#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QString>
#include <QDebug>
#include <QFile>


struct DatabaseResult {
    QList<QVariantMap> records; // 조회된 레코드 리스트
    int totalRecords = 0;       // 총 레코드 수
};

/**
 * @class DatabaseManager
 * @brief Singleton class to handle database operations using SQLite.
 *
 * This class provides an interface for connecting to an SQLite database, executing queries, and managing records
 * related to high-pass data and billing information.
 *
 * @author InhakJeon
 */
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

    int addCamera(const QString& cameraName, const QString& rtspUrl);

    QList<QVariantMap> getAllCameras();

    DatabaseResult getRecordsByFilters(
        const QDate& startDate,
        const QDate& endDate,
        const QString& plateNumber,
        const QList<int>& entryGates,
        const QList<int>& exitGates,
        int pageSize, int page
        );
private:
    QSqlDatabase db; /**< The database connection object. */

    DatabaseManager();

    ~DatabaseManager();

    // Prevent copying and assignment
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;
};

#endif // DATABASEMANAGER_H
