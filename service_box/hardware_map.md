# ServiceBox - Architecture & Hardware Map

Arhitectura utilizează un Hardware Abstraction Layer (HAL) prin `HardwareInterface`, cu
compilație condiționată pentru variantele hardware `SERVICEBOX_MARBLE` și
`SERVICEBOX_WAVESHARE`.

---

# 1. GroundStudio Marble Pico

Build configuration:

`SERVICEBOX_MARBLE`

MCU:

`RP2040`

## 1.1 LCD — ILI9341

Status: **NOT TESTED**

- [ ] TFT_CS  = GP9
- [ ] TFT_DC  = GP1
- [ ] TFT_RST = GP2
- [ ] TFT_BL  = GP13
- [ ] TFT_MOSI = GP11
- [ ] TFT_SCK  = GP10

## 1.2 Touch — XPT2046

Type:

`Resistive`

Status: **NOT TESTED**

- [ ] TOUCH_CS   = GP8
- [ ] TOUCH_MISO = GP12

## 1.3 SD Card

Status: **NOT TESTED**

- [ ] SD_CS   = GP5
- [ ] SD_MOSI = GP3
- [ ] SD_MISO = GP4
- [ ] SD_SCLK = GP6

## 1.4 USB / UF2

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

> Only the Waveshare LCD has been physically tested and confirmed functional.
>
> The `[x]` status applies only to the LCD/display interface itself.

## 2.2 Touch — CST328

Type:

`Capacitive`

Status: **NOT TESTED**

- [ ] TOUCH_SDA = GP6
- [ ] TOUCH_SCL = GP7
- [ ] TOUCH_INT = GP18
- [ ] TOUCH_RST = GP17

## 2.3 IMU — QMI8658

Bus:

`I2C0`

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

## 2.5 UART / RS485

Status: **NOT TESTED**

> UART pin mapping is not currently defined in `HardwareInterface.h`.
> Do not assume a pin mapping here until it is explicitly defined in the HAL.

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

### I2C0 signals

- SDA = GP6
- SCL = GP7

### Touch

- INT = GP18
- RST = GP17

### IMU

- INT1 = GP23
- INT2 = GP24

Status:

- [ ] Touch tested
- [ ] IMU tested
- [ ] Touch + IMU simultaneous operation tested

---

# 4. Current Hardware Validation Status

## Waveshare

- [x] LCD / Display
- [ ] Touch
- [ ] SD Card
- [ ] Battery measurement
- [ ] Charging detection
- [ ] RS485
- [ ] IMU
- [ ] LCD + Touch
- [ ] LCD + SD
- [ ] LCD + Touch + SD
- [ ] Full peripheral integration

## Marble Pico

- [ ] LCD / Display
- [ ] Touch
- [ ] SD Card
- [ ] Battery measurement
- [ ] Charging detection
- [ ] RS485
- [ ] USB
- [ ] UF2
- [ ] Full peripheral integration

---

# 5. Source of Truth

The GPIO definitions in this document must remain synchronized with:

`service_box/src/hal/HardwareInterface.h`

Current GPIO definitions are taken directly from the HAL header.

Do not mark a hardware function as tested merely because its GPIO mapping,
driver or initialization code exists.

A feature is marked `[x]` only after physical testing on the corresponding
target hardware.

Current confirmed hardware:

**Waveshare LCD only.**