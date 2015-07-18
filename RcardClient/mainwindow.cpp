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
    QByteArray bytes;
    char c;
    QString text;
    while ( port_.read(&c, 1)){
        bytes.append(c);
        text += QString(QByteArray::fromRawData(&c,1).toHex()).toUpper();
        text += " ";
    }
    this->addText(text);
}

void MainWindow::sendCmd(int cmd_enum)
{
    char readcmd[] = {'R', 0x0a, 0x0f};
    char idcmd[] = {'I'};

    switch(cmd_enum){
    case CMD_READ:
        port_.write(readcmd, sizeof readcmd);
        break;
    case CMD_ID:
        port_.write(idcmd, sizeof idcmd);
        break;
    }
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
    port_.setBaudRate(38400);
    port_.setDataBits(QSerialPort::Data8);
    port_.setStopBits(QSerialPort::OneStop);
    port_.setParity(QSerialPort::NoParity);
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
        this->setPortParameters();
    }
    if ( port_.isOpen() )
        return;
    if (port_.open(QIODevice::ReadWrite)){  // open
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

void MainWindow::on_readButton_clicked()
{
    this->openPort(port_.portName());
    this->sendCmd(CMD_READ);
}

void MainWindow::on_portToggle_toggled(bool checked)
{
    if (checked) {
        this->openPort(port_.portName());
    } else {
        this->closePort();
    }
}
