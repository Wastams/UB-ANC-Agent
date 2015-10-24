#ifndef UBNETWORK_H
#define UBNETWORK_H

#include <QHostAddress>
#include <QByteArray>
#include <QQueue>

class QTimer;
class QTcpSocket;

class UBNetwork : public QObject
{
    Q_OBJECT
public:
    explicit UBNetwork(QObject *parent = 0);

    QByteArray getData();

private:

signals:
    void dataReady();

public slots:
    void setSysID(quint8 id) {m_id = id;}
    void sendData(quint8 desID, const QByteArray& data);

protected slots:
    void connectionEvent();
    void disconnectEvent();
    void dataSentEvent(qint64);
    void dataReadyEvent();
    void errorEvent(QAbstractSocket::SocketError);

    void phyTracker();

private:
    quint8 m_id;

    quint16 m_port;
    QTcpSocket* m_socket;

    qint64 m_size;
    QByteArray m_data;

    QTimer* m_timer;

    QQueue<QByteArray*> m_send_buffer;
    QQueue<QByteArray*> m_receive_buffer;
};

#endif // UBNETWORK_H
