#ifndef ALARM_SERVICE_H
#define ALARM_SERVICE_H

#include "SharedPanel.h"

namespace AlarmService
{
    void init();
    void initializeBootState(SharedPanel &state);
    void update(const SharedPanel &state, Direction previousDirection);

    bool isPlaying();
    void stop();
    bool setGain(float gain);
    float gain();
}

#endif