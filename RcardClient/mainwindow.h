#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPortInfo>
#include <QSerialPort>
#include <QRadioButton>
#include <QFileDialog>
#include <QTime>
#include <QDebug>

#include "frame.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    enum CMD {
        CMD_READ,
        CMD_ID,
        CMD_DELAY
    };

private slots:
    void choosePort();
    void readPort();
    void sendCmd(int cmd_enum, char msb=0, char lsb=0);
    void readFrame(int block, int frame);

    void on_chooseFileBtn_clicked();

    void on_portToggle_toggled(bool checked);

    void on_idButton_clicked();

    void on_readFrameBtn_clicked();

    void on_setDelayBtn_clicked();

private:
    QString openSaveFile();
    void setPortParameters();
    void setPort(QString portName);
    void openPort(QString portName = QString());
    void closePort();
    void addText(QString text);
    QString char2Hex(char c);
    Ui::MainWindow *ui;
    QSerialPort port_;
    QList<QRadioButton*> all_porots_;
    QString memcard_fn_;
    int last_cmd_;

    Frame frame_dbg_;
};

#endif // MAINWINDOW_H
