#ifndef FRAME_H
#define FRAME_H

#include <QObject>

class Frame : public QObject
{
    Q_OBJECT
public:
    explicit Frame(QObject *parent = 0);
    explicit Frame( unsigned int msb,
                    unsigned int lsb,
                    QByteArray data,
                    QObject *parent = 0);
    char checksum();
    QString checksumHex();
    bool isFull();
    bool isEmpty();
    QString dataHex();
signals:

public slots:
    int appendData(QByteArray data);
    void setAddress( unsigned int msb, unsigned int lsb);
    void clear();
private:
    unsigned int msb_;
    unsigned int lsb_;
    QByteArray data_;
    char sum_;
};

#endif // FRAME_H
