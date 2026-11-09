#include "SDCard.h"

#include "Pins.h"

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>

namespace
{
    bool initialized = false;
}

namespace SDCard
{
    bool init()
    {
        if (initialized) return true;

        pinMode(Pins::SD::CS, OUTPUT);
        digitalWrite(Pins::SD::CS, HIGH);
        initialized = SD.begin(Pins::SD::CS, SD_SCK_MHZ(20), SPI);
        return initialized;
    }

    bool isReady()
    {
        return initialized;
    }
}