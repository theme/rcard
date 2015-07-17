#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPortInfo>
#include <QSerialPort>
#include <QRadioButton>
#include <QFileDialog>
#include <QDebug>

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
        CMD_ID
    };

private slots:
    void choosePort();
    void readPort();
    void sendCmd(int cmd_enum);

    void on_chooseFileBtn_clicked();

    void on_saveButton_clicked();

private:
    QString openSaveFile();
    void setPortParameters();
    Ui::MainWindow *ui;
    QSerialPort port_;
    QList<QRadioButton*> all_porots_;
    QString memcard_fn_;
};

#endif // MAINWINDOW_H
