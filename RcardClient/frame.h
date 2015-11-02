#ifndef FRAME_H
#define FRAME_H

#include <QObject>
#include "serialcmd.h"

class Frame : public QObject
{
    Q_OBJECT
public:
    explicit Frame( QByteArray ack, QObject *parent = 0);
    int findex() const;
    const QByteArray &data()const;
    char checksum();

    bool isGood();
private:
    static bool parseACK(const QByteArray &ack);
    QByteArray data_;
    char sum_;
    int findex_;
    bool good_;
};

#endif // FRAME_H
