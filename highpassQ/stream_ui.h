#ifndef STREAM_UI_H
#define STREAM_UI_H

#include <QWidget>
#include "rtpclient.h"
namespace Ui {
class stream_ui;
}

class stream_ui : public QWidget
{
    Q_OBJECT

public:
    explicit stream_ui(QWidget *parent = nullptr);
    ~stream_ui() override;
    rtpClient* rtpCli;

private:
    Ui::stream_ui *ui;
    QString url;

protected:
    void resizeEvent(QResizeEvent *event) override;
private slots:
    void on_startBtn_clicked();

    void slot_streaming_start();
    void slot_video_start();
    void slot_streaming_fail();

signals:
    void signal_clikQuit();
};

#endif // STREAM_UI_H
