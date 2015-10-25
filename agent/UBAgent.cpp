#include "UBAgent.h"

#include "config.h"
#include "QsLog.h"

#include "UBNetwork.h"
#include "UBVision.h"

#include "UASManager.h"
#include "LinkManager.h"
#include "ArduPilotMegaMAV.h"
#include "LinkManagerFactory.h"

UBAgent::UBAgent(QObject *parent) : QObject(parent),
    m_uav(NULL),
    m_stage(STAGE_START),
    m_loiter_timer(0)
{
    m_msg.append(MAV_CMD_NAV_TAKEOFF);

    m_net = new UBNetwork(this);
    m_sensor = new UBVision(this);

    m_timer = new QTimer(this);
    m_timer->setInterval(MISSION_TRACK_RATE);

    connect(m_timer, SIGNAL(timeout()), this, SLOT(missionTracker()));
}

void UBAgent::startAgent() {
    int port = MAV_PORT;

    int idx = QCoreApplication::arguments().indexOf("--port");
    if (idx > 0)
        port = QCoreApplication::arguments().at(idx + 1).toInt();

    int link = 0;

//    link = LinkManagerFactory::addSerialConnection(SERIAL_PORT, BAUD_RATE);
    link = LinkManagerFactory::addTcpConnection(QHostAddress::LocalHost, "", port, false);
    if (!link) {
        QLOG_FATAL() << "Agent was unable to connect to APM!";
        return;
    }

    LinkManager::instance()->connectLink(link);

    connect(UASManager::instance(), SIGNAL(UASCreated(UASInterface*)), this, SLOT(uavCreateEvent(UASInterface*)));
    connect(UASManager::instance(), SIGNAL(UASDeleted(UASInterface*)), this, SLOT(uavDeleteEvent(UASInterface*)));
}

void UBAgent::uavCreateEvent(UASInterface* uav) {
    if (m_uav)
        return;

    m_uav = dynamic_cast<ArduPilotMegaMAV*>(uav);

    if (!m_uav)
        return;

    int port = MAV_PORT;

    int idx = QCoreApplication::arguments().indexOf("--port");
    if (idx > 0)
        port = QCoreApplication::arguments().at(idx + 1).toInt();

    m_net->startNetwork(m_uav->getUASID(), (PHY_PORT - MAV_PORT) + port);
    m_sensor->startSensor((SNR_PORT - MAV_PORT) + port);

    m_uav->setHeartbeatEnabled(true);
    connect(m_uav, SIGNAL(heartbeatTimeout(bool,uint)), this, SLOT(heartbeatTimeoutEvent(bool,uint)));

//    QTimer::singleShot(START_DELAY, m_trackTimer, SLOT(start()));

    m_timer->start();
}

void UBAgent::uavDeleteEvent(UASInterface* uav) {
    if (uav != m_uav)
        return;

    m_timer->stop();

    m_net->stopNetwork();
    m_sensor->stopSensor();

    disconnect(m_uav, SIGNAL(heartbeatTimeout(bool,uint)), this, SLOT(heartbeatTimeoutEvent(bool,uint)));
    m_uav = NULL;
}

void UBAgent::heartbeatTimeoutEvent(bool timeout, uint ms) {
    if (timeout)
        QLOG_WARN() << "UAV connection lost! Millisecond: " << ms;
    else
        QLOG_INFO() << "UAV reconnected!";
}

void UBAgent::missionTracker() {
//    if (m_start_time < START_DELAY) {
//        m_start_time++;
//        return;
//    }

    if (!m_uav)
        return;

    switch (m_stage) {
    case STAGE_START:
        stageStart();
        break;
    case STAGE_LOITER:
        stageLoiter();
        break;
    case STAGE_STOP:
        stageStop();
        break;
    }
}

void UBAgent::stageStart() {
    if (!m_uav->isArmed()) {
        return;
    }

    if (m_uav->getAltitudeRelative() < ALT_MIN) {

        if (m_uav->getSatelliteCount() < GPS_ACCURACY)
            return;

        if (m_uav->getCustomMode() != ApmCopter::GUIDED) {
            m_uav->setMode(MAV_MODE_FLAG_CUSTOM_MODE_ENABLED, ApmCopter::GUIDED);
            return;
        }

//        if (!m_uav->isArmed()) {
//            m_uav->executeCommand(MAV_CMD_COMPONENT_ARM_DISARM, 1, 1, 0, 0, 0, 0, 0, 0, 0);
//            return;
//        }

        m_uav->executeCommand(MAV_CMD_NAV_TAKEOFF, 1, 0, 0, 0, 0, 0, 0, ALT_MAX, 0);
        return;
    }

    if (m_uav->getAltitudeRelative() > ALT_MAX - ALT_MIN) {
            m_uav->executeCommand(MAV_CMD_NAV_LOITER_TIME, 1, LOITER_TIME, 0, 0, 0, 0, 0, 0, 0);
            m_loiter_timer = QGC::groundTimeSeconds();
            m_stage = STAGE_LOITER;
    }
}

void UBAgent::stageLoiter() {
    if ((QGC::groundTimeSeconds() - m_loiter_timer > LOITER_TIME)) {
        m_uav->executeCommand(MAV_CMD_NAV_LAND, 1, 0, 0, 0, 0, 0, 0, 0, 0);
        m_net->sendData(2, m_msg);
        m_stage = STAGE_STOP;
        return;
    }

    if (m_uav->getAltitudeRelative() > ALT_MAX + ALT_MIN) {
        m_uav->executeCommand(MAV_CMD_NAV_LOITER_TIME, 1, LOITER_TIME, 0, 0, 0, 0, 0, 0, 0);
    }
}

void UBAgent::stageStop() {
    m_net->sendData(2, m_msg);

    if (m_uav->getAltitudeRelative() > ALT_MAX - ALT_MIN) {
        m_uav->executeCommand(MAV_CMD_NAV_LAND, 1, 0, 0, 0, 0, 0, 0, 0, 0);
        return;
    }

    if (m_uav->getAltitudeRelative() < ALT_MIN) {
        if (m_uav->isArmed()) {
            m_uav->executeCommand(MAV_CMD_COMPONENT_ARM_DISARM, 1, 0, 0, 0, 0, 0, 0, 0, 0);
            return;
        } else {
            QLOG_INFO() << "Mission is done ...";

            m_stage = STAGE_START;
        }
    }
}
