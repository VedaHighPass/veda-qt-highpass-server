#include <QDebug>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include "DatabaseManager.h"
#include <QMessageBox>
#include <QDateTime>
#include <QString>
#include <QMenu>
#include <QTabWidget>

#include "videostream.h"
#include "ui_videostream.h"
#include "rtpclient.h"
#include "DatabaseManager.h"
#include "httpserver.h"
#include "stream_ui.h"


videoStream::videoStream(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::videoStream)
{
    ui->setupUi(this);
    ui->tabWidget_2->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(ui->tabWidget_2, &QTabWidget::customContextMenuRequested, this, &videoStream::showContextMenu);
}

QString getCurrentFormattedTime() {
    // 현재 시간을 가져옵니다.
    QDateTime currentTime = QDateTime::currentDateTime();\
    // 지정된 형식으로 시간을 포맷팅합니다.
    QString formattedTime = currentTime.toString("yyyy_MM_dd_HH:mm:ss.zz");
    return formattedTime;
}


videoStream::~videoStream()
{
    delete ui;
}

void videoStream::slot_ffmpeg_debug(QString error,rtpClient* textedit_key)
{
   if(map_textedit.contains(textedit_key))
   {
       map_textedit[textedit_key]->append(error);
   }
   qDebug()<<error;
   // ui->textEdit->append(error);
}


void videoStream::on_btnConnectDB_clicked()
{
    //if (DatabaseManager::instance().connectToDatabase("/home/iam/veda_project/veda-qt-highpass-server/highpassQ/gotomars.db")) {
    if (DatabaseManager::instance().connectToDatabase("../highpassQ/gotomars.db")) {
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

void videoStream:: showContextMenu(const QPoint& pos) {
    QMenu menu(this);
    QAction* addTabAction = menu.addAction("Add Tab");
    connect(addTabAction, &QAction::triggered, this, &videoStream::addNewTab);
    menu.exec(ui->tabWidget_2->mapToGlobal(pos));

}

 void videoStream:: addNewTab() {
    stream_ui* newTab = new stream_ui();
    QString tabName = QString("CAM %1").arg(ui->tabWidget_2->count());
    ui->tabWidget_2->addTab(newTab, tabName);
    QTextEdit *newDebug = new QTextEdit();
    ui->tabWidget->addTab(newDebug,QString("CAM %1").arg(ui->tabWidget_2->count()-1));
    connect(newTab->rtpCli,SIGNAL(signal_ffmpeg_debug(QString,rtpClient*)),this,SLOT(slot_ffmpeg_debug(QString,rtpClient*)));
    map_textedit.insert(newTab->rtpCli,newDebug);
     qDebug() << "Created new tab with stream_ui object at address: " << newTab;
    //newTab->show();
}
