#ifndef SERIALHOST_H
#define SERIALHOST_H

#include <QObject>
#include <QStateMachine>
#include <QSerialPortInfo>
#include <QSerialPort>
#include <QTimer>
#include <QDebug>

class SerialHost : public QObject
{
    Q_OBJECT
public:
    explicit SerialHost(QObject *parent = 0);
    // send out cmdarg bytes, set ack length and ack time out in ms
    bool sendCmd(const QByteArray& cmdarg, int acklen, int ackms = 10);
    const QByteArray &ack();
    void setPort(QString port);
    QString portName();

    bool openPort();
    void closePort();
    bool isOpen();
signals:
    void sigPortOpened(bool success = true);
    void sigPortClosed();
    void sigCmdSent();
    void sigGotAck();
private slots:
    void onPortReadyRead();
    void onNAEntered();
    void onCMDEntered();
    void onWAITACKEntered();
private:
    // idle, send cmd, waiting ACK
    QStateMachine sm_;
    QState *s_na ;	// serial port not ready
    QState *s_opened ;

    QState *s_cmd ;
    QState *s_waitAck ;

    QSerialPort port_;

    QByteArray cmd_;
    QByteArray ack_;
    int acklen_;
    QTimer ackTimer_;

    enum CURRENT_STATE { NA, CMD, WAITACK } stat_;
    QString dbgState_();
};

#endif // SERIALHOST_H
