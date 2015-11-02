#include "memcard.h"

MemCard::MemCard(QObject *parent) : QObject(parent)
{ }

int MemCard::nextFrame()
{
    for (int i = 0; i <= 0x3FF; i++){
        if( !frames_.contains(i))
            return i;
    }
    return -1;
}

void MemCard::insertFrame(Frame *f)
{
    frames_.insert(f->findex(), f);
    if (isFull())
        emit sigFull();
}

bool MemCard::isFull()
{
    return nextFrame() < 0;
}

QByteArray MemCard::data()
{
    QByteArray data;
    for (int i = 0; i <= 0x3FF; i++){
        if( frames_.contains(i))
            data.append(frames_.value(i)->data());
        else
            data.append(QByteArray(128,0));
    }
    return data;
}

void MemCard::clear()
{
    frames_.clear();
}

