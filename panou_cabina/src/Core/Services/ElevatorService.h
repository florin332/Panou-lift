#ifndef ELEVATOR_SERVICE_H
#define ELEVATOR_SERVICE_H

#include "SharedPanel.h"

namespace ElevatorService
{
    void init();
    void update(SharedPanel &state);

    void commCountEnable();
    void commCountDisable();
    void commCountReset();
    bool commCountIsEnabled();
    void commAnalyzeEnable();
    void commAnalyzeDisable();
}

#endif