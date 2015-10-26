#ifndef UBVISION_H
#define UBVISION_H

#include <QList>
#include <QObject>
#include <QByteArray>

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
    void startSensor(quint16 port);

protected slots:
    void connectionEvent();
    void dataReadyEvent();

    void sensorTracker();

private:
    QTimer* m_timer;
    QByteArray m_data;
    QList<quint8> m_objs;
    QTcpSocket* m_socket;
};

#endif // UBVISION_H
