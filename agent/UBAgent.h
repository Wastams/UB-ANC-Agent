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

private:
    enum mission_stage {
        STAGE_START,
        STAGE_LOITER,
        STAGE_STOP
    } m_stage;

private:
    ArduPilotMegaMAV* m_uav;
    int m_loiter_timer;

private:
    void stageStart();
    void stageStop();
    void stageLoiter();

signals:

protected slots:
    void newUAVEvent(UASInterface *uav);

    void missionTracker();

private:
    QTimer* m_timer;

    UBNetwork* m_net;
    UBVision* m_vision;

    QByteArray m_msg;
};

#endif // UBAGENT_H
