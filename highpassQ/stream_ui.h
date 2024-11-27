#ifndef STREAM_UI_H
#define STREAM_UI_H

#include <QWidget>

namespace Ui {
class stream_ui;
}

class stream_ui : public QWidget
{
    Q_OBJECT

public:
    explicit stream_ui(QWidget *parent = nullptr);
    ~stream_ui();

private:
    Ui::stream_ui *ui;

private slots:
    void on_startBtn_clicked();

    void on_pauseBtn_clicked();

    void on_restartBtn_clicked();

    void on_quitBtn_clicked();

    void slot_streaming_start();
    void slot_video_start();
    void slot_streaming_fail();

signals:
    void signal_clikQuit();
};

#endif // STREAM_UI_H
