# Service Box UI Requirements

## 1. Purpose

Define the graphical UI for the Service Box.

The UI must be simple, professional and suitable for field service use.

Do not add functionality, screens or information that is not explicitly specified.

---

## 2. Display

- Target resolution: 240x320 pixels.
- Orientation: portrait.
- All hardware targets use the same logical UI.
- Display-specific code belongs to the HAL.
- Touch-specific code belongs to the HAL.
- UI code must not contain hardware-specific display/touch handling.

Hardware targets:

1. Waveshare RP2350-Touch-LCD-2.8
2. GroundStudio Marble Pico + 3.2" TFT

Both targets must provide the same user experience.

---

## 3. Startup Screen

The startup screen is a product splash screen.

Display:

- SERVICE BOX
- PANOURI ASCENSOARE
- firmware version
- START button

Optional SD information:

- SD status
- free space / total space

Example:

    SERVICE BOX

    PANOURI ASCENSOARE

         v1.0.0

        [ START ]

    SD  ● 742 MB / 1.8 GB FREE

Do NOT display on the startup screen:

- CPU status
- display status
- touch status
- RS485 status
- WDT status
- packet counters
- diagnostic information
- "Starting..."
- progress bars

The Service Box must remain usable if no SD card is present.

Startup flow:

    POWER ON
       ↓
    STARTUP SCREEN
       ↓
    user presses START
       ↓
    MAIN MENU

---

## 4. Touch

- Use large, clearly separated touch targets.
- Touch hitboxes may be slightly larger than the visible button.
- Avoid small controls.
- Avoid unnecessary gestures.
- Prefer simple tap interaction.
- Touch coordinates are provided by the HAL.

---

## 5. Visual Style

Style:

- industrial
- clean
- technical
- professional
- restrained

Avoid:

- unnecessary decoration
- excessive colors
- smartphone-style UI
- complex animations
- excessive information
- unnecessary icons

Use visual hierarchy through:

- size
- spacing
- borders
- contrast
- simple status indicators

---

## 6. Branding

Primary name:

SERVICE BOX

Product description:

PANOURI ASCENSOARE


---

## 7. UI Architecture

Keep UI independent from hardware.

Recommended structure:

    Application / Service Logic
             ↓
          UI Logic
             ↓
         UI Renderer
             ↓
            HAL
          ↙     ↘
      Display    Touch

Do not duplicate menu logic for different hardware targets.

Do not add hardware-specific #ifdef blocks to UI code unless absolutely necessary.

---

## 8. Development Rules

Before implementing a new screen:

1. Check the existing Service Box code.
2. Reuse existing functionality and interfaces.
3. Do not invent new functionality.
4. Do not modify HAL functionality unless required.
5. Keep the UI modular.
6. Keep screen definitions and touch handling easy to modify.

The UI specification is authoritative for visual and interaction decisions.