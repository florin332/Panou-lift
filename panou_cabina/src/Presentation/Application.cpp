// src/Presentation/Application.cpp (Orchestratorul General cu Persistență Flash + Service Mode)

#include "Application.h"
#include "Arduino.h"
#include "SharedPanel.h"
#include "Config.h"
#include "Pins.h"
#include "AlarmService.h"
#include "ConfigurationService.h"
#include "DiagnosticService.h"
#include "ElevatorService.h"
#include "PresentationController.h"
#include "SDCard.h"
#include "Sound.h"
#include <hardware/watchdog.h>

namespace
{
    struct ResetButtonState {
        bool rawState = true;
        bool stableState = true;
        unsigned long lastRawChangeMillis = 0;
        unsigned long firstPressMillis = 0;
        uint8_t pressCount = 0;
    };

    ResetButtonState sResetButton;

    void initResetButton() {
        pinMode(Pins::Inputs::RESET, INPUT_PULLUP);
        const bool initialState = digitalRead(Pins::Inputs::RESET) == HIGH;
        sResetButton.rawState = initialState;
        sResetButton.stableState = initialState;
        sResetButton.lastRawChangeMillis = millis();
        sResetButton.firstPressMillis = 0;
        sResetButton.pressCount = 0;
    }

    void updateResetButton() {
        const unsigned long now = millis();
        const bool rawState = digitalRead(Pins::Inputs::RESET) == HIGH;
        if (rawState != sResetButton.rawState) {
            sResetButton.rawState = rawState;
            sResetButton.lastRawChangeMillis = now;
        }

        if (now - sResetButton.lastRawChangeMillis < Config::Timing::RESET_BUTTON_DEBOUNCE_MS ||
            rawState == sResetButton.stableState) {
            return;
        }

        sResetButton.stableState = rawState;
        if (rawState) return;

        if (sResetButton.pressCount == 0 ||
            now - sResetButton.firstPressMillis >= Config::Timing::RESET_PRESS_WINDOW_MS) {
            sResetButton.pressCount = 1;
            sResetButton.firstPressMillis = now;
            return;
        }

        sResetButton.pressCount++;
        if (sResetButton.pressCount >= Config::Timing::RESET_PRESS_COUNT) {
            watchdog_hw->scratch[0] = Config::Hardware::RESET_MAGIC_BUTTON;
            watchdog_start_tick(12);
            watchdog_reboot(0, 0, 0);
        }
    }
}

namespace Application
{
    void init() {
        initResetButton();
        ConfigurationService::init();
        AlarmService::init();
        ElevatorService::init();
        DiagnosticService::init();
        PresentationController::initCore0();

        SharedPanel initPanel = {};
        ConfigurationService::initializeStartupState(initPanel);
        AlarmService::initializeBootState(initPanel);
        ApplicationState::publish(initPanel);
    }

    void initCore1() {
        PresentationController::initCore1();
    }

    void runCore0() {
        Sound::update();
        updateResetButton();
        static SharedPanel localState = {};
        static unsigned long lastUptimeUpdate = 0;

        if (ApplicationState::read(localState)) {
            const Direction previousDirection = localState.lift1.sj;
            ElevatorService::update(localState);
            AlarmService::update(localState, previousDirection);

            if (millis() - lastUptimeUpdate >= 1000) {
                lastUptimeUpdate += 1000;
                localState.system.uptimeSeconds++;
            }

            ApplicationState::publish(localState);
            Sound::update();
        }

        PresentationController::pollCore0();
    }

    void runCore1() {
        SDCard::update();
        PresentationController::updateCore1();
    }
}