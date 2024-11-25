#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class videoStream;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_btnConnectDB_clicked();

    void on_btnSearchDB_clicked();

    void on_btnInsertDB_clicked();

    void on_pushButton_clicked();

private:
    Ui::MainWindow *ui;
    videoStream* ui_videostream;
};
#endif // MAINWINDOW_H
