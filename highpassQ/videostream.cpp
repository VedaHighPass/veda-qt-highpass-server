
#include "videostream.h"
#include "ui_videostream.h"
#include "rtpclient.h"
#include "DatabaseManager.h"
#include "httpserver.h"
#include "stream_ui.h"`

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
#include <QNetworkAccessManager>
#include <opencv2/opencv.hpp>

#include <iostream>
#include <string>
#include <sstream>
#include <ctime>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>



#include <iostream>
#include <string>
#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <sstream>
#include <iomanip>
#include <vector>
#include <json.hpp>
#include <base64.h>  // base64 라이브러리 (별도 설치 필요)
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <openssl/ssl.h>
#include <openssl/err.h>


videoStream::videoStream(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::videoStream)
{
    ui->setupUi(this);
    ui->tabWidget_2->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(ui->tabWidget_2, &QTabWidget::customContextMenuRequested, this, &videoStream::showContextMenu);
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

QString getCurrentFormattedTime() {
    // 현재 시간을 가져옵니다.
    QDateTime currentTime = QDateTime::currentDateTime();\
    // 지정된 형식으로 시간을 포맷팅합니다.
    QString formattedTime = currentTime.toString("yyyy_MM_dd_HH:mm:ss.zz");
    return formattedTime;
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
using json = nlohmann::json;
void uploadImage(const std::string& serverAddress, int port) {
    // OpenCV로 500x600 크기의 빨간색 이미지 생성
    cv::Mat redImage(300, 400, CV_8UC3, cv::Scalar(0, 0, 255));  // 빨간색 (BGR)
    std::vector<uchar> buf;
    cv::imencode(".jpg", redImage, buf);
    std::string base64Image = base64_encode(buf.data(), buf.size());
    // JSON 데이터 생성
    json requestData = {
        {"GateNum", 1},
        {"PlateNum", "ABC123"},
        {"time", "2024-12-06T12:34:56"},
        {"imageRaw", base64Image},
        {"imageWidth", redImage.cols},
        {"imageHeight", redImage.rows}
    };
    // JSON 문자열로 변환
    std::string jsonData = requestData.dump();
    // OpenSSL 초기화
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    const SSL_METHOD* method = TLS_client_method();
    SSL_CTX* ctx = SSL_CTX_new(method);
    if (!ctx) {
        std::cerr << "Unable to create SSL context" << std::endl;
        ERR_print_errors_fp(stderr);
        return;
    }
    // 소켓 생성
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Socket creation failed!" << std::endl;
        SSL_CTX_free(ctx);
        return;
    }
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(serverAddress.c_str());
    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
        std::cerr << "Connection failed!" << std::endl;
        close(sock);
        SSL_CTX_free(ctx);
        return;
    }
    // SSL 연결 설정
    SSL* ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sock);
    if (SSL_connect(ssl) <= 0) {
        std::cerr << "SSL connection failed!" << std::endl;
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        close(sock);
        SSL_CTX_free(ctx);
        return;
    }
    // HTTP 요청 헤더 생성
    std::stringstream requestStream;
    requestStream << "POST /upload HTTP/1.1\r\n";
    requestStream << "Host: " << serverAddress << "\r\n";
    requestStream << "Content-Type: application/json\r\n";
    requestStream << "Content-Length: " << jsonData.size() << "\r\n";
    requestStream << "\r\n";
    requestStream << jsonData;
    // HTTP 요청 전송
    std::string request = requestStream.str();
    SSL_write(ssl, request.c_str(), request.size());
    // 서버 응답 받기
    char buffer[1024];
    int bytesRead = SSL_read(ssl, buffer, sizeof(buffer) - 1);
    if (bytesRead <= 0) {
        std::cerr << "Error reading response!" << std::endl;
        ERR_print_errors_fp(stderr);
    } else {
        buffer[bytesRead] = '\0';  // null terminate the string
        std::cout << "Server Response: " << buffer << std::endl;
    }
    // 연결 종료
    SSL_shutdown(ssl);
    SSL_free(ssl);
    close(sock);
    SSL_CTX_free(ctx);
}

void videoStream::on_btnUploadTest_clicked()
{
    std::string serverAddress = "127.0.0.1";  // 서버 주소
    int port = 8080;  // 포트 번호
    uploadImage(serverAddress, port);  // 이미지 업로드
}

