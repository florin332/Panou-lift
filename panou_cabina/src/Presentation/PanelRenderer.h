#ifndef PANEL_RENDERER_H
#define PANEL_RENDERER_H

#include "Display.h"  // DisplayTarget + LiftState (via SharedPanel.h)

namespace PanelRenderer {
    // Randare ecran principal pentru un lift pe un target TFT.
    // 'label' trebuie să fie string literal (lifetime permanent).
    void render(DisplayTarget target, const LiftState &lift, const char *label);

    // Invalidare forțată: marchează starea internă pentru redraw complet la următorul render.
    // Folosit când un alt modul (ex: meniu) a șters fizic ecranul prin clearTargetScreen.
    void invalidate(DisplayTarget target);
}

#endif