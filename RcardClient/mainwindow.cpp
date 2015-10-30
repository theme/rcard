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

    connect(&rcard_timer_, SIGNAL(timeout()),
            this, SLOT(onRcardTimer()));

    connect(this, SIGNAL(sigFrameGot()),
            this, SLOT(saveFrame()));

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
    case READ:
        if ( frame_dbg_.isEmpty() ) {
            // skip header
            bytes = bytes.mid(11);
        }
        i = frame_dbg_.appendData(bytes) ;
        if ( i+1 < bytes.size() ){
            // get remote check sum
            char checksum = bytes.at(i);
            char status = bytes.at(i+1);
            if ( status == 0x47
                 && checksum == frame_dbg_.checksum()
                 && frame_dbg_.isFull()){
                emit sigFrameGot();
                this->addText("got frame "
                              + frame_dbg_.indexString());
                this->addText(frame_dbg_.dataHex());
            }
        }
        break;
    case GETID:
        break;
    default:
        break;
    }
}

void MainWindow::sendCmd(int cmd_enum, char msb, char lsb)
{
    if (!port_.isOpen())
        this->openPort(port_.portName());

    char readcmd[] = {READ, msb, lsb};
    char idcmd[] = {GETID};
    char delaycmd[] = {SETDELAY, msb, lsb};

    switch(cmd_enum){
    case READ:
        port_.write(readcmd, sizeof readcmd);
        break;
    case GETID:
        port_.write(idcmd, sizeof idcmd);
        break;
    case SETDELAY:
        port_.write(delaycmd, sizeof delaycmd);
        break;
    }
    last_cmd_ = cmd_enum;

    if (port_.error() != QSerialPort::NoError)
        this->addText("error write Serial."+ port_.errorString());

}

void MainWindow::readFrame(int block, int frame)
{
    this->addText("readFrame " + QString::number(block)
                  + " , " + QString::number(frame));
    frame_dbg_.clear();
    frame_dbg_.setIndex(block,frame);
    this->sendCmd(READ, frame_dbg_.msb(), frame_dbg_.lsb());
}

void MainWindow::on_chooseFileBtn_clicked()
{
    QString fn = openSaveFile();
    if ( fn != ui->fileName->text() )
    ui->fileName->setText(fn);
    card_.clear();
}

QString MainWindow::openSaveFile()
{
    QString selfilter = tr("Memory Card File (*.mcr)");
    QString fn = QFileDialog::getSaveFileName( this,
                                               tr("Choose Memory Card file"),
                                               QDir::homePath(),
                                               tr("Memory Card File (*.mcr)"),
                                               &selfilter);
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
    this->sendCmd(GETID);
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
    this->sendCmd(SETDELAY, msb, lsb);
}

void MainWindow::on_saveCardButton_clicked()
{
    rcard_timer_.start(1000);
}

void MainWindow::onRcardTimer()
{
    rcard_timer_.stop();
    qint32 addr;
    if ( !card_.isFull() ){
        // which frame is need ?
        addr  = card_.needFrameAtAddr();
        // set frame
        if ( 0 != frame_dbg_.addr() - addr){
            frame_dbg_.clear();
            frame_dbg_.setAddress(addr);
        }
        // read frame
        this->readFrame(frame_dbg_.block(),
                        frame_dbg_.frame());

        rcard_timer_.start(1000);
    } else {
        saveCard2File();
    }
}

void MainWindow::saveFrame()
{
    card_.insertFrame(frame_dbg_);
}

void MainWindow::saveCard2File()
{
    QFile f(ui->fileName);
    if (f.open(QIODevice::WriteOnly)){
        f.write(card_.data());
        f.close();
        this->addText(f.fileName() + " saved.");
    }
}

void MainWindow::on_stopReadButton_clicked()
{
    rcard_timer_.stop();
}
