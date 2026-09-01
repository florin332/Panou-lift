# Service Box Menu

## Display

- Resolution: 240x320 pixels
- Orientation: portrait
- Touch controlled

---

# 1. Startup Screen

The Startup Screen is the first screen displayed after power-on.

Purpose:

- identify the Service Box
- show firmware version
- allow the operator to enter the main menu

The screen must be simple and clean.

## Layout

```text
┌──────────────────────────────┐
│                              │
│                              │
│       SERVICE BOX            │
│                              │
│       PANOURI ASCENSOARE     │
│                              │
│          v1.0.0              │
│                              │
│                              │
│        ┌──────────┐          │
│        │  START   │          │
│        └──────────┘          │
│                              │
│                              │
│                              │
│    SD ● 742 MB / 1.8 GB FREE │
│                              │
└──────────────────────────────┘
```

## Elements

### Title

Text:

`SERVICE BOX`

Large, centered.

### Subtitle

Text:

`PANOURI ASCENSOARE`

Centered below the title.

### Firmware version

Format:

`vX.X.X`

Centered.

### START button

Large touch button.

Pressing START opens the Main Menu.

### SD status

Optional.

Format:

`SD ● xxx MB / x.x GB FREE`

The displayed values must be read from the actual SD card.

If no SD card is present:

`SD ● NOT PRESENT`

SD absence must not prevent entering the Main Menu.

## Startup behavior

- Display Startup Screen after initialization.
- Do not automatically enter the Main Menu.
- Wait for the operator to press START.
- On START press, switch to Main Menu.

## Animation

No complex animation is required for the first implementation.

The layout should allow a future short startup animation without changing the screen structure.

---

# 2. Main Menu

TODO — define after Startup Screen implementation.
