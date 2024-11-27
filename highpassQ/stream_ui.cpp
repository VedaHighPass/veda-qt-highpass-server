#include <QDebug>
#include <QString>
#include <QMenu>
#include <QTabWidget>

#include "stream_ui.h"
#include "ui_stream_ui.h"
#include "videostream.h"
#include "ui_videostream.h"
#include "rtpclient.h"

stream_ui::stream_ui(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::stream_ui)
{
    ui->setupUi(this);
    ui->widget_3->hide();
    connect(this,SIGNAL(signal_clikQuit()),rtpClient::instance(),SLOT(slot_quitBtn()));
    connect(rtpClient::instance(),SIGNAL(signal_ffmpeg_debug(QString)),this,SLOT(slot_ffmpeg_debug(QString)));
    connect(rtpClient::instance(),SIGNAL(signal_streaming_start()),this,SLOT(slot_streaming_start()));
    connect(rtpClient::instance(),SIGNAL(signal_video_start()),this,SLOT(slot_video_start()));
    connect(this,SIGNAL(send_url(QString)),rtpClient::instance(),SLOT(recv_url(QString)));
    connect(rtpClient::instance(),SIGNAL(signal_stream_fail()),this,SLOT(slot_streaming_fail()));

}

stream_ui::~stream_ui()
{
    delete ui;
}


void stream_ui::on_startBtn_clicked()
{
    rtpClient::instance()->videoLabel = ui->video_label;
    QString url = ui->lineEdit->text();
    //emit send_url(url);
    rtpClient::instance()->startFFmpegProcess(url);
    qDebug() << "start ffmpeg";
}


void stream_ui::on_pauseBtn_clicked()
{

}

void stream_ui::on_restartBtn_clicked()
{

}

void stream_ui::on_quitBtn_clicked()
{
    emit signal_clikQuit();
    this->hide();
}

void stream_ui::slot_streaming_start()
{
    ui->widget_3->show();
}
void stream_ui::slot_video_start()
{
    ui->widget_2->hide();
}
void stream_ui::slot_streaming_fail()
{
    ui->widget_2->show();
    ui->widget_3->hide();
}
