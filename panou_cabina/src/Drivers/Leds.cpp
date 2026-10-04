//Drivers/Leds.cpp (Implementarea Driverului Optic)

#include "Leds.h"
#include "Pins.h"
#include "Config.h"
#include "Arduino.h"

namespace Leds
{
    struct LedContext {
        unsigned long lastUpdateMillis;
        bool toggleFast;
        bool toggleSlow;
    };

    static LedContext ctx = {0, false, false};

    void init() {
        pinMode(Pins::UI::LED_RED, OUTPUT);
        pinMode(Pins::UI::LED_GREEN, OUTPUT);
        digitalWrite(Pins::UI::LED_RED, LOW);
        digitalWrite(Pins::UI::LED_GREEN, LOW);
        ctx = {0, false, false};
    }

    void update(const SharedPanel &localPanel) {
        unsigned long currentMillis = millis();

        ctx.toggleFast = (currentMillis / Config::Timing::LED_BLINK_FAST) % 2 == 0;
        ctx.toggleSlow = (currentMillis / Config::Timing::LED_BLINK_SLOW) % 2 == 0;

        switch (localPanel.ui.led.red) {
            case LedMode::On:        digitalWrite(Pins::UI::LED_RED, HIGH); break;
            case LedMode::BlinkFast: digitalWrite(Pins::UI::LED_RED, ctx.toggleFast ? HIGH : LOW); break;
            case LedMode::BlinkSlow: digitalWrite(Pins::UI::LED_RED, ctx.toggleSlow ? HIGH : LOW); break;
            case LedMode::Off:
            default:                 digitalWrite(Pins::UI::LED_RED, LOW); break;
        }

        switch (localPanel.ui.led.green) {
            case LedMode::On:        digitalWrite(Pins::UI::LED_GREEN, HIGH); break;
            case LedMode::BlinkFast: digitalWrite(Pins::UI::LED_GREEN, ctx.toggleFast ? HIGH : LOW); break;
            case LedMode::BlinkSlow: digitalWrite(Pins::UI::LED_GREEN, ctx.toggleSlow ? HIGH : LOW); break;
            case LedMode::Off:
            default:                 digitalWrite(Pins::UI::LED_GREEN, LOW); break;
        }
    }
}
