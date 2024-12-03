#ifndef VIDEOSTREAM_H
#define VIDEOSTREAM_H

#include <QWidget>
#include <QMap>
class QTextEdit;
class rtpClient;
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

    void slot_ffmpeg_debug(QString error,rtpClient* textedit_key);

    void on_btnConnectDB_clicked();

    void on_btnInsertDB_clicked();

    void on_btnSearchDB_clicked();
    void slot_streaming_start();
    void slot_video_start();
    void slot_streaming_fail();
    void showContextMenu(const QPoint& pos);
    void addNewTab();
private:
    Ui::videoStream *ui;
    QMap <rtpClient*,QTextEdit*> map_textedit;
signals:
    void signal_clikQuit();
    //void signal_clikQuit();
   // void send_url(QString url);
};

#endif // VIDEOSTREAM_H
