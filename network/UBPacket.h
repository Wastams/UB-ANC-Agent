#ifndef UBPACKET_H
#define UBPACKET_H

#include <QObject>

class UBPacket : public QObject
{
    Q_OBJECT
public:
    explicit UBPacket(QObject *parent = 0);

signals:

public slots:
    void setSrcID(quint32 srcID) {m_srcID = srcID;}
    void setDesID(quint32 desID) {m_desID = desID;}
    void setPayload(const QByteArray& payload) {m_payload = payload;}

private:
    quint32 m_srcID;
    quint32 m_desID;

    QByteArray m_payload;

public:
    quint32 getSrcID(void) {return m_srcID;}
    quint32 getDesID(void) {return m_desID;}
    QByteArray getPayload(void) {return m_payload;}

    QByteArray packetize(void);
    void depacketize(const QByteArray &packet);
};

#endif // UBPACKET_H
