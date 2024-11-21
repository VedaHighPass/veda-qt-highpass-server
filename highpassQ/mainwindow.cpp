#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include "DatabaseManager.h"
#include <QMessageBox>
#include "PlateRecordInterface.h"
#include <QDateTime>
#include <QString>

#include "DatabaseManager.h"
#include "httpserver.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

}

MainWindow::~MainWindow()
{
    delete ui;
}


QString getCurrentFormattedTime() {
    // 현재 시간을 가져옵니다.
    QDateTime currentTime = QDateTime::currentDateTime();\
    // 지정된 형식으로 시간을 포맷팅합니다.
    QString formattedTime = currentTime.toString("yyyy_MM_dd_HH:mm:ss.zz");
    return formattedTime;
}

void MainWindow::on_btnConnectDB_clicked()
{
    if (DatabaseManager::instance().connectToDatabase("../veda-qt-highpass-server/highpassQ/gotomars.db")) {
        QMessageBox::information(this, "Success", "Connected to gotomars.db");
    } else {
        QMessageBox::critical(this, "Error", "Failed to connect to gotomars.db");
    }
}


void MainWindow::on_btnSearchDB_clicked()
{
    PlateRecordInterface plateRecord(DatabaseManager::instance());
    QStringList records = plateRecord.getAllRecords();

    if (records.isEmpty()) {
        QMessageBox::information(this, "Plate Records", "No records found.");
    } else {
        QString recordStr = records.join("\n");
        QMessageBox::information(this, "Plate Records", recordStr);
    }
}

void MainWindow::on_btnInsertDB_clicked()
{
    //todo insert test code
    PlateRecordInterface plateRecord(DatabaseManager::instance());
    QString entryTime = QString("202411201550");
    QString plateNumber = QString("ABC1234");
    int gateNumber = rand()%100;

    if (plateRecord.addHighPassRecord(getCurrentFormattedTime(), plateNumber, gateNumber)) {
        QMessageBox::information(this, "Success", "Record inserted successfully.");
    } else {
        QMessageBox::critical(this, "Error", "Failed to insert record.");
    }
}

