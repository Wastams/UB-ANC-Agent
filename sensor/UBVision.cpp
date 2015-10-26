#include "UBVision.h"

#include "config.h"
#include "QsLog.h"

#include <QTimer>
#include <QTcpSocket>
#include <QHostAddress>

UBVision::UBVision(QObject *parent) : QObject(parent)
{
    m_socket = new QTcpSocket(this);

    connect(m_socket, SIGNAL(connected()), this, SLOT(connectionEvent()));
    connect(m_socket, SIGNAL(readyRead()), this, SLOT(dataReadyEvent()));

    m_timer = new QTimer(this);
    m_timer->setInterval(SNR_TRACK_RATE);

    connect(m_timer, SIGNAL(timeout()), this, SLOT(sensorTracker()));
}

void UBVision::startSensor(quint16 port) {
    m_socket->connectToHost(QHostAddress::LocalHost, port);
}

void UBVision::dataReadyEvent() {
    m_data += m_socket->readAll();

    while (m_data.contains(PACKET_END)) {
        int bytes = m_data.indexOf(PACKET_END);

        char* data = m_data.left(bytes).data();
        quint8 id = data[0];
        bool visible = data[1];

        if (visible) {
            if (!m_objs.contains(id)) {
                m_objs.append(id);
                emit inVisualRange(id);

                QLOG_INFO() << "Object Detected | ID: " << id;
            }
        }
        else {
            if (m_objs.contains(id)) {
                m_objs.removeAll(id);
                emit outVisualRange(id);

                QLOG_INFO() << "Object Out of Visual Range | ID: " << id;
            }
        }

        m_data = m_data.mid(bytes + qstrlen(PACKET_END));
    }
}

void UBVision::connectionEvent() {
    m_timer->start();
    QLOG_INFO() << "Sensor Connected!";
}

void UBVision::sensorTracker() {
//    if (m_socket->state() != QAbstractSocket::ConnectedState) {
//        m_socket->connectToHost(QHostAddress::LocalHost, m_port);
//    }
}
