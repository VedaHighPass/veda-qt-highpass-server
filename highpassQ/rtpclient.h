#ifndef RTPCLIENT_H
#define RTPCLIENT_H

#include <QObject>
class QProcess;
class QLabel;
class rtpClient : public QObject {
Q_OBJECT

public:
    static rtpClient* instance();
    rtpClient();
    rtpClient(const rtpClient &) = delete;
    rtpClient &operator=(const rtpClient &) = delete;
    // FFmpeg 프로세스 시작 및 데이터 읽기 메서드
    void startFFmpegProcess(QString url);

    void readFFmpegOutput();

    // FFmpeg 프로세스 관리용 변수
    QProcess *ffmpegProcess = nullptr;
    QLabel *videoLabel = nullptr;

private:
    rtpClient();
    rtpClient(const rtpClient &) = delete;
    rtpClient &operator=(const rtpClient &) = delete;
    //QString streaming_url;
public slots:
    void slot_quitBtn();
   // void recv_url(QString url);
signals:
    void signal_ffmpeg_debug(QString,rtpClient*);
    void signal_streaming_start();
    void signal_video_start();
    void signal_stream_fail();
};

#endif // RTPCLIENT_H
