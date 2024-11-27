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

    void slot_ffmpeg_debug(QString error);

    void on_btnConnectDB_clicked();

    void on_btnInsertDB_clicked();

    void on_btnSearchDB_clicked();
    void showContextMenu(const QPoint& pos);
    void addNewTab();
private:
    Ui::videoStream *ui;
signals:
    //void signal_clikQuit();
   // void send_url(QString url);
};

#endif // VIDEOSTREAM_H
