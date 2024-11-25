#include "videostream.h"
#include "ui_videostream.h"
#include "rtpclient.h"
#include <QDebug>
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

