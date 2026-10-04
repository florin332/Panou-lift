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
    uint16_t seqlockCollisions;
    uint16_t bootCounter;
    uint8_t  lastResetReason;
};

// Contoare receptie seriala detaliate (incrementate doar cand counting e activat din Service Box)
struct CommLineCounters {
    uint32_t rxFrames;      // cadre complet delimitate (START...END)
    uint32_t rxValid;       // cadre acceptate (format + CRC + domeniu)
    uint32_t rxTimeout;     // cadru inceput, dar neterminat in timeout
    uint32_t rxFormatError; // delimitatori/campuri/format invalide
    uint32_t rxCrcError;    // CRC prezent, dar incorect
    uint32_t rxDataError;   // format corect, dar valori imposibile
};

struct CommState {
    CommLineCounters lift1;
    uint8_t countingEnabled; // 1 = numarare activa (pornita cu comm_count_enable)
};

struct SharedPanel {
    LiftState lift1;
    UiState ui;                 
    SystemState system;
    CommState comm;
};

struct alignas(8) SharedMemory {
    volatile uint32_t seq;
    SharedPanel panel;
};

extern volatile SharedMemory gSharedMemory;

void shared_panel_write(volatile SharedMemory &sharedMem, const SharedPanel &newData);
bool shared_panel_read(volatile const SharedMemory &sharedMem, SharedPanel &localCopy);

#endif // SHARED_PANEL_H
