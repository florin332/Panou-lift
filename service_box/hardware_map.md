# ServiceBox - Architecture & Hardware Map

Arhitectura utilizează compilare condiționată pentru variantele hardware
`SERVICEBOX_MARBLE` și `SERVICEBOX_WAVESHARE`. Accesul la hardware este
implementat per-target direct în `service_box/src/main.cpp` (HAL-ul abstract
a fost eliminat pe branch-ul `graphic_ui`).

---

# 1. GroundStudio Marble Pico

Build configuration:

`SERVICEBOX_MARBLE`

MCU:

`RP2040`

## 1.1 LCD — ILI9341

Status: **TESTED - OK**

- [X] TFT_CS  = GP13
- [X] TFT_DC  = GP6
- [X] TFT_RST = GP14
- [X] TFT_BL  = GP4
- [X] TFT_MOSI = GP11
- [X] TFT_SCK  = GP10

## 1.2 Touch — XPT2046

Type:

`Resistive`

Status: **TESTED - OK**

- [X] TOUCH_CS   = GP9
- [X] TOUCH_MISO = GP12
- [X] TOUCH_IRQ = GP8

## 1.3 SD Card

Bus:

`SPI0`

Status: **NOT TESTED**

Pinii sunt conectați fizic pe placă (conform documentației Marble Pico):

- [ ] SD_CS   = GP17  (DAT3/CD, chip select)
- [ ] SD_MOSI = GP19  (CMD, SPI0 TX)
- [ ] SD_MISO = GP16  (DAT0, SPI0 RX)
- [ ] SD_SCLK = GP18  (CLK, SPI0 SCK)
- [ ] SD_DETECT = GP22 (SD switch / card detect; se separă LED-ul onboard)
- DAT1, DAT2 = liberi (nefolosiți în modul SPI)

## 1.4 Buton recalibrare touch

Status: **TESTED - OK**

- [X] RECALIB_BUTTON = GP5  (INPUT_PULLUP, activ LOW; ținut 2s -> recalibrare)

## 1.5 USB / UF2

Status: **NOT TESTED**

- [ ] USB connection
- [ ] UF2 bootloader detection
- [ ] UF2 flashing
- [ ] UF2 verification

---

# 2. Waveshare RP2350 Touch LCD

Build configuration:

`SERVICEBOX_WAVESHARE`

MCU:

`RP2350`

## 2.1 LCD — ST7789T3

Resolution:

`320 x 240`

Status: **TESTED / OK**

- [x] TFT_CS   = GP13
- [x] TFT_DC   = GP14
- [x] TFT_RST  = GP15
- [x] TFT_BL   = GP16
- [x] TFT_MOSI = GP11
- [x] TFT_MISO = GP12
- [x] TFT_SCLK = GP10

//> Only the Waveshare LCD has been physically tested and confirmed functional.
//>
//> The `[x]` status applies only to the LCD/display interface itself.

## 2.2 Touch — CST328

Type:

`Capacitive`

Status: **TESTED/ OK**

- [x] TOUCH_SDA = GP6
- [x] TOUCH_SCL = GP7
- [x] TOUCH_INT = GP18
- [x] TOUCH_RST = GP17

## 2.3 IMU — QMI8658

Bus:

`I2C1`

Status: **NOT TESTED**

- [ ] IMU_SDA  = GP6
- [ ] IMU_SCL  = GP7
- [ ] IMU_INT1 = GP23
- [ ] IMU_INT2 = GP24

## 2.4 SD Card

Bus:

`SPI1`

Status: **NOT TESTED**

- [ ] SD_CS   = GP8
- [ ] SD_MOSI = GP11
- [ ] SD_MISO = GP12
- [ ] SD_SCLK = GP10

## 2.5 Buton recalibrare touch

Status: **TESTED - OK**

- [X] RECALIB_BUTTON = GP29  (INPUT_PULLUP, activ LOW; ținut 2s -> recalibrare)

## 2.6 UART / RS485

Status: **NOT TESTED**

> UART pin mapping is not currently defined in the implementation.
> Do not assume a pin mapping here until it is explicitly defined.

- [ ] TX = TBD
- [ ] RX = TBD
- [ ] Baud rate = TBD

---

# 3. Shared / Bus Relationships

## 3.1 Waveshare LCD + SD

LCD and SD share the same SPI bus.

### SPI signals

- MOSI = GP11
- MISO = GP12
- SCLK = GP10

### Chip Select

- LCD CS = GP13
- SD CS  = GP8

Status:

- [x] LCD SPI operation tested
- [ ] SD SPI operation tested
- [ ] LCD + SD simultaneous operation tested

---

## 3.2 Waveshare Touch + IMU

Touch and IMU share the same I2C bus.

### I2C1 signals

- SDA = GP6
- SCL = GP7

### Touch

- INT = GP18
- RST = GP17

### IMU

- INT1 = GP23
- INT2 = GP24

Status:

- [x] Touch tested
- [ ] IMU tested
- [ ] Touch + IMU simultaneous operation tested

---

# 4. Current Hardware Validation Status

## Waveshare

- [x] LCD / Display
- [x] Touch
- [ ] SD Card
- [ ] Battery measurement
- [ ] Charging detection
- [ ] RS485
- [ ] IMU
- [x] LCD + Touch
- [ ] LCD + SD
- [ ] LCD + Touch + SD
- [ ] Full peripheral integration

## Marble Pico

- [x] LCD / Display
- [x] Touch
- [ ] SD Card
- [ ] Battery measurement
- [ ] Charging detection
- [ ] RS485
- [ ] USB
- [ ] UF2
- [ ] Full peripheral integration

---

# 5. Source of Truth

The GPIO definitions in this document must remain synchronized with the
implementation in:

`service_box/src/main.cpp`

(sections `#if defined(SERVICEBOX_MARBLE)` / `#if defined(SERVICEBOX_WAVESHARE)`).

Do not mark a hardware function as tested merely because its GPIO mapping,
driver or initialization code exists.

A feature is marked `[x]` only after physical testing on the corresponding
target hardware.

Current confirmed hardware:

**Waveshare: LCD + Touch (incl. recalibration button GP29).**
**Marble: LCD + Touch (incl. recalibration button GP5).**