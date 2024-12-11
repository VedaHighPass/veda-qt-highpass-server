
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

    std::string img = "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";

    // JSON 데이터 생성
    json requestData = {
        {"GateNum", 1},
        {"PlateNum", "ABC123"},
        {"time", "2024-12-06T12:34:56"},
        //{"imageRaw", base64Image},
        {"imageRaw", img},
        //{"imageWidth", redImage.cols},
        {"imageWidth", 288},
        //{"imageHeight", redImage.rows}
        {"imageHeight", 96}
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

