# ServiceBox - Architecture & Hardware Map

## Arhitectură Generală (HAL)
Logica de business este complet separată de hardware prin interfața `HardwareInterface`. Se folosește directiva `#ifdef` pentru a compila codul specific fiecărei plăci.

---

## 🟢 Varianta A: GroundStudio Marble Pico (`SERVICEBOX_MARBLE`)
- **Microcontroler:** RP2040 (Modular)
- **Display:** ILI9341 (sau ILI9340) - 320x240
  - `TFT_CS` = 0
  - `TFT_DC` = 1
  - `TFT_RST` = 2
  - `TFT_MOSI` = 3
  - `TFT_SCK` = 6
- **Touch:** Rezistiv XPT2046
  - `TOUCH_CS` = 7
  - `TOUCH_MISO` = 4
- **Comunicație (UART1):** 
  - `TX` = GP20
  - `RX` = GP21 (Baud: 115200)

---

## 🔵 Varianta B: Waveshare RP2350 Touch LCD (`SERVICEBOX_WAVESHARE`)
- **Microcontroler:** RP2350 (Integrat)
- **Display:** ST7789T3 - 320x240 (Rotit Landscape)
  - `TFT_CS` = 13
  - `TFT_DC` = 14
  - `TFT_RST` = 15
  - `TFT_MOSI` = 11
  - `TFT_SCLK` = 10
- **Touch:** Capacitiv CST328 (Magistrală I2C)
  - `SDA` = GP6
  - `SCL` = GP7
  - `INT/IRQ` = GP17
- **Comunicație (UART1):** 
  - `TX` = GP4
  - `RX` = GP5 (Baud: 115200)
