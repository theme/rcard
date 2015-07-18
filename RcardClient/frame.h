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
    explicit Frame( Frame & other);
    char checksum();
    QString checksumHex();
    bool isFull();
    bool isEmpty();
    QString dataHex();
    QString indexString();

    unsigned int block();
    unsigned int frame();
    char msb();
    char lsb();
    QByteArray data();
    unsigned long addr();
signals:

public slots:
    int appendData(QByteArray data);
    void setIndex(unsigned int block, unsigned int frame);
    void clear();
    void setAddress(unsigned long addr);
private:
    unsigned int block_;
    unsigned int frame_;
    char msb_;
    char lsb_;
    QByteArray data_;
    char sum_;
    unsigned long addr_;
};

#endif // FRAME_H
