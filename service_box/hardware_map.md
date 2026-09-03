## ServiceBox - Architecture & Hardware Map

Arhitectura utilizează un strat Hardware Abstraction Layer (HAL) care separă logica de business prin `HardwareInterface`, folosind directive `#ifdef` pentru compilație condiționată în funcție de placă.

## 🟢 Varianta A: GroundStudio Marble Pico (`SERVICEBOX_MARBLE`)
- **Microcontroler:** RP2040
- **Display:** ILI9341 (320x240, CS=0, DC=1, RST=2, MOSI=3, SCK=6)
- **Touch:** Rezistiv XPT2046 (CS=7, MISO=4)
- **UART1:** TX=GP20, RX=GP21 (115200 baud)

## 🔵 Varianta B: Waveshare RP2350 Touch LCD (`SERVICEBOX_WAVESHARE`)
- **Microcontroler:** RP2350
- **Display:** ST7789T3 (320x240, CS=GP13, DC=GP14, RST=GP15, BL=GP16, MOSI=GP11, MISO=GP12, SCLK=GP10)
- **Touch:** Capacitiv CST328 (SDA=GP6, SCL=GP7, RST=GP17, INT=GP18)
- **IMU:** QMI8658 (pe I2C0: SDA=GP6, SCL=GP7, INT1=GP23, INT2=GP24)
- **MicroSD:** SPI1 (CS=GP8, MOSI=GP11, MISO=GP12, SCLK=GP10)
- **UART1:** TX=GP4, RX=GP5 (115200 baud)
