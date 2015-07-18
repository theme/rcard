#include "memcard.h"

MemCard::MemCard(QObject *parent) : QObject(parent)
{

}

qint32 MemCard::needFrameAtAddr()
{
    quint32 addr;
    for (int b =0; b< 16; ++b){
        for (int f = 0; f< 64; ++f){
            addr = b * 64 * 128 + f * 128;
            if ( !frames_.contains(addr)){
                return addr;
            }
        }
    }
    return -1;  // full
}

void MemCard::insertFrame(Frame &f)
{
    frames_.insert(f.addr(), new Frame(&f));
    if (isFull())
        emit sigFull();
}

bool MemCard::isFull()
{
    if ( this->needFrameAtAddr() < 0 )
        return true;
    else
        return false;
}

QByteArray MemCard::data()
{
    QByteArray data;
    quint32 addr;
    for (int b =0; b< 16; ++b){
        for (int f = 0; f< 64; ++f){
            addr = b * 64 * 128 + f * 128;
            if ( !frames_.contains(addr)){
                data.append(frames_.value(addr)->data());
            } else {
                data.append(QByteArray(128,0x00));
            }
        }
    }
    return data;
}

void MemCard::clear()
{
    frames_.clear();
}

