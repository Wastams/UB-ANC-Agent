#include "UBNetwork.h"
#include "UBPacket.h"

#include "config.h"
#include "QsLog.h"

#include <QTimer>
#include <QTcpSocket>
#include <QCoreApplication>

UBNetwork::UBNetwork(QObject *parent) : QObject(parent),
    m_size(0),
    m_id(0)
{
    m_port = PHY_PORT;

    int idx = QCoreApplication::arguments().indexOf("--port");
    if (idx > 0)
        m_port = (PHY_PORT - MAV_PORT) + QCoreApplication::arguments().at(idx + 1).toInt();

    m_socket = new QTcpSocket(this);

    connect(m_socket, SIGNAL(bytesWritten(qint64)), this, SLOT(dataSentEvent(qint64)));
    connect(m_socket, SIGNAL(connected()), this, SLOT(connectionEvent()));
    connect(m_socket, SIGNAL(disconnected()), this, SLOT(disconnectEvent()));
    connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(errorEvent(QAbstractSocket::SocketError)));
    connect(m_socket, SIGNAL(readyRead()), this, SLOT(dataReadyEvent()));

    m_timer = new QTimer(this);
    m_timer->setInterval(PHY_TRACK_RATE);

    connect(m_timer, SIGNAL(timeout()), this, SLOT(phyTracker()));

    m_timer->start();
}

void UBNetwork::sendData(quint8 desID, const QByteArray& data) {
    UBPacket packet;

    packet.setSrcID(m_id);
    packet.setDesID(desID);
    packet.setPayload(data);

    QByteArray* stream = new QByteArray(packet.packetize());

    m_send_buffer.enqueue(stream);
}

QByteArray UBNetwork::getData() {
    QByteArray data;

    if (!m_receive_buffer.isEmpty()) {
        QByteArray* stream = m_receive_buffer.dequeue();

        data = *stream;
        delete stream;
    }

    return data;
}

void UBNetwork::dataSentEvent(qint64 size) {
    m_size -= size;

    if (!m_size) {
        QByteArray* stream = m_send_buffer.dequeue();

        UBPacket packet;
        packet.depacketize(*stream);

        QLOG_INFO() << "Packet Sent | From " << packet.getSrcID() << " to " << packet.getDesID() << " | Size: " << packet.getPayload().size();

        m_size = 0;
        delete stream;
    }
}

void UBNetwork::dataReadyEvent() {
    m_data += m_socket->readAll();

    while (m_data.contains(PACKET_END)) {
        int bytes = m_data.indexOf(PACKET_END);

        UBPacket packet;
        packet.depacketize(m_data.left(bytes));

        if (packet.getDesID() == m_id || packet.getDesID() == BROADCAST_ADDRESS) {
            QByteArray* data = new QByteArray(packet.getPayload());

            m_receive_buffer.enqueue(data);
            emit dataReady();

            QLOG_INFO() << "Packet Received | From " << packet.getSrcID() << " to " << packet.getDesID() << " | Size: " << packet.getPayload().size();
        }

        m_data = m_data.mid(bytes + qstrlen(PACKET_END));
    }
}

void UBNetwork::connectionEvent() {
    QLOG_DEBUG() << "PHY Connected!";
}

void UBNetwork::disconnectEvent() {
    QLOG_DEBUG() << "PHY Disconnected!";
}

void UBNetwork::errorEvent(QAbstractSocket::SocketError) {
   QLOG_DEBUG() << "PHY ERROR: " << m_socket->errorString();
}

void UBNetwork::phyTracker() {
    if (m_socket->state() != QAbstractSocket::ConnectedState) {
        m_socket->connectToHost(QHostAddress::LocalHost, m_port);
    } else if (!m_send_buffer.isEmpty() && !m_size) {
        QByteArray data(*m_send_buffer.first());
        data.append(PACKET_END);
        m_size = data.size();

        m_socket->write(data);
    }
}
