#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include "DatabaseManager.h"
#include <QMessageBox>
#include <QDateTime>
#include <QString>

#include "DatabaseManager.h"
#include "httpserver.h"
#include "videostream.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{

    ui->setupUi(this);
    ui_videostream = new videoStream();
    setCentralWidget(ui_videostream);
}

MainWindow::~MainWindow()
{
    delete ui;
}
