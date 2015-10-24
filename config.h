#ifndef CONFIG_H
#define CONFIG_H

#define COMM_RANGE 50
#define VISUAL_RANGE 10

#define MAV_PORT 5760
#define PHY_PORT 6760
#define SNR_PORT 7760

#define AGENT_FILE "./agent"
#define FIRMWARE_FILE "./firmware"

#define OBJECTS_PATH "./objects"

#define CONNECT_WAIT 2000

#define ENGINE_TRACK_RATE 1000
#define OBJECT_TRACK_RATE 1000
#define SERVER_TRACK_RATE 1000

#define PACKET_END "\r\r\n\n"
#define BROADCAST_ADDRESS 0

//#define PHY_PORT 52001
#define PHY_TRACK_RATE 1000
#define SNR_TRACK_RATE 1000

#define SERIAL_PORT "ttyACM0"
#define BAUD_RATE 115200

#define GPS_ACCURACY 5
#define ALT_MIN 1
#define ALT_MAX 4
#define LOITER_TIME 20
#define MISSION_START_DELAY 10000
#define MISSION_TRACK_RATE 1000

#endif // CONFIG_H
