#include "PanelRenderer.h"
#include "Display.h"
#include "Config.h"
#include "TextWidget.h"
#include "Arduino.h"
#include <cstdio>

#include "../Fonts/oneslot30.h"
#include "../Fonts/oneslot65.h"
#include "../Fonts/arrows40.h"
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
        TextWidget wArrow;
        TextWidget wEtd;
        TextWidget wLabel;
        bool firstRender = true;
        int lastPos = -1;
        int lastEtd = -1;
        int lastOcp = -1;
        ServiceState lastSvc = ServiceState::Missing;
    };

    constexpr uint16_t C_BLACK   = 0x0000;
    constexpr uint16_t C_GREEN   = 0x07E0;
    constexpr uint16_t C_RED     = 0xF800;
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
        st.wArrow.reset();
        st.wEtd.reset();
        st.wLabel.reset();
    }

    struct LayoutCoords {
        int16_t arrowStopX, arrowStopY;
        int16_t xLargeSingle, xLargeDouble;
        int16_t xSmallSingle, xSmallDouble;
        int16_t arrowEtdX, etdEtjOffset;
        const char* arrowChar;
    };

    constexpr LayoutCoords CFG_LEFT = {
        35, 53, 37, 5, 50, 35, 68, -30, "a"
    };

    constexpr LayoutCoords CFG_RIGHT = {
        33, 52, 36, 5, 50, 35, 2, 30, "b"
    };

    void drawDefect(DisplayTarget target, const char* label, const LiftState &lift) {
        const char* posStr = (lift.pos < Config::Hardware::FLOORS) ? floorStr[lift.pos] : "??";
        Display::drawText(target, 8, 25, &universalis12, label, C_YELLOW);
        Display::drawText(target, 29, 56, &universalis12, "DEFECT", C_YELLOW);
        char codeBuf[16];
        snprintf(codeBuf, sizeof(codeBuf), "( cod: 01-%s )", posStr);
        Display::drawText(target, 2, 82, &universalis12, codeBuf, C_MAGENTA);
        Display::drawText(target, 15, 105, &universalis12, "tel. service", C_YELLOW);
        Display::drawText(target, 3, 130, &universalis12, "0740.317.707", C_YELLOW);
        Display::drawText(target, 2, 155, &universalis12, "0740.317.707", C_YELLOW);
    }

    void drawRevizie(DisplayTarget target, const char* label) {
        Display::drawText(target, 10, 40, &universalis12, label, C_YELLOW);
        Display::drawText(target, 28, 85, &universalis12, "REVIZIE", C_YELLOW);
        Display::drawText(target, 3, 155, &universalis12, "0740.317.707", C_YELLOW);
    }

    void drawNoSerial(DisplayTarget target, const char* label) {
        Display::drawText(target, 8, 25, &universalis12, label, C_YELLOW);
        Display::drawText(target, 32, 54, &universalis12, "folositi", C_YELLOW);
        Display::drawText(target, 6, 75, &universalis12, "comanda de", C_YELLOW);
        Display::drawText(target, 27, 96, &universalis12, "pe usa", C_YELLOW);
        Display::drawText(target, 16, 123, &universalis12, "( cod: 03 )", C_MAGENTA);
        Display::drawText(target, 2, 155, &universalis12, "0740.317.707", C_YELLOW);
    }

    void drawStop(DisplayTarget target, const LayoutCoords &cfg,
                  const LiftState &lift, PanelScreenState &st) {
        const char* posStr = (lift.pos < Config::Hardware::FLOORS) ? floorStr[lift.pos] : "??";
        const int16_t xFloor = (lift.pos <= 9) ? cfg.xLargeSingle : cfg.xLargeDouble;
        const uint16_t numColor = (lift.ocp == Occupancy::Free) ? C_GREEN : C_RED;
        const bool atPanelFloor = (lift.pos == Config::Hardware::PANEL_FLOOR);

        if (atPanelFloor) {
            drawTextDirty(target, st.wArrow, cfg.arrowChar,
                          cfg.arrowStopX, cfg.arrowStopY, &arrows40, C_YELLOW);
            drawTextDirty(target, st.wFloor, posStr, xFloor, 147, &oneslot65, numColor);
        } else {
            eraseWidget(target, st.wArrow);
            drawTextDirty(target, st.wFloor, posStr, xFloor, 122, &oneslot65, numColor);
        }
    }

    void drawMovement(DisplayTarget target, const LayoutCoords &cfg,
                      const LiftState &lift, PanelScreenState &st,
                      int16_t yEtd, int16_t yFloor) {
        const char* posStr = (lift.pos < Config::Hardware::FLOORS) ? floorStr[lift.pos] : "??";
        const char* etdStr = (lift.etd < Config::Hardware::FLOORS) ? floorStr[lift.etd] : "??";
        const int16_t xFloor = (lift.pos <= 9) ? cfg.xLargeSingle : cfg.xLargeDouble;
        const int16_t xEtd   = (lift.etd <= 9) ? cfg.xSmallSingle : cfg.xSmallDouble;
        const uint16_t numColor = (lift.ocp == Occupancy::Free) ? C_GREEN : C_RED;
        const bool etdAtPanel = (lift.etd == Config::Hardware::PANEL_FLOOR);

        if (etdAtPanel) {
            drawTextDirty(target, st.wArrow, cfg.arrowChar,
                          cfg.arrowEtdX, yEtd + 10, &arrows40, C_YELLOW);
            drawTextDirty(target, st.wEtd, etdStr,
                          xEtd + cfg.etdEtjOffset, yEtd, &oneslot30, C_YELLOW);
        } else {
            eraseWidget(target, st.wArrow);
            drawTextDirty(target, st.wEtd, etdStr, xEtd, yEtd, &oneslot30, C_MAGENTA);
        }
        drawTextDirty(target, st.wFloor, posStr, xFloor, yFloor, &oneslot65, numColor);
    }

    PanelScreenState stLeft;
    PanelScreenState stRight;
}

namespace PanelRenderer {

void invalidate(DisplayTarget target) {
    PanelScreenState &st = (target == DisplayTarget::Left) ? stLeft : stRight;
    st.firstRender = true;
}

void render(DisplayTarget target, const LiftState &lift, const char* label) {
    PanelScreenState &st = (target == DisplayTarget::Left) ? stLeft : stRight;
    const LayoutCoords &cfg = (target == DisplayTarget::Left) ? CFG_LEFT : CFG_RIGHT;
    PanelLayout newLayout = getLayout(lift.svc, lift.sj);

    // --- 1. FIRST RENDER ---
    if (st.firstRender) {
        Display::clearTargetScreen(target);
        st.firstRender = false;
        st.layout = PanelLayout::None;
        resetAllWidgets(st);
        st.lastPos = -1;
        st.lastEtd = -1;
        st.lastOcp = -1;
        st.lastSvc = ServiceState::Missing;  // ← CORECTAT
    }

    // --- 2. LAYOUT CHANGE ---
    if (newLayout != st.layout) {
        if (isServiceLayout(newLayout) || isServiceLayout(st.layout)) {
            Display::clearTargetScreen(target);
        } else {
            eraseWidget(target, st.wFloor);
            eraseWidget(target, st.wArrow);
            eraseWidget(target, st.wEtd);
        }
        resetAllWidgets(st);
        st.layout = newLayout;
        st.lastPos = -1;
        st.lastEtd = -1;
        st.lastOcp = -1;
        st.lastSvc = ServiceState::Missing;  // ← CORECTAT
    }

    // --- 3. SAME LAYOUT - DIRTY CHECK ---
    if (lift.pos == st.lastPos && lift.etd == st.lastEtd
        && static_cast<int>(lift.ocp) == st.lastOcp && lift.svc == st.lastSvc) {
        return;
    }

    // --- 4. RENDER ---
    switch (newLayout) {
        case PanelLayout::Stop:
            drawStop(target, cfg, lift, st);
            break;
        case PanelLayout::Up:
            drawMovement(target, cfg, lift, st, 50, 155);
            break;
        case PanelLayout::Down:
            drawMovement(target, cfg, lift, st, 150, 90);
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
    st.lastOcp = static_cast<int>(lift.ocp);
    st.lastSvc = lift.svc;
}

} // namespace PanelRenderer