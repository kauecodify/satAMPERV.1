#ifndef SECURE_MAVLINK_H
#define SECURE_MAVLINK_H
#include <MAVLink.h>
#include <HardwareSerial.h>

class SecureMAVLink {
public:
    SecureMAVLink(HardwareSerial* serial_port, const char* secret);
    void sendSignedMessage(mavlink_message_t& msg);
    bool receiveSignedMessage(mavlink_message_t& msg);
};
#endif
