# Service Box - Local Development TODO List

## 1. Hardware Abstraction Layer & Calibration
- [ ] Connect and test the `TFT_BL` (Backlight) control line on the prototype breadboard.
- [ ] Adjust and verify resistive touch matrix boundaries (`RAW_X_MIN`, `RAW_X_MAX`) inside `HardwareMarble.cpp`.
- [ ] Implement software clipping via `constrain()` to prevent negative pixel coordinate outputs.

## 2. SD Card File System (SPI0)
- [ ] Implement a configuration file reader (`config.txt`) using the standard `<SD.h>` library layer.
- [ ] Extract and store system parameters from the SD card file: station layout variables, calibration offsets, and panel IDs.
- [ ] Create a fallback mechanism in the init phase if the SD card filesystem is missing or corrupt.

## 3. USB Host Comm State Machine
- [ ] Complete the non-blocking state loop inside `updateCommEngine()` to handle asynchronous string parsing.
- [ ] Test the 150ms timeout window tracking logic for out-of-order client packet drops.
- [ ] Enforce the 3-retry budget rule before raising a global pipeline failure alert on the local UI screen.
- [ ] Build visual pop-up templates to display client-returned error codes (`ERR 01`, `ERR 02`, `ERR 03`).

