#include "UBAgent.h"

#include "config.h"
#include "QsLog.h"

#include "UBNetwork.h"
#include "UBVision.h"

#include "UASManager.h"
#include "LinkManager.h"
#include "ArduPilotMegaMAV.h"
#include "LinkManagerFactory.h"

#include <GeographicLib/Geocentric.hpp>
#include <GeographicLib/LocalCartesian.hpp>

using namespace GeographicLib;

UBAgent::UBAgent(QObject *parent) : QObject(parent),
    m_uav(NULL),
    m_stage(STAGE_START),
    m_loiter_timer(0),
    m_proj(NULL)
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

    LinkManager::instance()->connectLink(link);

    connect(UASManager::instance(), SIGNAL(UASCreated(UASInterface*)), this, SLOT(uavCreateEvent(UASInterface*)));
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
    case STAGE_STOP:
        stageStop();
        break;
    case STAGE_LOITER:
        stageLoiter();
        break;
    }
}

void UBAgent::stageStart() {
    if (!m_uav->isArmed()) {
        return;
    }

    if (pointZone(m_uav->getLatitude(), m_uav->getLongitude(), 0)) {
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

        m_uav->executeCommand(MAV_CMD_NAV_TAKEOFF, 1, 0, 0, 0, 0, 0, 0, TAKEOFF_ALT, 0);
        return;
    }

    if (pointZone(m_uav->getLatitude(), m_uav->getLongitude(), TAKEOFF_ALT)) {
            m_uav->executeCommand(MAV_CMD_NAV_LOITER_TIME, 1, LOITER_TIME, 0, 0, 0, 0, 0, 0, 0);
            m_loiter_timer = QGC::groundTimeSeconds();
            m_stage = STAGE_LOITER;
    }
}

void UBAgent::stageStop() {
    if (pointZone(m_uav->getLatitude(), m_uav->getLongitude(), 0)) {
        if (m_uav->isArmed()) {
            m_uav->executeCommand(MAV_CMD_COMPONENT_ARM_DISARM, 1, 0, 0, 0, 0, 0, 0, 0, 0);
        } else {
            QLOG_INFO() << "Mission is done ...";
            m_stage = STAGE_START;
        }

        return;
    }

    m_uav->executeCommand(MAV_CMD_NAV_LAND, 1, 0, 0, 0, 0, 0, 0, 0, 0);
}

double UBAgent::distance(double lat1, double lon1, double alt1, double lat2, double lon2, double alt2) {
   double x1, x2, y1, y2, z1, z2;

   m_proj->Forward(lat1, lon1, alt1, x1, y1, z1);
   m_proj->Forward(lat2, lon2, alt2, x2, y2, z2);

   return sqrt(pow(x1 - x2, 2) + pow(y1 - y2, 2) + pow(z1 - z2, 2));
}

bool UBAgent::pointZone(double lat, double lon, double alt) {
    double dist = distance(lat, lon, alt, m_uav->getLatitude(), m_uav->getLongitude(), m_uav->getAltitudeRelative());

    if (dist < POINT_ZONE)
        return true;;

    return false;
}

void UBAgent::stageLoiter() {
    if ((QGC::groundTimeSeconds() - m_loiter_timer > LOITER_TIME)) {
        m_uav->executeCommand(MAV_CMD_NAV_LAND, 1, 0, 0, 0, 0, 0, 0, 0, 0);
        m_net->sendData(2, m_msg);
        m_stage = STAGE_STOP;
        return;
    }

    m_net->sendData(2, m_msg);
    m_uav->executeCommand(MAV_CMD_NAV_LOITER_TIME, 1, LOITER_TIME, 0, 0, 0, 0, 0, 0, 0);
}
