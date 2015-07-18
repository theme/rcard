#ifndef FRAME_H
#define FRAME_H

#include <QObject>

class Frame : public QObject
{
    Q_OBJECT
public:
    explicit Frame(QObject *parent = 0);
    explicit Frame( unsigned int block,
                    unsigned int frame,
                    QByteArray data,
                    QObject *parent = 0);
    char checksum();
    QString checksumHex();
    bool isFull();
    bool isEmpty();
    QString dataHex();
    char msb();
    char lsb();
signals:

public slots:
    int appendData(QByteArray data);
    void setIndex(unsigned int block, unsigned int frame);
    void clear();
private:
    char msb_;
    char lsb_;
    QByteArray data_;
    char sum_;
    unsigned long addr_;
};

#endif // FRAME_H
