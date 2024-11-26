#ifndef VIDEOSTREAM_H
#define VIDEOSTREAM_H

#include <QWidget>

namespace Ui {
class videoStream;
}

class videoStream : public QWidget
{
    Q_OBJECT

public:
    explicit videoStream(QWidget *parent = nullptr);
    ~videoStream();

private slots:

    void on_startBtn_clicked();

    void on_pauseBtn_clicked();

    void on_restartBtn_clicked();

    void on_quitBtn_clicked();
    void slot_ffmpeg_debug(QString error);

    void on_btnConnectDB_clicked();

    void on_btnInsertDB_clicked();

    void on_btnSearchDB_clicked();
    void slot_streaming_start();
    void slot_video_start();
    void slot_streaming_fail();
private:
    Ui::videoStream *ui;
signals:
    void signal_clikQuit();
    void send_url(QString url);
};

#endif // VIDEOSTREAM_H
