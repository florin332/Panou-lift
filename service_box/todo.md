# SVC_BOX TODO

## 🔴 1. Hardware bring-up — Waveshare

- [x] LCD / Display
- [ ] Touchscreen
- [ ] SD card
- [ ] Battery voltage measurement
- [ ] Charging detection
- [ ] RS485
- [ ] LCD + Touch + SD simultaneous operation
- [ ] Long-run stability test

> Current status: only the Waveshare LCD has been tested successfully.

---

## 🔴 2. Hardware bring-up — Marble Pico

- [ ] LCD / Display
- [ ] Touchscreen
- [ ] SD card
- [ ] Battery voltage measurement
- [ ] Charging detection
- [ ] RS485
- [ ] USB connection
- [ ] UF2 bootloader detection
- [ ] LCD + Touch + SD simultaneous operation
- [ ] Long-run stability test

> Nothing on the Marble hardware has been tested yet.

---

## 🟠 3. Base firmware / Hardware abstraction

- [ ] Board detection / board abstraction
- [ ] Display initialization
- [ ] Touch initialization
- [ ] SD initialization
- [ ] Battery measurement
- [ ] Charging state detection
- [ ] RS485 initialization
- [ ] USB / UF2 interface
- [ ] Hardware error states
- [ ] Watchdog / recovery handling

---

## 🟠 4. Startup sequence

Required sequence:

`BOOT → BATTERY CHECK → START SCREEN → MAIN MENU`

### BOOT

- [ ] Initialize MCU
- [ ] Initialize required hardware
- [ ] Detect hardware errors

### BATTERY CHECK

- [ ] Display `BATTERY LEVEL`
- [ ] Display battery percentage
- [ ] 5 second display period
- [ ] Red / yellow / green battery indication
- [ ] Detect charging
- [ ] While charging: display `CHARGING`
- [ ] While charging: remain on Battery Check page

### START SCREEN

- [ ] Display `SERVICE BOX`
- [ ] Display CPU status
- [ ] Display Display status
- [ ] Display Touch status
- [ ] Display RS485 status
- [ ] Display SD status
- [ ] START button
- [ ] START enters Main Menu
- [ ] No automatic entry into Main Menu

---

## 🟠 5. Main Menu

- [ ] Implement Main Menu according to `svcbox_menu.drawio`
- [ ] Navigation
- [ ] Back / Return handling
- [ ] Touch button states
- [ ] Error indication
- [ ] Consistent page layout

---

## 🟠 6. Service Mode

### Service Main

- [ ] STATUS
- [ ] DIAGNOSTIC
- [ ] COMMUNICATION
- [ ] DISPLAY
- [ ] RUNTIME
- [ ] EXIT

### Navigation

- [ ] `mb_in → Service Main`
- [ ] `mb_com → Communication`
- [ ] `mb_com_out → Service Main`
- [ ] `mb_out → Normal`

### STATUS

- [ ] Lift 1 status
- [ ] Lift 2 status
- [ ] General Service Box status

### DIAGNOSTIC

- [ ] System information
- [ ] Lift 1 information
- [ ] Lift 2 information
- [ ] Communication diagnostics
- [ ] Version / build information

### COMMUNICATION

Display real-time:

- [ ] Lift 1: `POS • DEST • S/J • OCP • SVC`
- [ ] Lift 2: `POS • DEST • S/J • OCP • SVC`
- [ ] RX errors
- [ ] Missing packets
- [ ] Timeouts
- [ ] Communication state

### DISPLAY

- [ ] Display test
- [ ] Touch test
- [ ] Backlight test

### RUNTIME

- [ ] Uptime
- [ ] Runtime information
- [ ] Relevant system counters

### EXIT

- [ ] Return to normal mode

---

## 🔴 7. RS485 / Panel communication

- [ ] RS485 electrical test
- [ ] Verify baud rate
- [ ] Verify frame format
- [ ] Verify timeout handling
- [ ] Panel detection
- [ ] Handshake
- [ ] Receive panel status
- [ ] Send commands
- [ ] Retry mechanism
- [ ] Error handling
- [ ] Communication watchdog
- [ ] Test with no panel connected
- [ ] Test with one panel
- [ ] Test with two panels
- [ ] Finalize ASCII protocol
- [ ] Finalize CRC-8 handling if enabled

### CRC-8

- [ ] Polynomial `0x07`
- [ ] Initial value `0x00`
- [ ] No reflection
- [ ] XOR out `0x00`
- [ ] CRC calculated over ASCII payload
- [ ] Exclude delimiters
- [ ] Exclude CRC field itself

---

## 🔴 8. SD card

- [ ] Detect SD card
- [ ] Display SD status
- [ ] Mount filesystem
- [ ] Read directory
- [ ] Read firmware files
- [ ] Handle missing SD
- [ ] Handle corrupted / unreadable SD
- [ ] Long-run read test
- [ ] Test simultaneous SD + LCD
- [ ] Test simultaneous SD + Touch
- [ ] Test simultaneous SD + RS485

---

## 🔴 9. UF2 firmware update

Service Box must be able to write a preloaded UF2 image from its own SD card to a blank Marble Pico.

- [ ] Detect Marble Pico over USB
- [ ] Detect UF2 bootloader
- [ ] Detect blank / unconfigured Marble
- [ ] Read UF2 files from SD
- [ ] Display available UF2 files
- [ ] Select UF2
- [ ] Validate UF2
- [ ] Confirmation before flashing
- [ ] Start flashing
- [ ] Display progress
- [ ] Handle USB disconnect/reconnect
- [ ] Verify result
- [ ] Reboot Marble
- [ ] Detect flashing failure
- [ ] Display error
- [ ] Test with completely blank Marble Pico

### Configuration behavior after UF2

- [ ] New UF2 deletes CONFIG
- [ ] Floor variable survives power loss
- [ ] `floor = 99` means unconfigured
- [ ] Verify behavior after firmware update

---

## 🟠 10. Error handling

- [ ] No SD
- [ ] Low battery
- [ ] Charging
- [ ] Display initialization failure
- [ ] Touch initialization failure
- [ ] RS485 failure
- [ ] No panel detected
- [ ] Panel timeout
- [ ] Invalid communication frame
- [ ] USB failure
- [ ] Invalid UF2
- [ ] UF2 flashing failure
- [ ] Recovery / reboot behavior

---

## 🟡 11. Integration tests

### Hardware

- [ ] Cold boot
- [ ] Warm reboot
- [ ] Battery only
- [ ] USB power
- [ ] Charging
- [ ] Low battery
- [ ] No SD
- [ ] SD inserted
- [ ] Touch
- [ ] RS485

### Peripheral combinations

- [ ] LCD + Touch
- [ ] LCD + SD
- [ ] LCD + RS485
- [ ] Touch + SD
- [ ] SD + RS485
- [ ] LCD + Touch + SD
- [ ] LCD + Touch + SD + RS485

### Panel communication

- [ ] No panel
- [ ] One panel
- [ ] Two panels
- [ ] Communication loss
- [ ] Recovery after communication loss
- [ ] Long-run communication test

### UF2

- [ ] Blank Marble Pico
- [ ] UF2 from SD
- [ ] Successful flash
- [ ] Failed flash
- [ ] Reboot after flash
- [ ] Verify `floor = 99`
- [ ] Verify CONFIG behavior

---

## 🟡 12. Final validation

- [ ] Full hardware test
- [ ] Full startup sequence
- [ ] Full menu navigation
- [ ] Full Service Mode
- [ ] Full RS485 communication
- [ ] Full SD functionality
- [ ] Full UF2 workflow
- [ ] Long-duration test
- [ ] Power-cycle test
- [ ] Recovery/error tests
- [ ] Final documentation update

---

## Status rule

**Do not mark a hardware feature as `[x]` until it has been physically tested on the target hardware.**

Current confirmed hardware status:

- Waveshare LCD: **TESTED / OK**
- Waveshare Touch: **NOT TESTED**
- Waveshare SD: **NOT TESTED**
- Waveshare Battery: **NOT TESTED**
- Waveshare Charging: **NOT TESTED**
- Waveshare RS485: **NOT TESTED**
- Marble LCD: **NOT TESTED**
- Marble Touch: **NOT TESTED**
- Marble SD: **NOT TESTED**
- Marble Battery: **NOT TESTED**
- Marble Charging: **NOT TESTED**
- Marble RS485: **NOT TESTED**
- Marble USB / UF2: **NOT TESTED**
