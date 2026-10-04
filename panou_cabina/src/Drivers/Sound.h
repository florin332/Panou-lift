//Drivers/Sound.h (Interfața Driverului Acustic)

#ifndef SOUND_H
#define SOUND_H

#include "SharedPanel.h"

namespace Sound
{
    void init();
    void update(const SharedPanel &localPanel);
}

#endif // SOUND_H
