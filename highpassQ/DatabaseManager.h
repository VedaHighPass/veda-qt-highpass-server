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
    /**
     * @brief Gets the singleton instance of the DatabaseManager.
     *
     * This function ensures that only one instance of the DatabaseManager is created during the application's
     * lifetime.
     *
     * @return A reference to the single instance of DatabaseManager.
     */
    static DatabaseManager& instance();

    /**
     * @brief Connects to the SQLite database.
     *
     * Establishes a connection to the specified SQLite database.
     *
     * @param dbName The name of the database file to connect to.
     * @return True if the connection is successful, false otherwise.
     */
    bool connectToDatabase(const QString& dbName);

    /**
     * @brief Executes a query on the database.
     *
     * This function executes a non-SELECT SQL query, such as an INSERT, UPDATE, or DELETE query.
     *
     * @param queryStr The SQL query string to execute.
     * @return True if the query was executed successfully, false if there was an error.
     */
    bool executeQuery(const QString& queryStr);

    /**
     * @brief Executes a SELECT query on the database.
     *
     * This function executes a SELECT SQL query and returns the result as a QSqlQuery object.
     *
     * @param queryStr The SQL SELECT query string to execute.
     * @return The QSqlQuery object containing the result of the SELECT query.
     */
    QSqlQuery executeSelectQuery(const QString& queryStr);

    /**
     * @brief Retrieves all records from the HIGHPASS_RECORD table.
     *
     * This function retrieves all records from the HIGHPASS_RECORD table and returns them as a list of QVariantMaps.
     *
     * @return A list of QVariantMaps where each map represents a record from the HIGHPASS_RECORD table.
     */
    QList<QVariantMap> getAllRecords();

    /**
     * @brief Retrieves all gate records from the GATELIST table.
     *
     * This function retrieves all gate records from the GATELIST table and returns them as a list of QVariantMaps.
     *
     * @return A list of QVariantMaps where each map represents a gate record from the GATELIST table.
     */
    QList<QVariantMap> getAllGates();

    /**
     * @brief Checks if the specified gate number corresponds to an entry gate.
     *
     * This function checks whether the gate specified by gateNumber is an entry gate or not.
     *
     * @param gateNumber The gate number to check.
     * @return True if the gate is an entry gate, false if it is an exit gate.
     */
    bool checkIsEnterGate(int gateNumber);

    /**
     * @brief Adds a high-pass record to the HIGHPASS_RECORD table.
     *
     * This function inserts a new record into the HIGHPASS_RECORD table with the provided entry time, plate number,
     * and gate number.
     *
     * @param entryTime The entry time for the record.
     * @param plateNumber The plate number associated with the record.
     * @param gateNumber The gate number where the record is associated.
     * @return The ID of the newly inserted record if successful, -1 if an error occurs.
     */
    int addHighPassRecord(const QString& entryTime, const QString& plateNumber, int gateNumber);

    /**
     * @brief Inserts an enter step bill record into the BILL table.
     *
     * This function inserts a new bill record related to an entry gate into the BILL table.
     *
     * @param plateNumber The plate number associated with the bill.
     * @param enterGateID The ID of the enter gate.
     * @param enterGateRecordID The ID of the record in the HIGHPASS_RECORD table associated with the entry gate.
     * @return True if the bill is inserted successfully, false otherwise.
     */
    bool insertEnterStepBill(const QString& plateNumber, int enterGateID, int enterGateRecordID);

    /**
     * @brief Inserts an exit step bill record into the BILL table.
     *
     * This function inserts a new bill record related to an exit gate into the BILL table.
     *
     * @param plateNumber The plate number associated with the bill.
     * @param exitGateID The ID of the exit gate.
     * @param exitGateRecordID The ID of the record in the HIGHPASS_RECORD table associated with the exit gate.
     * @return True if the exit step bill is updated successfully, false otherwise.
     */
    bool insertExitStepBill(const QString& plateNumber, int exitGateID, int exitGateRecordID);

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

    /**
     * @brief Private constructor for the DatabaseManager singleton class.
     *
     * This constructor is private to prevent direct instantiation of the class.
     */
    DatabaseManager();

    /**
     * @brief Destructor for the DatabaseManager class.
     *
     * Closes the database connection when the DatabaseManager instance is destroyed.
     */
    ~DatabaseManager();

    // Prevent copying and assignment
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;
};

#endif // DATABASEMANAGER_H
