// src/Presentation/Application.cpp (Orchestratorul General cu Persistență Flash + Service Mode)

#include "Application.h"
#include "Arduino.h"
#include "SharedPanel.h"
#include "AlarmService.h"
#include "ConfigurationService.h"
#include "DiagnosticService.h"
#include "ElevatorService.h"
#include "PresentationController.h"

namespace Application
{
    void init() {
        ConfigurationService::init();
        AlarmService::init();
        ElevatorService::init();
        DiagnosticService::init();
        PresentationController::init();

        SharedPanel initPanel = {};
        ConfigurationService::initializeStartupState(initPanel);
        AlarmService::initializeBootState(initPanel);
        ApplicationState::publish(initPanel);
    }

    void runCore0() {
        PresentationController::update();
    }

    void runCore1() {
        static SharedPanel localState = {};
        static unsigned long lastUptimeUpdate = 0;

        if (!ApplicationState::read(localState)) return;

        const Direction previousDirection = localState.lift1.sj;
        ElevatorService::update(localState);
        AlarmService::update(localState, previousDirection);

        if (millis() - lastUptimeUpdate >= 1000) {
            lastUptimeUpdate += 1000;
            localState.system.uptimeSeconds++;
        }

        ApplicationState::publish(localState);
    }
}