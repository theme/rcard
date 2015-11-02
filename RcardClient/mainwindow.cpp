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

    connect(&sh_, SIGNAL(sigGotAck()), this, SLOT(processACK()));
    connect(&sh_, SIGNAL(sigPortOpened(bool)), ui->portToggle, SLOT(setChecked(bool)));
    connect(&rcard_timer_, SIGNAL(timeout()), this, SLOT(onRcardTimer()));
    connect(&card_, SIGNAL(sigFull()), &rcard_timer_, SLOT(stop()));
    connect(&card_, SIGNAL(sigFull()), this, SLOT(saveCard2File()));

    // auto select if only one serial port
    if ( all_porots_.length() == 1 ){
        QRadioButton *w = all_porots_.first();
        w->setChecked(true);
        sh_.setPort(w->text());
    }
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
    QByteArray ack = sh_.ack();
    Frame *f = new Frame(ack, this);

    switch((enum SERIALCMD)ack.at(0)){
    case FRAMEDATA:
        this->addText(">> FRAMEDATA: " + ack.toHex());
        if( f->isGood() ) {
            this->addText("Good Frame " +  QString::number(f->findex()));
            if(!card_.isFull())
                card_.insertFrame(f);
        }
        break;
    default:
        this->addText(ack.toHex());
    }
}

bool MainWindow::sendCmd(unsigned char cmd_enum, unsigned char msb, unsigned char lsb)
{
    QByteArray cmdarg;
    cmdarg.resize(3);
    cmdarg[0] = cmd_enum;
    cmdarg[1] = msb;
    cmdarg[2] = lsb;

    int acklen = 1;  // unknown cmd ack
    switch(cmd_enum){
    case READFRAME:
        acklen = READFRAME_ACK_SIZE;
        break;
    case GETID:
        acklen = GETID_ACK_SIZE;
        break;
    default:
        acklen = DEFAULT_ACK_SIZE;
        break;
    }
    if (!sh_.sendCmd(cmdarg, acklen)) return false;
    this->addText("<< " + cmdarg.toHex());
    return true;
}

void MainWindow::readFrame(int findex)
{
    if(this->sendCmd(READFRAME, (findex>>8) & 0xFF, findex & 0xFF))
        this->addText("<< read frame " + QString::number(findex));
}

void MainWindow::on_chooseFileBtn_clicked()
{
    QString fn = dialogChooseSaveFileName();
    if ( fn != ui->fileName->text() ){
        ui->fileName->setText(fn);
        card_.clear();
    }
}

QString MainWindow::dialogChooseSaveFileName()
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
    if(this->sendCmd(GETID))
        this->addText("<< get id ");
}

void MainWindow::on_readFrameBtn_clicked()
{
    int bindex = ui->blockIndex->value();
    int findex = ui->frameIndex->value();
    this->readFrame(bindex * 64 + findex);
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
    rcard_timer_.setInterval(200);
    rcard_timer_.start();
}

void MainWindow::onRcardTimer()
{
    qDebug() << "onRcardTimer()";
    rcard_timer_.stop();
    if (!sh_.isOpen())
        return;

    int addr = card_.nextFrame();
    qDebug() << "nextFrame()" << QString::number(addr);

    if ( addr >= 0){
        this->readFrame(addr);
    }
    if( !card_.isFull())
        rcard_timer_.start();
}

void MainWindow::saveCard2File()
{
    QFile f(ui->fileName->text());
    if (f.open(QIODevice::WriteOnly)){
        qint64 c = f.write(card_.data());
        f.close();
        if ( c == card_.data().size() && card_.isFull())
            this->addText(f.fileName() + ": All card saved.");
        else if ( c == card_.data().size() && !card_.isFull())
            this->addText(f.fileName() + ": Partial card saved.");
    } else {    // not all data in card_ wrote to file
        this->addText("error open file for write: " + f.fileName() + " " + f.errorString());
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

void MainWindow::on_savePartialBtn_clicked()
{
    this->saveCard2File();
}
