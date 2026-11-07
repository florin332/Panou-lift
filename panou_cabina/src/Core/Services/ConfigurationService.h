#ifndef CONFIGURATION_SERVICE_H
#define CONFIGURATION_SERVICE_H

#include "SharedPanel.h"
#include "Settings.h"
#include <cstdint>

namespace ConfigurationService
{
    void init();
    void initializeStartupState(SharedPanel &state);

    uint16_t liftNumber();
    bool setLiftNumber(uint16_t number);
    float audioGain();
    bool resetDefaults();
    Settings::NetworkSettings networkSettings();
    bool setNetworkSettings(const Settings::NetworkSettings &settings);
}

#endif