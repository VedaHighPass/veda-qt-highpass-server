#include <QDebug>
#include <QString>
#include <QMenu>
#include <QTabWidget>
#include "stream_ui.h"
#include "ui_stream_ui.h"
#include "videostream.h"
#include "ui_videostream.h"

stream_ui::stream_ui(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::stream_ui)
{
    ui->setupUi(this);
    ui->widget_3->hide();
    url="";
    rtpCli = new rtpClient();
    rtpCli->videoLabel = ui->video_label;
}

stream_ui::~stream_ui()
{
    delete ui;
   // delete rtpCli;
}


void stream_ui::on_startBtn_clicked()
{

    connect(rtpCli,SIGNAL(signal_streaming_start()),this,SLOT(slot_streaming_start()));
    connect(rtpCli,SIGNAL(signal_video_start()),this,SLOT(slot_video_start()));
    connect(rtpCli,SIGNAL(signal_stream_fail()),this,SLOT(slot_streaming_fail()));
    connect(this,SIGNAL(signal_clikQuit()),rtpCli,SLOT(slot_quitBtn()));


    url = ui->lineEdit->text();
    //emit send_url(url);
    rtpCli->startFFmpegProcess(url);
    qDebug() << "start ffmpeg";
}


void stream_ui::on_pauseBtn_clicked()
{
    emit signal_clikQuit();
}

void stream_ui::on_restartBtn_clicked()
{
    //emit send_url(url);
    rtpCli->startFFmpegProcess(url);
    qDebug() << "start ffmpeg";
}

void stream_ui::on_quitBtn_clicked()
{
    emit signal_clikQuit();
    url = "";
    QPixmap pixmap(":/images/videostreamback.png");
    ui->video_label->setPixmap(pixmap);
    ui->widget_2->show();
    ui->widget_3->hide();
}

void stream_ui::slot_streaming_start()
{
    ui->widget_3->show();
}
void stream_ui::slot_video_start()
{
    ui->widget_2->hide();
   //ui->video_label->setPixmap();

}
void stream_ui::slot_streaming_fail()
{
    ui->widget_2->show();
    ui->widget_3->hide();
}
