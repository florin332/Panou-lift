# Palier Duplex Panel - Local Development TODO List

## 1. Asynchronous USB Client Parser
- [ ] Expand string tokenization inside `processCommand()` using `indexOf()` and `substring()` to split parameters safely.
- [ ] Fix the boundary offset index configuration for `mb_display_reinit` to avoid invalid look-ahead memory reads.
- [ ] Implement direct automated ASCII feedback transmissions: `ACK SRV ENTER`, `ACK SRV EXIT`, and global `ERR` strings.

## 2. Dual TFT Graphic Rendering Layers
- [ ] Code the service layout frame overlay to lock out standard display rendering when `mb_in` status is active.
- [ ] Map the tokenized data array from the `mb_com` response payload directly into separate Left/Right text layout objects.
- [ ] Create the fullscreen visual state switcher for the isolated **"PASS"** graphic testing pattern (`mb_display_test`).
- [ ] Fill the low-level hardware controller reinitialization placeholder inside the display reset branch.

## 3. Background RS485 Processing
- [ ] Secure dual-channel asynchronous packet receiving via `Serial1` (Lift 2) and `Serial2` (Lift 1) inside `Protocol.cpp`.
- [ ] Optimize the `parsePacket()` validator structure to translate raw packets safely into `LiftState` memory spaces.

## 4. Service Mode — recuperare la deconectare USB accidentală (BLOCAT: verificare fizică flash)
Problemă: dacă USB-ul cade cât Service Mode e activ, panoul rămâne blocat (singura ieșire actuală este `mb_00`, imposibil de trimis; doar reset-ul hardware cu 4 apăsări rapide îl recuperează).

- [ ] **Verificat fizic mai întâi:** un timeout de inactivitate în Service Mode NU trebuie să afecteze procedura de flash prin `mb_02` (ACK + delay 170 ms + `reset_usb_boot`); timeout-ul rulează pe core0 în `ServiceMenu::update()`, iar între primirea `mb_02` și execuția ei expirarea nu trebuie să intre pe fir. Test cu Service Box real, nu doar build.
- [ ] Implementat timeout inactivitate (varianta agreată: N min, propus 5–10 min): orice comandă validă resetează cronometrul (`sLastActivityMillis` din `ServiceMenu.cpp` există deja, dar nu e folosit); la expirare se iese silențios din service (`stopColorTest` + `Display::setServiceMode(false)` + invalidare randare), fără ACK pe un link mort.
- [ ] Decis comportamentul cu `mb_17` activ: testul de culori se oprește la timeout (propus: da — test activ fără USB e nesigur) SAU testul activ resetează cronometrul.
- [ ] Documentat escape hatch-ul existent în `service_commands.md`: 4 apăsări rapide buton apel = reset hardware = ieșire garantată din Service Mode.
- [ ] (Opțional, ulterior) Detecție DTR (`Serial.dtr()` există în Arduino-Pico 6.3.0) ca reacție rapidă la închiderea portului de host; nesigur la detach fizic — necesită test în teren înainte de adoptare. NU heartbeat: box-ul trebuie emulabil din serial monitor pe PC.
