//Drivers/Presentation.h

#ifndef PRESENTATION_H
#define PRESENTATION_H

#include "SharedPanel.h"

namespace Presentation
{
    void init();
    bool update(const SharedPanel &snapshot);
}

#endif // PRESENTATION_H
