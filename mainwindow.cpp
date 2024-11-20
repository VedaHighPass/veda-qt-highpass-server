#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include "databasemanager.h"
#include <QMessageBox>
#include "PlateRecordInterface.h"

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


void MainWindow::on_BtnConnectDB_clicked()
{
    if (DatabaseManager::instance().connectToDatabase("/home/server/veda-qt-highpass-server/gotomars.db")) {
        QMessageBox::information(this, "Success", "Connected to gotomars.db");
    } else {
        QMessageBox::critical(this, "Error", "Failed to connect to gotomars.db");
    }
}


void MainWindow::on_BtnSearchDB_clicked()
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

