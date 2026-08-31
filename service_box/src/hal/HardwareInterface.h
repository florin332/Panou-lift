#ifndef HARDWARE_INTERFACE_H
#define HARDWARE_INTERFACE_H

#include <Arduino.h>

class HardwareInterface {
public:
    virtual void init() = 0;
    virtual void updateTouch() = 0;
    virtual bool isScreenTouched() = 0;
    virtual int getTouchX() = 0;
    virtual int getTouchY() = 0;
    virtual void sendProtocolData(uint8_t* data, uint16_t len) = 0;
};

// Declarație pentru instanța globală care va fi apelată în main
extern HardwareInterface& Hardware;

#endif
