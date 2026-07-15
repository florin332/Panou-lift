#ifndef SHARED_PANEL_H
#define SHARED_PANEL_H

#include <stdint.h>

// ======== ENUMERĂRI TIPISATE EXCLUSIV PENTRU LIFTURI (Observația 1) ========
enum class Direction : uint8_t {
    Idle,   // Fostul 0
    Up,     // Fostul 1
    Down    // Fostul 2
};

enum class ServiceState : uint8_t {
    Fault,    // Fostul 1
    Revision, // Fostul 2
    Missing,  // Fostul 3
    Normal    // Fostul 5
};

enum class Occupancy : uint8_t {
    Free,   // Fostul 0
    Busy    // Fostul 1
};

// ======== ENUMERĂRI TIPISATE EXCLUSIV PENTRU INTERFAȚA UI ========
enum class SoundEvent : uint8_t { None, Boot, Confirm, Arrival, Error };
enum class LedMode    : uint8_t { Off, On, BlinkFast, BlinkSlow };
enum class ScreenMode : uint8_t { Normal, StandbyActive, StandbyShift };

// Structura curată a unui singur ascensor (Fără primitive numerice - Observația 1)
struct LiftState {
    uint8_t pos;        // Etaj curent (0-17)
    uint8_t etd;        // Etaj destinație (0-17)
    Occupancy ocp;      // Ocupat / Liber
    Direction sj;       // Idle, Up, Down
    ServiceState svc;   // Fault, Revision, Missing, Normal
};

struct LedState {
    LedMode red;
    LedMode green;
};

struct SoundState {
    SoundEvent event;
    uint8_t seq; 
};

struct ScreenState {
    ScreenMode tft1;
    ScreenMode tft2;
};

struct UiState {
    LedState led;
    SoundState sound;
    ScreenState screen;
    uint8_t brightness;  
    uint8_t theme;       
};

struct SystemState {
    uint32_t uptimeSeconds;
    uint16_t packetErrorsLift1;
    uint16_t packetErrorsLift2;
    uint16_t seqlockCollisions; 
    uint16_t bootCounter;       
    uint8_t  lastResetReason;   
};

struct SharedPanel {
    LiftState lift1;
    LiftState lift2;
    UiState ui;                 
    SystemState system;
};

struct alignas(8) SharedMemory {
    volatile uint32_t seq;
    SharedPanel panel;
};

extern volatile SharedMemory gSharedMemory;

void shared_panel_write(volatile SharedMemory &sharedMem, const SharedPanel &newData);
bool shared_panel_read(volatile const SharedMemory &sharedMem, SharedPanel &localCopy);

#endif // SHARED_PANEL_H
