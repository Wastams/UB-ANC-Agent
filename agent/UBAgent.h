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
        STAGE_IDLE,
        STAGE_START,
        STAGE_MISSION,
        STAGE_STOP,
    } m_stage;

private:
    void stageStart();
    void stageMission();
    void stageStop();

signals:

protected slots:
    void armedEvent();
    void uavCreateEvent(UASInterface *uav);
    void heartbeatTimeoutEvent(bool timeout, uint ms);

    void startMission();
    void missionTracker();

protected:
    QTimer* m_timer;
    ArduPilotMegaMAV* m_uav;

    UBNetwork* m_net;
    UBVision* m_sensor;

protected:
    double distance(double lat1, double lon1, double alt1, double lat2, double lon2, double alt2);
    bool inPointZone(double lat, double lon, double alt);
};

#endif // UBAGENT_H
