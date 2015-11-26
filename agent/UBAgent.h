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
    enum EMissionStage {
        STAGE_IDLE,
        STAGE_BEGIN,
        STAGE_MISSION,
        STAGE_END,
    } m_mission_stage;

    struct SMissionData {
        int stage;
        int timer;

        void reset() {
            stage = 0;
            timer = 0;
        }
    } m_mission_data;

private:
    void stageBegin();
    void stageMission();
    void stageEnd();

signals:

protected slots:
    void armedEvent();
    void disarmedEvent();

    void UASCreatedEvent(UASInterface *uav);
    void navModeChangedEvent(int uasID, int mode);

    void startMission();
    void stopMission();

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
