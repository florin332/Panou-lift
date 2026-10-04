// Logic/LiftController.h

#ifndef LIFT_CONTROLLER_H
#define LIFT_CONTROLLER_H

#include "SharedPanel.h"


// Structura propusă de tine pentru rezultatul decizional (Observația 4)
struct ControllerResult {
    bool transmit;
    uint8_t lift;
    bool refreshDisplay;
    bool saveStatistics;
    bool bootCompleted;
    bool logEvent;
};

namespace LiftController
{
    // Inițializează pinul butonului și contextul
    void init();

    // Rerturnează o structură ControllerResult tipizată (Observația 4)
    ControllerResult process(SharedPanel &localPanel);
}

#endif // LIFT_CONTROLLER_H
