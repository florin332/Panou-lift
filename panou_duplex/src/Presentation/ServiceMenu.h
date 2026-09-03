// Presentation/ServiceMenu.h

#ifndef SERVICE_MENU_H
#define SERVICE_MENU_H

#include <cstdint>
#include "ServiceProtocol.h"

namespace ServiceMenu {

void init();
void update();   // Apelează în runCore0()
bool isInServiceMode();
bool isDisplayCommandActive();   // true cât timp un test display e pe ecran

} // namespace ServiceMenu

#endif