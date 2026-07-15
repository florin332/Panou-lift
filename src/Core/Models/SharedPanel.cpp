
#include "SharedPanel.h"
#include "Arduino.h"

// INSTANȚIEREA FIZICĂ A MAGISTRALEI DE DATE PARTAJATE PENTRU TOT PROIECTUL
volatile SharedMemory gSharedMemory = {0, {}};

void shared_panel_write(volatile SharedMemory &sharedMem, const SharedPanel &newData) {
    sharedMem.seq++; // Pasul 1: Marcăm începutul scrierii (Devine IMPAR)
    asm volatile("dmb" : : : "memory"); // Barieră de memorie hardware ARM
    
    // --- COPIERE PRIMITIVĂ TYPE-SAFE STRUCTURA LIFT 1 ---
    sharedMem.panel.lift1.pos = newData.lift1.pos;
    sharedMem.panel.lift1.etd = newData.lift1.etd;
    sharedMem.panel.lift1.ocp = newData.lift1.ocp;
    sharedMem.panel.lift1.sj  = newData.lift1.sj;
    sharedMem.panel.lift1.svc = newData.lift1.svc;

    // --- COPIERE PRIMITIVĂ TYPE-SAFE STRUCTURA LIFT 2 ---
    sharedMem.panel.lift2.pos = newData.lift2.pos;
    sharedMem.panel.lift2.etd = newData.lift2.etd;
    sharedMem.panel.lift2.ocp = newData.lift2.ocp;
    sharedMem.panel.lift2.sj  = newData.lift2.sj;
    sharedMem.panel.lift2.svc = newData.lift2.svc;

    // --- COPIERE PRIMITIVĂ TYPE-SAFE STRUCTURA UI (LEDS & SOUND & SCREENS) ---
    sharedMem.panel.ui.led.red      = newData.ui.led.red;
    sharedMem.panel.ui.led.green    = newData.ui.led.green;
    sharedMem.panel.ui.sound.event  = newData.ui.sound.event;
    sharedMem.panel.ui.sound.seq    = newData.ui.sound.seq;
    sharedMem.panel.ui.screen.tft1  = newData.ui.screen.tft1;
    sharedMem.panel.ui.screen.tft2  = newData.ui.screen.tft2;
    sharedMem.panel.ui.brightness   = newData.ui.brightness;
    sharedMem.panel.ui.theme        = newData.ui.theme;

    // --- COPIERE PRIMITIVĂ TYPE-SAFE STRUCTURA SYSTEM ---
    sharedMem.panel.system.uptimeSeconds      = newData.system.uptimeSeconds;
    sharedMem.panel.system.packetErrorsLift1  = newData.system.packetErrorsLift1;
    sharedMem.panel.system.packetErrorsLift2  = newData.system.packetErrorsLift2;
    sharedMem.panel.system.seqlockCollisions  = newData.system.seqlockCollisions;
    sharedMem.panel.system.bootCounter        = newData.system.bootCounter;
    sharedMem.panel.system.lastResetReason    = newData.system.lastResetReason;
    
    asm volatile("dmb" : : : "memory"); // Asigură scrierea datelor înainte de modificarea seq finală
    sharedMem.seq++; // Pasul 2: Validăm snapshot-ul stabil (Devine PAR)
}

bool shared_panel_read(volatile const SharedMemory &sharedMem, SharedPanel &localCopy) {
    uint32_t start_seq, end_seq;
    uint8_t retries = 0;
    
    do {
        start_seq = sharedMem.seq;
        asm volatile("dmb" : : : "memory"); // Împiedică anticiparea citirii datelor din bus fabric
        
        // --- EXTRASEGMENTARE PRIMITIVĂ DIN MEMORIA VOLATILĂ SPRE STIVĂ LOCALĂ ---
        localCopy.lift1.pos = sharedMem.panel.lift1.pos;
        localCopy.lift1.etd = sharedMem.panel.lift1.etd;
        localCopy.lift1.ocp = sharedMem.panel.lift1.ocp;
        localCopy.lift1.sj  = sharedMem.panel.lift1.sj;
        localCopy.lift1.svc = sharedMem.panel.lift1.svc;

        localCopy.lift2.pos = sharedMem.panel.lift2.pos;
        localCopy.lift2.etd = sharedMem.panel.lift2.etd;
        localCopy.lift2.ocp = sharedMem.panel.lift2.ocp;
        localCopy.lift2.sj  = sharedMem.panel.lift2.sj;
        localCopy.lift2.svc = sharedMem.panel.lift2.svc;

        localCopy.ui.led.red     = sharedMem.panel.ui.led.red;
        localCopy.ui.led.green   = sharedMem.panel.ui.led.green;
        localCopy.ui.sound.event = sharedMem.panel.ui.sound.event;
        localCopy.ui.sound.seq   = sharedMem.panel.ui.sound.seq;
        localCopy.ui.screen.tft1 = sharedMem.panel.ui.screen.tft1;
        localCopy.ui.screen.tft2 = sharedMem.panel.ui.screen.tft2;
        localCopy.ui.brightness  = sharedMem.panel.ui.brightness;
        localCopy.ui.theme       = sharedMem.panel.ui.theme;

        localCopy.system.uptimeSeconds     = sharedMem.panel.system.uptimeSeconds;
        localCopy.system.packetErrorsLift1 = sharedMem.panel.system.packetErrorsLift1;
        localCopy.system.packetErrorsLift2 = sharedMem.panel.system.packetErrorsLift2;
        localCopy.system.seqlockCollisions = sharedMem.panel.system.seqlockCollisions;
        localCopy.system.bootCounter       = sharedMem.panel.system.bootCounter;
        localCopy.system.lastResetReason   = sharedMem.panel.system.lastResetReason;
        
        asm volatile("dmb" : : : "memory"); // Împiedică devansarea citirii contorului final seq
        end_seq = sharedMem.seq;
        
        retries++;
        
        // Corecție conform Observației 6: prag limitat strict la 3 reîncercări pentru fluiditate SPI
        if (retries > 3) {
            localCopy.system.seqlockCollisions++; // Incrementăm contorul de diagnoză pe stiva locală
            return false; // Renunțăm instant la eșantionul corupt pentru a păstra determinismul
        }
        
    } while ((start_seq != end_seq) || (start_seq & 1));
    
    return true;
}
