#ifndef UBNETWORK_H
#define UBNETWORK_H

#include <QQueue>
#include <QObject>
#include <QByteArray>

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
    void startNetwork(quint8 id, quint16 port);
    void sendData(quint8 desID, const QByteArray& data);

protected slots:
    void connectionEvent();
    void dataReadyEvent();
    void dataSentEvent(qint64);

    void phyTracker();

private:
    quint8 m_id;

    QTcpSocket* m_socket;

    qint64 m_size;
    QByteArray m_data;

    QTimer* m_timer;

    QQueue<QByteArray*> m_send_buffer;
    QQueue<QByteArray*> m_receive_buffer;
};

#endif // UBNETWORK_H
