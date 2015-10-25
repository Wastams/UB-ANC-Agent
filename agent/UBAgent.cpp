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
    int port = MAV_PORT;

    int idx = QCoreApplication::arguments().indexOf("--port");
    if (idx > 0)
        port = QCoreApplication::arguments().at(idx + 1).toInt();

    int link = 0;

//    link = LinkManagerFactory::addSerialConnection(SERIAL_PORT, BAUD_RATE);
    link = LinkManagerFactory::addTcpConnection(QHostAddress::LocalHost, "", port, false);
    if (!link)
        return;

    LinkManager::instance()->connectLink(link);

    connect(UASManager::instance(), SIGNAL(UASCreated(UASInterface*)), this, SLOT(newUAVEvent(UASInterface*)));

    m_net = new UBNetwork(this);
    m_msg.append(MAV_CMD_NAV_TAKEOFF);

    m_vision = new UBVision(this);

    m_timer = new QTimer(this);
    m_timer->setInterval(MISSION_TRACK_RATE);

    connect(m_timer, SIGNAL(timeout()), this, SLOT(missionTracker()));
}

//void UBAgent::handleNewLink(int linkid) {
//    if(LinkManager::instance()->getLinkType(linkid) != LinkInterface::SERIAL_LINK)
//        return;

//    SerialLinkInterface* link = static_cast<SerialLinkInterface*>(LinkManager::instance()->getLink(linkid));

//    link->setPortName("ttyACM0");
//    link->setBaudRate(BAUD_RATE);
//    link->connect();

//    connect(UASManager::instance(), SIGNAL(UASCreated(UASInterface*)), this, SLOT(handleNewUAV(UASInterface*)), Qt::UniqueConnection);
//}

void UBAgent::newUAVEvent(UASInterface* uav) {
    m_uav = dynamic_cast<ArduPilotMegaMAV*>(uav);

    if (!m_uav)
        return;

//    m_uav->setHeartbeatEnabled(true);
//    connect(m_uav, SIGNAL(heartbeatTimeout(bool, uint)), this, SLOT(handleHeartbeatTimeout(bool, uint)));

    m_net->setSysID(m_uav->getUASID());

//    QTimer::singleShot(START_DELAY, m_trackTimer, SLOT(start()));

    m_timer->start();
}

void UBAgent::missionTracker() {
//    if (m_start_time < START_DELAY) {
//        m_start_time++;
//        return;
//    }

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
