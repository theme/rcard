#include "serialhost.h"

SerialHost::SerialHost(QObject *parent) :
    QObject(parent)
{
    s_na = new QState();
    s_opened = new QState();
    s_cmd = new QState(s_opened);
    s_waitAck = new QState(s_opened);
    s_opened->setInitialState(s_cmd);

    s_na->addTransition(this, SIGNAL(sigPortOpened()),s_opened);
    s_opened->addTransition(this, SIGNAL(sigPortClosed()), s_na);
    s_cmd->addTransition(this, SIGNAL(sigCmdSent()), s_waitAck);
    s_waitAck->addTransition(this, SIGNAL(sigGotAck()), s_cmd);
    s_waitAck->addTransition(&ackTimer_, SIGNAL(timeout()), s_cmd);

    connect(s_na, SIGNAL(entered()), this, SLOT(onNAEntered()));
    connect(s_cmd, SIGNAL(entered()), this, SLOT(onCMDEntered()));
    connect(s_waitAck, SIGNAL(entered()), this, SLOT(onWAITACKEntered()));
    connect(s_waitAck, SIGNAL(entered()), &ackTimer_, SLOT(start()));

    sm_.addState(s_na);
    sm_.addState(s_opened);
    sm_.setInitialState(s_na);
    sm_.start();

    connect(&port_, SIGNAL(readyRead()), this, SLOT(onPortReadyRead()));
}

bool SerialHost::sendCmd(const QByteArray &cmdarg, int acklen, int ackms)
{
    if(CMD != stat_) {
        qDebug() << " not in CMD state: " << this->dbgState_();
        return false;
    }

    if(!this->openPort()) return false;

    if(!port_.clear()) return false;

    int sent = port_.write(cmdarg);
    if (sent != cmdarg.size()) return false;

    acklen_ = acklen;
    ack_.clear();

    ackTimer_.setSingleShot(true);
    ackTimer_.setInterval(ackms);

    emit sigCmdSent();
    return true;
}

const QByteArray &SerialHost::ack()
{
    return ack_;
}

void SerialHost::setPort(QString port)
{
    if (port_.portName() == port) return;

    this->closePort();
    port_.setPortName(port);
}

QString SerialHost::portName()
{
    return port_.portName();
}

bool SerialHost::openPort()
{
    if (port_.isOpen()) return true;

    if(!port_.open(QIODevice::ReadWrite)){ return false; }

    port_.setBaudRate(QSerialPort::Baud38400);
    // arduino defauts to 8-n-1
    port_.setDataBits(QSerialPort::Data8);
    port_.setParity(QSerialPort::NoParity);
    port_.setStopBits(QSerialPort::OneStop);
    emit sigPortOpened();
    return true;
}

void SerialHost::closePort()
{
    port_.close();
    emit sigPortClosed();
    qDebug() << port_.portName() << " closed.";
}

bool SerialHost::isOpen()
{
    return port_.isOpen();
}

void SerialHost::onPortReadyRead()
{
    ack_.append(port_.readAll());

    if (ack_.size() >= acklen_){
        ackTimer_.stop();
        ack_.resize(acklen_);
        emit sigGotAck();
    }
}

void SerialHost::onNAEntered()
{
    stat_ = NA;
}

void SerialHost::onCMDEntered()
{
    stat_ = CMD;
}

void SerialHost::onWAITACKEntered()
{
    stat_ = WAITACK;
}

QString SerialHost::dbgState_()
{
    switch( stat_ ){
    case NA:
        return "NA";
    case CMD:
        return "CMD";
    case WAITACK:
        return "WAITACK";
    }
    return QString();
}
