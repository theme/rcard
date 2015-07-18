#include "frame.h"

Frame::Frame(QObject *parent) : QObject(parent),
    msb_(0), lsb_(0), data_(QByteArray())
{

}

Frame::Frame(unsigned int block, unsigned int frame, QByteArray data, QObject *parent) : QObject(parent),
    msb_(0), lsb_(0), data_(QByteArray())
{
    this->setIndex(block, frame);
    this->appendData(data);
}

Frame::Frame(Frame &other)
{
    msb_ = other.msb();
    lsb_ = other.lsb();
    data_ = other.data();
    sum_ = other.checksum();
    addr_ = other.addr();
}

char Frame::checksum()
{
    return sum_;
}

QString Frame::checksumHex()
{
    char s = this->checksum();
    QByteArray a(&s, 1);
    return QString(a.toHex());
}

bool Frame::isFull()
{
    return data_.size() == 128;
}

bool Frame::isEmpty()
{
    return data_.size() == 0;
}

QString Frame::dataHex()
{
    return QString ( data_.toHex()).toUpper();
}

QString Frame::indexString()
{
    return QString::number(block_) + "," + QString::number(frame_);
}

unsigned int Frame::block()
{
    return block_;
}

unsigned int Frame::frame()
{
    return frame_;
}

char Frame::msb()
{
    return msb_;
}

char Frame::lsb()
{
    return lsb_;
}

QByteArray Frame::data()
{
    return data_;
}

unsigned long Frame::addr()
{
    return addr_;
}

int Frame::appendData(QByteArray data)
{
    int i = 0;
    for (; i< data.size(); ++i){
        if ( data_.size() < 128) {
            data_.append(data.at(i));
            sum_ ^= data.at(i);
        } else {
            return i;
        }
    }
    return i;
}

void Frame::setIndex(unsigned int block, unsigned int frame)
{
    if ( block > 15 || frame > 63) return;
    /* block 0 - 15 , each 8KB*/
    /* frame 0 - 63 , each 128 B */
    block_ = block;
    frame_= frame;
    addr_ = 8 * 1024 * block + 128 * frame;

    msb_ = addr_;
    sum_ ^= msb_;
    lsb_ = (addr_>>8);
    sum_ ^= lsb_;
}

void Frame::clear()
{
    msb_ = lsb_ = sum_ = 0;
    data_.clear();
}

void Frame::setAddress(unsigned long addr)
{
    block_ = addr / (64 * 128);
    frame_ = (addr % (64 *128)) / 128;
    this->setIndex(block_, frame_);
}

