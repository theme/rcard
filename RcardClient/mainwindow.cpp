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
        connect(w, SIGNAL(clicked(bool)), this, SLOT(choosePort()));
        ui->gpPorts->layout()->addWidget(w);
    }

    connect(&rcard_timer_, SIGNAL(timeout()),
            this, SLOT(onRcardTimer()));

    connect(this, SIGNAL(sigFrameGot()),
            this, SLOT(saveFrame()));

    // auto select if only one serial port
    if ( all_porots_.length() == 1 ){
        QRadioButton *w = all_porots_.first();
        w->setChecked(true);
        sh_.setPort(w->text());
    }

    connect(&sh_, SIGNAL(sigGotAck()), this, SLOT(processACK()));
    connect(&sh_, SIGNAL(sigPortOpened(bool)), ui->portToggle, SLOT(setChecked(bool)));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::choosePort()
{
    foreach(QRadioButton *w, all_porots_){
        if(w->isChecked()){
            sh_.setPort(w->text());
            break;
        }
    }
}

void MainWindow::processACK()
{
    QByteArray bytes = sh_.ack();
    // TODO: debug:
    this->addText("DEBUG echo:" + bytes.toHex());
    return;

    int i;
    QByteArray ackdata(bytes.mid(1));
    switch((enum SERIALCMD)bytes.at(0)){
    case UNKNOWNCMD:
        this->addText(">> unknown cmd.");
        break;
    case CARDID:
        this->addText(">> card id: " + (unsigned long)(ackdata.at(0) << 8 + ackdata.at(1)));
        this->addText("   in ack data:" + ackdata.toHex());
        break;
    case ACKSETDELAY:
        this->addText(">> Delay set: " + ackdata.toHex());
        break;
    case ACKSETSPEEDDIV:
        this->addText(">> Speed Div set: " + ackdata.toHex());
        break;
    case FRAMEDATA:
        if ( frame_dbg_.isEmpty() ) {
            // skip header
            bytes = bytes.mid(11);
        }
        i = frame_dbg_.appendData(bytes) ;
        if ( i+1 <= bytes.size() ){
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
    default:
        this->addText(bytes.toHex());
    }
}

bool MainWindow::sendCmd(int cmd_enum, char msb, char lsb)
{
    QByteArray cmdarg;
    cmdarg.resize(3);
    cmdarg[0] = cmd_enum;
    cmdarg[1] = msb;
    cmdarg[2] = lsb;

    int acklen = 1;  // unknown cmd ack
    switch(cmd_enum){
    case READ:
        acklen = 1 + 10 + 128 + 2 + 8;
        break;
    case SETDELAY:
    case SETSPEEDDIV:
        acklen = 3;
        break;
    case GETID:
        acklen =  1 + 10;
        break;
    }
    if (!sh_.sendCmd(cmdarg, acklen)) return false;
    this->addText("<< " + cmdarg.toHex());
    return true;
}

void MainWindow::readFrame(int bindex, int findex)
{
    frame_dbg_.clear();
    frame_dbg_.setIndex(bindex,findex);
    if(this->sendCmd(READ, frame_dbg_.msb(), frame_dbg_.lsb()))
        this->addText("readFrame " + QString::number(bindex)
                      + " , " + QString::number(findex));

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
    if (!checked)
        sh_.closePort();
    else {
        if (sh_.openPort())
            this->addText("opened " + sh_.portName());
        else {
            this->addText("failed open " + sh_.portName());
            ui->portToggle->setChecked(false);
        }
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

void MainWindow::on_setSpeedDivBtn_clicked()
{
    unsigned int d = ui->speedDivValue->value();
    char msb = d >> 8;
    char lsb = d;
    this->sendCmd(SETSPEEDDIV, msb, lsb);
}
