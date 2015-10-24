#include "UBVision.h"

#include "config.h"
#include "QsLog.h"

#include <QTimer>
#include <QTcpSocket>
#include <QCoreApplication>

UBVision::UBVision(QObject *parent) : QObject(parent)
{
    m_port = SNR_PORT;

    int idx = QCoreApplication::arguments().indexOf("--port");
    if (idx > 0)
        m_port = (SNR_PORT - MAV_PORT) + QCoreApplication::arguments().at(idx + 1).toInt();

    m_socket = new QTcpSocket(this);

    connect(m_socket, SIGNAL(connected()), this, SLOT(connectionEvent()));
    connect(m_socket, SIGNAL(disconnected()), this, SLOT(disconnectEvent()));
    connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(errorEvent(QAbstractSocket::SocketError)));
    connect(m_socket, SIGNAL(readyRead()), this, SLOT(dataReadyEvent()));

    m_timer = new QTimer(this);
    m_timer->setInterval(SNR_TRACK_RATE);

    connect(m_timer, SIGNAL(timeout()), this, SLOT(sensorTracker()));

    m_timer->start();
}

void UBVision::dataReadyEvent() {
    m_data += m_socket->readAll();

    while (m_data.contains(PACKET_END)) {
        int bytes = m_data.indexOf(PACKET_END);

        char* data = m_data.left(bytes).data();
        if (data[1]) {
            if (!m_objs.contains(data[0])) {
                m_objs.append(data[0]);
                emit inVisualRange(data[0]);

                QLOG_INFO() << "Object Detected | ID: " << data[0];
            }
        }
        else {
            if (m_objs.contains(data[0])) {
                m_objs.removeAll(data[0]);
                emit outVisualRange(data[0]);

                QLOG_INFO() << "Object Out of Visual Range | ID: " << data[0];
            }
        }

        m_data = m_data.mid(bytes + qstrlen(PACKET_END));
    }
}

void UBVision::connectionEvent() {
    QLOG_DEBUG() << "Sensor Connected!";
}

void UBVision::disconnectEvent() {
    QLOG_DEBUG() << "Sensor Disconnected!";
}

void UBVision::errorEvent(QAbstractSocket::SocketError) {
   QLOG_DEBUG() << "Sensor ERROR: " << m_socket->errorString();
}

void UBVision::sensorTracker() {
    if (m_socket->state() != QAbstractSocket::ConnectedState) {
        m_socket->connectToHost(QHostAddress::LocalHost, m_port);
    }
}
