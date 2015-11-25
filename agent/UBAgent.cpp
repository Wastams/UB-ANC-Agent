#include "UBAgent.h"

#include "config.h"
#include "QsLog.h"

#include "UBNetwork.h"
#include "UBVision.h"

#include "UASManager.h"
#include "LinkManager.h"
#include "ArduPilotMegaMAV.h"
#include "LinkManagerFactory.h"

#include "mercatorprojection.h"

UBAgent::UBAgent(QObject *parent) : QObject(parent),
    m_uav(NULL),
    m_stage(STAGE_IDLE)
{
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

    m_uav = qobject_cast<ArduPilotMegaMAV*>(uav);

    if (!m_uav)
        return;

    int port = MAV_PORT;

    int idx = QCoreApplication::arguments().indexOf("--port");
    if (idx > 0)
        port = QCoreApplication::arguments().at(idx + 1).toInt();

    m_net->startNetwork(m_uav->getUASID(), (PHY_PORT - MAV_PORT) + port);
    m_sensor->startSensor((SNR_PORT - MAV_PORT) + port);

    m_uav->setHeartbeatEnabled(true);
    m_uav->getWaypointManager()->readWaypoints(true);
    m_uav->setMode(MAV_MODE_FLAG_CUSTOM_MODE_ENABLED, ApmCopter::GUIDED);

    connect(m_uav, SIGNAL(armed()), this, SLOT(armedEvent()));
    connect(m_uav, SIGNAL(heartbeatTimeout(bool,uint)), this, SLOT(heartbeatTimeoutEvent(bool,uint)));

//    QTimer::singleShot(START_DELAY, this, SLOT(startMission()));

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
    case STAGE_MISSION:
        stageMission();
        break;
    case STAGE_STOP:
        stageStop();
        break;
    default:
        break;
    }
}

void UBAgent::startMission() {
    if (m_uav->getSatelliteCount() < GPS_ACCURACY)
        return;

    if (m_uav->getCustomMode() != ApmCopter::GUIDED)
        m_uav->setMode(MAV_MODE_FLAG_CUSTOM_MODE_ENABLED, ApmCopter::GUIDED);

    m_uav->executeCommand(MAV_CMD_NAV_TAKEOFF, 1, 0, 0, 0, 0, 0, 0, TAKEOFF_ALT, 0);

    m_stage = STAGE_START;
}

void UBAgent::armedEvent() {
    startMission();
}

void UBAgent::heartbeatTimeoutEvent(bool timeout, uint ms) {
    if (timeout)
        QLOG_WARN() << "UAV connection lost! Millisecond: " << ms;
    else
        QLOG_INFO() << "UAV reconnected!";
}

void UBAgent::stageStart() {
    if (inPointZone(m_uav->getLatitude(), m_uav->getLongitude(), TAKEOFF_ALT))
        m_stage = STAGE_MISSION;
}

void UBAgent::stageStop() {
    if (inPointZone(m_uav->getLatitude(), m_uav->getLongitude(), 0)) {
        m_stage = STAGE_IDLE;

        QLOG_INFO() << "Mission is done ...";
    }
}

double UBAgent::distance(double lat1, double lon1, double alt1, double lat2, double lon2, double alt2) {
   double x1, y1, z1;
   double x2, y2, z2;

   projections::MercatorProjection proj;

   proj.FromGeodeticToCartesian(lat1, lon1, alt1, x1, y1, z1);
   proj.FromGeodeticToCartesian(lat2, lon2, alt2, x2, y2, z2);

   return sqrt(pow(x1 - x2, 2) + pow(y1 - y2, 2) + pow(z1 - z2, 2));
}

bool UBAgent::inPointZone(double lat, double lon, double alt) {
    double dist = distance(m_uav->getLatitude(), m_uav->getLongitude(), m_uav->getAltitudeRelative(), lat, lon, alt);

    if (dist < POINT_ZONE)
        return true;

    return false;
}

void UBAgent::stageMission() {
    static int stage = 0;
    static int timer = 0;
    static double lat, lon;

    if (stage == 0) {
        stage++;

        double x, y, z;

        projections::MercatorProjection proj;
        proj.FromGeodeticToCartesian(m_uav->getLatitude(), m_uav->getLongitude(), m_uav->getAltitudeRelative(), x, y, z);
        proj.FromCartesianTGeodetic(x + 3, y, z, lat, lon);

        Waypoint wp;
        wp.setFrame(MAV_FRAME_GLOBAL_RELATIVE_ALT);
    //    wp.setAction(MAV_CMD_NAV_WAYPOINT);
    //    wp.setParam1(0);
    //    wp.setParam2(POINT_ZONE);
    //    wp.setParam3(POINT_ZONE);
    //    wp.setParam4(0);
    //    wp.setParam5(m_uav->getLatitude());
    //    wp.setParam6(m_uav->getLongitude());
    //    wp.setParam7(TAKEOFF_ALT);
        wp.setLatitude(lat);
        wp.setLongitude(lon);
        wp.setAltitude(m_uav->getAltitudeRelative());

        m_uav->getWaypointManager()->goToWaypoint(&wp);

        return;
    }

    if (stage == 1) {
        if (inPointZone(lat, lon, m_uav->getAltitudeRelative())) {
            stage++;
        }

        return;
    }

    if (timer > 20) {
        m_uav->land();

        stage = 0;
        timer = 0;

        m_stage = STAGE_STOP;
    } else {
        timer++;

        m_net->sendData(2, QByteArray(1, MAV_CMD_NAV_TAKEOFF));
    }
}
