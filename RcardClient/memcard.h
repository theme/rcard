#ifndef MEMCARD_H
#define MEMCARD_H

#include <QObject>
#include <QHash>
#include "frame.h"

class MemCard : public QObject
{
    Q_OBJECT
public:
    explicit MemCard(QObject *parent = 0);
    qint32 needFrameAtAddr();
    void insertFrame(Frame &f);
    bool isFull();
    QByteArray data();
signals:
    void sigFull();

public slots:
    void clear();
private:
    QHash< quint32, Frame* > frames_;
};

#endif // MEMCARD_H
