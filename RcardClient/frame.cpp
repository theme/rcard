#include "frame.h"

Frame::Frame(QObject *parent) : QObject(parent),
    msb_(0), lsb_(0), data_(QByteArray())
{

}

Frame::Frame(unsigned int msb, unsigned int lsb, QByteArray data, QObject *parent) : QObject(parent),
    msb_(0), lsb_(0), data_(QByteArray())
{
    this->setAddress(msb, lsb);
    this->appendData(data);
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

void Frame::setAddress(unsigned int msb, unsigned int lsb)
{
    msb_ = msb;
    sum_ ^= msb_;
    lsb_ = lsb;
    sum_ ^= lsb_;
}

void Frame::clear()
{
    msb_ = lsb_ = sum_ = 0;
    data_.clear();
}

