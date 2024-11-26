#include <QDebug>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include "DatabaseManager.h"
#include <QMessageBox>
#include <QDateTime>
#include <QString>

#include "videostream.h"
#include "ui_videostream.h"
#include "rtpclient.h"
#include "DatabaseManager.h"
#include "httpserver.h"

QString getCurrentFormattedTime() {
    // 현재 시간을 가져옵니다.
    QDateTime currentTime = QDateTime::currentDateTime();\
    // 지정된 형식으로 시간을 포맷팅합니다.
    QString formattedTime = currentTime.toString("yyyy_MM_dd_HH:mm:ss.zz");
    return formattedTime;
}

videoStream::videoStream(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::videoStream)
{
    ui->setupUi(this);
    connect(this,SIGNAL(signal_clikQuit()),rtpClient::instance(),SLOT(slot_quitBtn()));
    connect(rtpClient::instance(),SIGNAL(signal_ffmpeg_debug(QString)),this,SLOT(slot_ffmpeg_debug(QString)));
}

videoStream::~videoStream()
{
    delete ui;
}

void videoStream::on_startBtn_clicked()
{
    rtpClient::instance()->videoLabel = ui->video_label;
    rtpClient::instance()->startFFmpegProcess();
    qDebug() << "start ffmpeg";
}


void videoStream::on_pauseBtn_clicked()
{

}


void videoStream::on_restartBtn_clicked()
{

}


void videoStream::on_quitBtn_clicked()
{
    emit signal_clikQuit();
    this->hide();
}

void videoStream::slot_ffmpeg_debug(QString error)
{
    ui->textEdit->append(error);
}


void videoStream::on_btnConnectDB_clicked()
{
    if (DatabaseManager::instance().connectToDatabase("/home/iam/veda_project/veda-qt-highpass-server/highpassQ/gotomars.db")) {
        QMessageBox::information(this, "Success", "Connected to gotomars.db");
    } else {
        QMessageBox::critical(this, "Error", "Failed to connect to gotomars.db");
    }
}


void videoStream::on_btnInsertDB_clicked()
{
    //todo insert test code
    QString entryTime = QString("202411201550");
    QString plateNumber = QString("ABC1234");
    int gateNumber = rand()%100;

    if (DatabaseManager::instance().addHighPassRecord(getCurrentFormattedTime(), plateNumber, gateNumber)) {
        QMessageBox::information(this, "Success", "Record inserted successfully.");
    } else {
        QMessageBox::critical(this, "Error", "Failed to insert record.");
    }
}


void videoStream::on_btnSearchDB_clicked()
{
    ui->textEdit_2->setText("");
    QList<QVariantMap> records = DatabaseManager::instance().getAllRecords();
    if (records.isEmpty()) {
        QMessageBox::information(this, "Plate Records", "No records found.");
    } else {
        //QStringList recordStrList;

        // QList<QVariantMap>의 각 레코드를 문자열로 변환하여 QStringList에 추가
        for (const QVariantMap &record : records) {
            QString recordStr = QString("ID: %1, EntryTime: %2, PlateNumber: %3, GateNumber: %4")
                                .arg(record["ID"].toInt())
                                .arg(record["EntryTime"].toString())
                                .arg(record["PlateNumber"].toString())
                                .arg(record["GateNumber"].toInt());
            ui->textEdit_2->append(recordStr);
        }
        ui->textEdit_2->append("\n");
        // 레코드를 줄바꿈으로 구분하여 메시지 박스에 표시
       // QString recordStr = recordStrList.join("\n");
        //QMessageBox::information(this, "Plate Records", recordStr);
    }
}

