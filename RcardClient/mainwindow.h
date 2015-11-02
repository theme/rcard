#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QRadioButton>
#include <QFileDialog>
#include <QTime>
#include <QTimer>
#include <QDebug>

#include "memcard.h"
#include "serialhost.h"

extern "C" {
    #include "serialcmd.h"
}

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

signals:
    void sigFrameGot();

private slots:
    void choosePort();
    void processACK();

    bool sendCmd(int cmd_enum, char msb=0, char lsb=0);
    void readFrame(int bindex, int findex);

    void on_chooseFileBtn_clicked();

    void on_portToggle_toggled(bool checked);

    void on_idButton_clicked();

    void on_readFrameBtn_clicked();

    void on_setDelayBtn_clicked();

    void on_saveCardButton_clicked();

    void onRcardTimer();
    void saveFrame();
    void saveCard2File();
    void on_stopReadButton_clicked();

    void on_setSpeedDivBtn_clicked();

private:
    QString openSaveFile();
    void addText(QString text);
    QString char2Hex(char c);
    Ui::MainWindow *ui;
    QList<QRadioButton*> all_porots_;

    Frame frame_dbg_;
    MemCard card_;
    QTimer rcard_timer_;

    SerialHost sh_;
};

#endif // MAINWINDOW_H
