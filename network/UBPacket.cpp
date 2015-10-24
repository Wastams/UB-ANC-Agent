#include "UBPacket.h"

UBPacket::UBPacket(QObject *parent) : QObject(parent),
    m_srcID(0),
    m_desID(0)
{

}

QByteArray UBPacket::packetize(void) {
    QByteArray src(((char*)(&m_srcID)), sizeof(m_srcID));
    QByteArray des(((char*)(&m_desID)), sizeof(m_desID));

    QByteArray payload(m_payload);

    return src + des + payload;
}

void UBPacket::depacketize(const QByteArray& packet) {
    const char* data = packet.data();

    m_srcID = *((quint32*)(data));
    m_desID = *((quint32*)(data + sizeof(quint32)));

    m_payload = QByteArray((data + 2 * sizeof(quint32)), packet.size() - (2 * sizeof(quint32)));
}
