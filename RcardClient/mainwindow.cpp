#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    foreach( QSerialPortInfo i, QSerialPortInfo::availablePorts()){
        QRadioButton *w = new QRadioButton(i.portName(), this);
        all_porots_.append(w);
        connect(w, SIGNAL(clicked(bool)),
                this, SLOT(choosePort()));
        ui->gpPorts->layout()->addWidget(w);
    }

    connect(&port_, SIGNAL(readyRead()),
            this, SLOT(readPort()));

    // auto select if only one serial port
    if ( all_porots_.length() == 1 ){
        QRadioButton *w = all_porots_.first();
        w->setChecked(true);
        this->setPort(w->text());
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::choosePort()
{
    if (port_.isOpen())
        port_.close();

    foreach(QRadioButton *w, all_porots_){
        if(w->isChecked()){
            this->setPort(w->text());
            break;
        }
    }
}

void MainWindow::readPort()
{
    QByteArray bytes = port_.readAll();
    QString text = QString(bytes.toHex());
    this->addText(text.toUpper());

    int i;
    switch ( last_cmd_ ) {
    case CMD_READ:
        if ( frame_dbg_.isEmpty() ) {
            // skip header
            bytes = bytes.mid(11);
        }
        i = frame_dbg_.appendData(bytes) ;
        if ( i < bytes.size() ){
            // get remote check sum
            char checksum = bytes.at(i);
            char status = bytes.at(i+1);
            this->addText("remote checksum = " + char2Hex(checksum));
            this->addText("remote status = " + char2Hex(status));
        }
        if (frame_dbg_.isFull()) {
            this->addText("got frame, sum = "+
                          frame_dbg_.checksumHex());
            this->addText(frame_dbg_.dataHex());
            frame_dbg_.clear();
        }
        break;
    case CMD_ID:
        break;
    }
}

void MainWindow::sendCmd(int cmd_enum, char msb, char lsb)
{
    if (!port_.isOpen())
        this->openPort(port_.portName());

    char readcmd[] = {'R', msb, lsb};
    char idcmd[] = {'S'};
    char delaycmd[] = {'D', msb, lsb};

    switch(cmd_enum){
    case CMD_READ:
        port_.write(readcmd, sizeof readcmd);
        last_cmd_ = CMD_READ;
        break;
    case CMD_ID:
        port_.write(idcmd, sizeof idcmd);
        last_cmd_ = CMD_ID;
        break;
    case CMD_DELAY:
        port_.write(delaycmd, sizeof delaycmd);
        last_cmd_ = CMD_DELAY;
        break;
    }

    if (port_.error() != QSerialPort::NoError)
        this->addText("error write Serial."+ port_.errorString());

}

void MainWindow::readFrame(int block, int frame)
{
    this->addText("readFrame " + QString::number(block)
                  + " , " + QString::number(frame));
    frame_dbg_.clear();
    frame_dbg_.setIndex(block,frame);
    this->sendCmd(CMD_READ, frame_dbg_.msb(), frame_dbg_.lsb());
}

void MainWindow::on_chooseFileBtn_clicked()
{
    memcard_fn_ = openSaveFile();
    ui->fileName->setText(memcard_fn_);
}

QString MainWindow::openSaveFile()
{
    QString selfilter = tr("Memory Card File (*.mcr)");
    QString fn = QFileDialog::getSaveFileName( this,
                                               tr("Choose Memory Card file"),
                                               QDir::homePath(),
                                               tr("Memory Card File (*.mcr)"),
                                               &selfilter,
                                               QFileDialog::DontConfirmOverwrite );
    return fn;
}

void MainWindow::setPortParameters()
{
    port_.setBaudRate(QSerialPort::Baud38400);
    // arduino defauts to 8-n-1
    port_.setDataBits(QSerialPort::Data8);
    port_.setParity(QSerialPort::NoParity);
    port_.setStopBits(QSerialPort::OneStop);
}

void MainWindow::setPort(QString portName)
{
    port_.setPortName(portName);
    this->statusBar()->showMessage(portName);
}

void MainWindow::openPort(QString portName)
{
    if ( port_.portName() != portName) {
        if ( port_.isOpen()) {
            port_.close();
        }
        this->setPort( portName );
    }
    if ( port_.isOpen() )
        return;
    if (port_.open(QIODevice::ReadWrite)){  // open
        this->setPortParameters();
        this->addText(port_.portName() + " opened.");
        ui->portToggle->setChecked(true);
    } else {
        this->addText("error open " + port_.portName());
        return;
    }
}

void MainWindow::closePort()
{
    if (port_.isOpen()){
        port_.close();
        this->addText(port_.portName() + " closed.");
    }
    ui->portToggle->setChecked(false);
}

void MainWindow::addText(QString text)
{
    ui->text->appendPlainText(QTime::currentTime().toString() + "| "+ text);
}

QString MainWindow::char2Hex(char c)
{
    QByteArray a(&c,1);
    return QString(a.toHex()).toUpper();
}

void MainWindow::on_portToggle_toggled(bool checked)
{
    if (checked) {
        this->openPort(port_.portName());
    } else {
        this->closePort();
    }
}

void MainWindow::on_idButton_clicked()
{
    this->sendCmd(CMD_ID);
}

void MainWindow::on_readFrameBtn_clicked()
{
    this->readFrame(ui->blockIndex->value(),
                    ui->frameIndex->value());   // DEBUG
}

void MainWindow::on_setDelayBtn_clicked()
{
    unsigned long d = ui->delayValue->value();
    char msb = d >> 8;
    char lsb = d;
    this->sendCmd(CMD_DELAY, msb, lsb);
}
