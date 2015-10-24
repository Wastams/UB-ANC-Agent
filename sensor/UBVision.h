#ifndef UBVISION_H
#define UBVISION_H

#include <QHostAddress>
#include <QByteArray>
#include <QList>

class QTimer;
class QTcpSocket;

class UBVision : public QObject
{
    Q_OBJECT
public:
    explicit UBVision(QObject *parent = 0);

private:

signals:
    void inVisualRange(quint8);
    void outVisualRange(quint8);

public slots:

protected slots:
    void connectionEvent();
    void disconnectEvent();
    void dataReadyEvent();
    void errorEvent(QAbstractSocket::SocketError);

    void sensorTracker();

private:
    quint16 m_port;

    QTimer* m_timer;
    QByteArray m_data;
    QList<quint8> m_objs;
    QTcpSocket* m_socket;
};

#endif // UBVISION_H
