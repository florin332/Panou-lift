//Drivers/Leds.h (Interfața Driverului Optic)


#ifndef LEDS_H
#define LEDS_H

#include "SharedPanel.h"

namespace Leds
{
    void init();
    void update(const SharedPanel &localPanel);
}

#endif // LEDS_H
