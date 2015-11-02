#include "frame.h"

Frame::Frame(QByteArray ack, QObject *parent) : QObject(parent),
    good_(false)
{
    if ( ack.size() != READFRAME_ACK_SIZE ) return;

    // header
    if (! ( 0x5A == ack.at(3) &&
            0x5D == ack.at(4) &&
            0x5C == ack.at(7) &&
            0x3F >= ack.at(9) ) )
        return;

    // check sum
    char checksum = ack.at(9) ^ ack.at(10);
    int i = 11;
    while (i < 11 + 128 ){ checksum ^= ack.at(i++); }
    if (checksum != ack.at(i)) return;

    sum_ = checksum;
    data_ = ack.mid(11,128);
    findex_ = (unsigned char)(ack.at(9));
    findex_ <<= 8;
    findex_ |= (unsigned char)(ack.at(10));
    good_ = true;
}

char Frame::checksum()
{ return sum_; }

bool Frame::isGood()
{ return good_; }

const QByteArray& Frame::data() const
{ return data_; }

int Frame::findex() const
{ return findex_; }
