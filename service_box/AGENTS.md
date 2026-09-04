# AGENTS.md — Service Box

## 1. SCOPE

These instructions apply to the entire `service_box` project.

The Service Box is a dedicated service/configuration device for the
Panou-lift project.

The project contains hardware-specific implementations. Changes must
always be made for the explicitly requested target and must not
accidentally affect another hardware target.

---

## 2. GENERAL WORKING RULES

Before modifying code:

1. Inspect the current repository state.
2. Inspect the relevant existing implementation.
3. Inspect the corresponding hardware documentation.
4. Check `platformio.ini` and identify the affected PlatformIO environment.
5. Reuse existing abstractions and implementations whenever possible.

Do not assume that an older implementation, previous prompt, or previous
analysis still represents the current state of the project.

The current repository is the source of truth for software structure.

Confirmed hardware information is the source of truth for hardware
mapping.

---

## 3. TARGET SEPARATION

The Service Box may contain multiple hardware implementations.

In particular:

- Marble Pico
- Waveshare RP2350 Touch LCD 2.8"

Do NOT modify the Marble Pico implementation when working on the
Waveshare RP2350 target.

Do NOT modify the Waveshare implementation when working on Marble Pico.

Only modify another target when the task explicitly requires it.

---

## 4. WAVESHARE RP2350 HARDWARE

Current Waveshare target:

    waveshare_rp2350

Board/display:

    Waveshare RP2350 Touch LCD 2.8"

Display:

    ST7789-based
    320 x 240 physical display
    current UI orientation must follow the project UI specification

The LCD and touch are separate peripherals.

---

## 5. WAVESHARE LCD PINOUT

The confirmed LCD mapping is:

    TFT_CS   = GP13
    TFT_DC   = GP14
    TFT_RST  = GP15
    TFT_BL   = GP16
    TFT_MOSI = GP11
    TFT_MISO = GP12
    TFT_SCLK = GP10

LCD bus:

    SPI1

These pins are confirmed hardware assignments.

Do not change them without explicit instruction.

Important:

    GP16 = LCD backlight

Therefore GP16 must never be used as a touch reset pin or for another
unrelated hardware function.

---

## 6. WAVESHARE TOUCH

Touch controller:

    CST328

Interface:

    I2C

Confirmed touch mapping:

    TOUCH_SDA = GP6
    TOUCH_SCL = GP7
    TOUCH_RST = GP17
    TOUCH_INT = GP18

These touch pins have been physically verified.

Treat these pins as CONFIRMED HARDWARE INFORMATION.

Do not autodetect them.

Do not replace them with pins taken from a generic Waveshare example.

Do not invent alternative mappings.

Important:

    GP17 = touch reset
    GP18 = touch interrupt

The touch is NOT an XPT2046/SPI touchscreen on the Waveshare target.

Do not convert the CST328 interface to SPI.

Do not create a second SPI bus for the touch.

Use the CST328 implementation/library already used by the project.

The local `XPT2046_Touchscreen` library must not be used for the
Waveshare CST328 hardware.

---

## 7. LCD AND TOUCH BUS SEPARATION

The Waveshare LCD uses:

    SPI1

The Waveshare touch uses:

    I2C

Do not assume that LCD and touch share the same bus.

Do not introduce SPI chip-select handling for the CST328.

Do not change the LCD bus in order to implement touch.

Do not move `Wire`, `Wire1`, SPI1, or another peripheral to a different
bus without first establishing that the current hardware architecture
requires it.

If a bus change appears necessary, stop and report the conflict before
making an architectural change.

---

## 8. EXISTING HARDWARE ABSTRACTION

The project already contains a hardware abstraction.

Before adding new hardware code, inspect:

    src/hal/HardwareInterface.h

and the corresponding hardware implementations.

For touch, the existing interface includes the concepts:

    updateTouch()
    isScreenTouched()
    getTouchX()
    getTouchY()

Reuse the existing abstraction.

Do not create a parallel touch API if the existing abstraction can support
the requested functionality.

UI code must not directly access the CST328 hardware when the HAL already
provides the required interface.

---

## 9. LCD STATUS

The Waveshare LCD initialization is an already-developed part of the
project.

When working on another feature:

- preserve the working LCD implementation;
- do not rewrite the LCD driver unnecessarily;
- do not replace the LCD library;
- do not move the LCD away from SPI1;
- do not change confirmed LCD GPIO assignments;
- do not alter the LCD initialization sequence unless the task explicitly
  requires it.

A task concerning touch, UI, battery, RS485, SD, or another subsystem
must not become an excuse to rewrite the LCD implementation.

---

## 10. UI ARCHITECTURE

The Service Box UI is defined by the project's UI documentation and
diagram.

Relevant files include:

    src/ui_requirements.md
    svcbox_menu.drawio
    menu.md
    service_menu.md

The UI implementation must follow these documents.

Do not invent a different menu hierarchy when implementing UI features.

Do not redesign the UI unless explicitly requested.

Hardware input handling and UI behaviour should remain separated.

For example:

    hardware
        ↓
    HAL
        ↓
    input/state
        ↓
    UI
        ↓
    menu/application logic

Do not bypass these layers without a concrete reason.

---

## 11. HARDWARE DOCUMENTATION

The hardware documentation is:

    hardware_map.md

When hardware assignments change as a result of an approved hardware
decision, update the documentation so that it reflects the real hardware.

Do not modify hardware documentation merely to make it agree with an
incorrect software assumption.

Never silently resolve a hardware conflict by guessing.

If software, documentation and confirmed hardware disagree:

1. identify the conflict;
2. report it;
3. use confirmed hardware information as the priority;
4. update documentation only when appropriate.

---

## 12. PIN SAFETY

Never invent GPIO assignments.

Before assigning a GPIO:

1. inspect `hardware_map.md`;
2. inspect the existing HAL implementation;
3. inspect `HardwareInterface.h`;
4. inspect `platformio.ini` if relevant;
5. check for existing peripheral use.

Never reuse a GPIO simply because it is convenient.

Particular confirmed Waveshare assignments:

    GP6  = touch SDA
    GP7  = touch SCL
    GP10 = LCD SCLK
    GP11 = LCD MOSI
    GP12 = LCD MISO
    GP13 = LCD CS
    GP14 = LCD DC
    GP15 = LCD reset
    GP16 = LCD backlight
    GP17 = touch reset
    GP18 = touch interrupt

Do not change these assignments without explicit approval.

---

## 13. SPI / I2C RULES

Do not change bus architecture based on library defaults.

Always inspect how the project currently initializes:

    SPI1
    Wire
    Wire1

before modifying peripheral communication.

A library using a global `SPI` or `Wire` object does not automatically
mean that the project should be moved to that bus.

The physical hardware mapping and existing working implementation take
priority.

---

## 14. PLATFORMIO

Always inspect:

    platformio.ini

before changing dependencies or build configuration.

Build only the affected environment unless the task explicitly requires
testing multiple environments.

Do not introduce arbitrary library versions.

Do not reintroduce known-invalid PlatformIO dependency specifications.

In particular, avoid dependency specifications that produce errors such
as:

    SemanticVersionError:
    Invalid simple spec:
    ^0.0.0-alpha+sha...

Use the existing project dependency arrangement whenever possible.

---

## 15. LIBRARIES

Before adding a library:

1. check whether the project already contains an implementation;
2. check `platformio.ini`;
3. check the local `lib/` directory;
4. check whether the required functionality already exists in the HAL.

Do not add duplicate libraries.

Do not replace a working library without a concrete requirement.

For Waveshare touch, use the project's CST328 implementation.

Do not substitute XPT2046 for CST328.

---

## 16. MINIMAL CHANGES

Make the smallest change required to complete the requested task.

Do not perform unrelated refactoring.

Do not:

- rename unrelated files;
- reorganize directories unnecessarily;
- rewrite working drivers;
- change unrelated GPIOs;
- change RS485 behaviour;
- change Ethernet behaviour;
- change SD behaviour;
- change battery behaviour;
- change UF2 behaviour;
- change the menu architecture;
- change another hardware target.

Working code has priority over stylistic cleanup.

---

## 17. EXISTING CODE TAKES PRIORITY OVER ASSUMPTIONS

If an existing implementation looks unusual but works with the current
hardware, do not replace it merely because another implementation appears
cleaner.

Before changing working code, establish a concrete reason:

- hardware requirement;
- documented requirement;
- compile failure;
- functional bug;
- explicitly requested architectural change.

Do not "improve" working code without justification.

---

## 18. TASK BOUNDARIES

Implement ONLY the requested task.

Example:

If the task is:

    Implement CST328 touch reading.

Then the task does NOT automatically include:

- final menu navigation;
- gestures;
- animations;
- button widgets;
- UI redesign;
- calibration screens;
- battery changes;
- RS485 changes.

Implement the requested layer first.

Validate it.

Only then proceed to the next layer.

---

## 19. DEBUGGING

Temporary diagnostic output is allowed when it helps validate hardware.

Diagnostic code should:

- be simple;
- be non-blocking where practical;
- avoid unnecessary long delays;
- not interfere with normal operation;
- be easy to remove.

Do not leave continuous debug output in normal operation unless explicitly
required.

When testing touch, useful diagnostic information may include:

    TOUCH: X=... Y=...

Only report touch events when appropriate; do not continuously flood the
serial port when the screen is untouched.

---

## 20. TIMING AND BLOCKING

Avoid unnecessary `delay()` calls in application and input handling.

Hardware initialization may require delays when dictated by the device
datasheet or a verified working implementation.

Runtime UI and input processing should remain responsive.

Do not introduce long blocking waits into the main loop merely to simplify
a test.

---

## 21. UI INPUT

Touch input should be handled as an input source, not mixed directly into
menu rendering code.

The preferred separation is:

    CST328
       ↓
    Hardware HAL
       ↓
    touch state / coordinates
       ↓
    UI input handling
       ↓
    menu action

Raw hardware coordinates and UI/screen coordinates should remain
conceptually separate.

Do not introduce arbitrary calibration or coordinate transformations
without verifying the physical display orientation and touch behaviour.

---

## 22. DOCUMENTATION VS TEMPORARY TESTS

Temporary hardware test code is not automatically part of the final UI.

If a task requires a temporary test:

- clearly isolate it;
- keep it minimal;
- do not let it become permanent architecture;
- remove or disable it when the validation phase is complete, unless the
  task explicitly requires retaining it.

---

## 23. BUILD VALIDATION

After modifying code:

1. Build the affected PlatformIO environment.
2. Check compiler errors and warnings relevant to the change.
3. Check for GPIO conflicts.
4. Check that existing buses remain unchanged unless explicitly required.
5. Check that unrelated targets were not modified.
6. Report the build result.

If the build fails:

- fix errors caused by the current task;
- do not perform unrelated refactoring to hide the problem.

---

## 24. HARDWARE VALIDATION

Compilation is not sufficient for hardware changes.

When a task involves physical hardware, distinguish between:

    BUILD VERIFIED

and:

    HARDWARE VERIFIED

Do not claim hardware functionality has been verified merely because the
code compiles.

If physical verification is still required, state that explicitly.

---

## 25. FILE MODIFICATION RULES

Before modifying a file, inspect its current contents.

Do not replace an entire file when a small modification is sufficient.

After modification, report:

1. every modified file;
2. what was changed;
3. why it was changed;
4. which existing functionality was preserved;
5. any hardware pins or buses affected.

---

## 26. IMPORTANT PROJECT FILES

When working on Service Box, relevant files may include:

    AGENTS.md
    hardware_map.md
    platformio.ini
    svcbox_menu.drawio
    menu.md
    service_menu.md
    todo.md

and:

    src/main.cpp
    src/hal/HardwareInterface.h
    src/hal/HardwareWaveshare.cpp
    src/hal/HardwareMarble.cpp
    src/ui_requirements.md

Inspect only the files relevant to the current task, but always check the
existing architecture before creating new code.

---

## 27. SOURCE OF TRUTH PRIORITY

When information conflicts, use this priority:

1. Confirmed physical hardware information
2. Current working implementation
3. Current hardware documentation
4. Current UI specification / project documentation
5. Historical prompts, analyses or temporary test instructions
6. Generic assumptions or library examples

Historical prompts and analysis files must NOT override the current
repository state.

---

## 28. NO GUESSING RULE

If an important technical detail cannot be established from:

- the current code;
- the current documentation;
- confirmed hardware information;
- or an explicitly supplied requirement,

do not guess.

Report what is missing and identify the decision that must be made.

This is especially important for:

- GPIO mappings;
- buses;
- display orientation;
- touch coordinate transformations;
- library selection;
- protocol behaviour.

---

## 29. FINAL REPORT

At the end of every implementation task, provide a concise report:

    Files modified:
    - ...

    Changes:
    - ...

    Hardware affected:
    - ...

    Build:
    - ...

    Hardware verification:
    - ...

    Remaining issues:
    - ...

Do not claim successful hardware operation unless it was actually tested.

---

## 30. FINAL PRINCIPLE

Preserve what already works.

Make the smallest correct change.

Do not guess hardware.

Do not mix hardware targets.

Do not turn a focused task into a refactoring project.

Validate every change.

Keep the Service Box architecture deterministic, documented and
incremental.