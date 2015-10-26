#ifndef UBAGENT_H
#define UBAGENT_H

#include <QObject>

class QTimer;

class UASInterface;
class ArduPilotMegaMAV;

class UBNetwork;
class UBVision;

class UBAgent : public QObject
{
    Q_OBJECT
public:
    explicit UBAgent(QObject *parent = 0);

public slots:
    void startAgent();

private:
    enum mission_stage {
        STAGE_START,
        STAGE_LOITER,
        STAGE_STOP
    } m_stage;

private:
    void stageStart();
    void stageStop();
    void stageLoiter();

signals:

protected slots:
    void uavCreateEvent(UASInterface *uav);
    void heartbeatTimeoutEvent(bool timeout, uint ms);

    void missionTracker();

protected:
    int m_loiter_timer;

    QTimer* m_timer;

    UBNetwork* m_net;
    UBVision* m_sensor;

    QByteArray m_msg;

    ArduPilotMegaMAV* m_uav;
};

#endif // UBAGENT_H
