#include "PanelRenderer.h"
#include "Display.h"
#include "DisplayGeometry.h"
#include "Config.h"
#include "TextWidget.h"
#include "Arduino.h"
#include <cstdio>

#include "../Fonts/oneslot65.h"
#include "../Fonts/oneslot100.h"
#include "../Fonts/universalis12.h"

namespace {
    enum class PanelLayout {
        None = -1,
        Stop,
        Up,
        Down,
        Defect,
        Revizie,
        NoSerial
    };

    struct PanelScreenState {
        PanelLayout layout = PanelLayout::None;
        TextWidget wFloor;
        TextWidget wEtd;
        TextWidget wLabel;
        bool firstRender = true;
        int lastPos = -1;
        int lastEtd = -1;
        ServiceState lastSvc = ServiceState::Missing;
    };

    constexpr uint16_t C_BLACK   = 0x0000;
    constexpr uint16_t C_GREEN   = 0x07E0;
    constexpr uint16_t C_YELLOW  = 0xFFE0;
    constexpr uint16_t C_MAGENTA = 0xF81D;

    const char* const floorStr[] = {
        "P","1","2","3","4","5","6","7","8","9",
        "10","11","12","13","14","15","16","17"
    };

    inline PanelLayout getLayout(ServiceState svc, Direction sj) {
        switch (svc) {
            case ServiceState::Fault:    return PanelLayout::Defect;
            case ServiceState::Revision: return PanelLayout::Revizie;
            case ServiceState::Missing:  return PanelLayout::NoSerial;
            case ServiceState::Normal:
                switch (sj) {
                    case Direction::Idle: return PanelLayout::Stop;
                    case Direction::Up:   return PanelLayout::Up;
                    case Direction::Down: return PanelLayout::Down;
                    default: return PanelLayout::None;
                }
            default: return PanelLayout::None;
        }
    }

    inline bool isServiceLayout(PanelLayout l) {
        return l == PanelLayout::Defect
            || l == PanelLayout::Revizie
            || l == PanelLayout::NoSerial;
    }

    inline void eraseWidget(DisplayTarget target, TextWidget &widget) {
        if (widget.drawn) {
            Display::drawText(target, widget.x, widget.y, widget.font, widget.text, C_BLACK);
            widget.reset();
        }
    }

    inline void drawTextDirty(DisplayTarget target, TextWidget &widget,
                              const char* text, int16_t x, int16_t y,
                              const GFXfont* font, uint16_t color) {
        if (!widget.isDirty(text, font, x, y, color)) return;
        if (widget.drawn) {
            Display::drawText(target, widget.x, widget.y, widget.font, widget.text, C_BLACK);
        }
        Display::drawText(target, x, y, font, text, color);
        widget.commit(text, font, x, y, color);
    }

    inline void resetAllWidgets(PanelScreenState &st) {
        st.wFloor.reset();
        st.wEtd.reset();
        st.wLabel.reset();
    }

    // Toate pozițiile se calculează din rezoluția ecranului și metricile
    // fonturilor, prin HAL-ul DisplayGeometry — fără coordonate hardcodate.

    // Mișcare: cifra mare ocupă zona din stânga (centrată pe jumătatea stângă),
    // etd în dreapta. Animația sus/jos folosește fracțiuni din înălțime.

    // Procent din înălțimea ecranului → pixel (tipizat, fără -Wnarrowing)
    inline int16_t pctY(int pct) {
        return static_cast<int16_t>((pct * static_cast<int>(DisplayGeom::screenH())) / 100);
    }

    void drawDefect(DisplayTarget target, const char* label, const LiftState &lift) {
        const char* posStr = (lift.pos < Config::Hardware::FLOORS) ? floorStr[lift.pos] : "??";
        const int16_t cx = DisplayGeom::centerX();
        char codeBuf[16];
        snprintf(codeBuf, sizeof(codeBuf), "( cod: 01-%s )", posStr);
        struct { const char* txt; int16_t cy; uint16_t col; } rows[] = {
            { label,            pctY(15), C_YELLOW  },
            { "DEFECT",         pctY(35), C_YELLOW  },
            { codeBuf,          pctY(54), C_MAGENTA },
            { "tel. service",   pctY(72), C_YELLOW  },
            { "0740.317.707",   pctY(84), C_YELLOW  },
        };
        for (const auto &r : rows) {
            Display::drawText(target,
                DisplayGeom::cursorXForCenter(&universalis12, r.txt, cx),
                DisplayGeom::baselineForVCenter(&universalis12, r.txt, r.cy),
                &universalis12, r.txt, r.col);
        }
    }

    void drawRevizie(DisplayTarget target, const char* label) {
        const int16_t cx = DisplayGeom::centerX();
        struct { const char* txt; int16_t cy; } rows[] = {
            { label,          pctY(22) },
            { "REVIZIE",      pctY(49) },
            { "0740.317.707", pctY(86) },
        };
        for (const auto &r : rows) {
            Display::drawText(target,
                DisplayGeom::cursorXForCenter(&universalis12, r.txt, cx),
                DisplayGeom::baselineForVCenter(&universalis12, r.txt, r.cy),
                &universalis12, r.txt, C_YELLOW);
        }
    }

    void drawNoSerial(DisplayTarget target, const char* label) {
        const int16_t cx = DisplayGeom::centerX();
        struct { const char* txt; int16_t cy; uint16_t col; } rows[] = {
            { label,            pctY(15), C_YELLOW  },
            { "folositi",       pctY(30), C_YELLOW  },
            { "comanda de",     pctY(42), C_YELLOW  },
            { "pe usa",         pctY(55), C_YELLOW  },
            { "( cod: 03 )",    pctY(68), C_MAGENTA },
            { "0740.317.707",   pctY(86), C_YELLOW  },
        };
        for (const auto &r : rows) {
            Display::drawText(target,
                DisplayGeom::cursorXForCenter(&universalis12, r.txt, cx),
                DisplayGeom::baselineForVCenter(&universalis12, r.txt, r.cy),
                &universalis12, r.txt, r.col);
        }
    }

    void drawStop(DisplayTarget target, const LiftState &lift, PanelScreenState &st) {
        const char* posStr = (lift.pos < Config::Hardware::FLOORS) ? floorStr[lift.pos] : "??";
        const int16_t xFloor = DisplayGeom::cursorXForCenter(&oneslot100, posStr, DisplayGeom::centerX());
        const int16_t yFloor = DisplayGeom::baselineForVCenter(&oneslot100, posStr, DisplayGeom::centerY());
        drawTextDirty(target, st.wFloor, posStr, xFloor, yFloor, &oneslot100, C_YELLOW);
    }

    // Mișcare: poziția curentă (font mare, verde) și destinația (font mic, magenta),
    // ambele centrate pe axa centrală (x=W/2), una sus și una jos, în funcție de sens.
    //   up:   destinație sus (~31%), poziție curentă jos (~94%)
    //   down: poziție curentă sus, destinație jos (~94%)
    // La coborâre poziția curentă (font mare, 129px) nu poate sta la ~31% — ar ieși
    // din ecran în sus — deci o coborâm la ~44% (baseline 140/320), sub care rămâne
    // loc pentru destinație.
    void drawMovement(DisplayTarget target,
                      const LiftState &lift, PanelScreenState &st,
                      bool movingUp) {
        const char* posStr = (lift.pos < Config::Hardware::FLOORS) ? floorStr[lift.pos] : "??";
        const char* etdStr = (lift.etd < Config::Hardware::FLOORS) ? floorStr[lift.etd] : "??";
        const int16_t cx = DisplayGeom::centerX();

        const int16_t yFloor = movingUp ? pctY(94) : pctY(44);
        const int16_t yEtd   = movingUp ? pctY(31) : pctY(94);

        const int16_t xFloor = DisplayGeom::cursorXForCenter(&oneslot100, posStr, cx);
        const int16_t xEtd   = DisplayGeom::cursorXForCenter(&oneslot65,  etdStr, cx);
        drawTextDirty(target, st.wEtd, etdStr, xEtd, yEtd, &oneslot65, C_MAGENTA);
        drawTextDirty(target, st.wFloor, posStr, xFloor, yFloor, &oneslot100, C_GREEN);
    }

    PanelScreenState stPanel;
}

namespace PanelRenderer {

void invalidate(DisplayTarget target) {
    (void)target;
    stPanel.firstRender = true;
}

void render(DisplayTarget target, const LiftState &lift, const char* label) {
    (void)target;
    PanelScreenState &st = stPanel;
    PanelLayout newLayout = getLayout(lift.svc, lift.sj);

    // --- 1. FIRST RENDER ---
    if (st.firstRender) {
        Display::clearTargetScreen(target);
        st.firstRender = false;
        st.layout = PanelLayout::None;
        resetAllWidgets(st);
        st.lastPos = -1;
        st.lastEtd = -1;
        st.lastSvc = ServiceState::Missing;  // ← CORECTAT
    }

    // --- 2. LAYOUT CHANGE ---
    if (newLayout != st.layout) {
        if (isServiceLayout(newLayout) || isServiceLayout(st.layout)) {
            Display::clearTargetScreen(target);
        } else {
            eraseWidget(target, st.wFloor);
            eraseWidget(target, st.wEtd);
        }
        resetAllWidgets(st);
        st.layout = newLayout;
        st.lastPos = -1;
        st.lastEtd = -1;
        st.lastSvc = ServiceState::Missing;  // ← CORECTAT
    }

    // --- 3. SAME LAYOUT - DIRTY CHECK ---
    if (lift.pos == st.lastPos && lift.etd == st.lastEtd
        && lift.svc == st.lastSvc) {
        return;
    }

    // --- 4. RENDER ---
    switch (newLayout) {
        case PanelLayout::Stop:
            drawStop(target, lift, st);
            break;
        case PanelLayout::Up:
            drawMovement(target, lift, st, true);
            break;
        case PanelLayout::Down:
            drawMovement(target, lift, st, false);
            break;
        case PanelLayout::Defect:
            if (lift.pos != st.lastPos) {
                Display::clearTargetScreen(target);
                resetAllWidgets(st);
                drawDefect(target, label, lift);
            }
            break;
        case PanelLayout::Revizie:
            if (st.lastPos == -1) {
                drawRevizie(target, label);
            }
            break;
        case PanelLayout::NoSerial:
            if (st.lastPos == -1) {
                drawNoSerial(target, label);
            }
            break;
        default:
            break;
    }

    st.lastPos = lift.pos;
    st.lastEtd = lift.etd;
    st.lastSvc = lift.svc;
}

} // namespace PanelRenderer