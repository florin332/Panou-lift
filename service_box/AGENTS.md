# AGENTS.md — Service Box

SCOPE
These instructions apply to the service_box project.

CURRENT TASK
Repair the Waveshare RP2350 Touch LCD 2.8" display initialization.

Use the official working implementation in:
https://github.com/florin332/demo-rp2030

as the source of truth for the Waveshare LCD initialization.

CRITICAL RULES

- Work only on the waveshare_rp2350 target unless explicitly requested otherwise.
- Do NOT modify Marble Pico implementation.
- Do NOT modify RS485, Ethernet, protocol, or unrelated Service Box architecture.
- Do NOT introduce another LCD library.
- Do NOT move the LCD from SPI1.
- Avoid general refactoring.
- Keep changes minimal and directly related to the Waveshare LCD problem.

WAVESHARE LCD PINOUT

LCD:
TFT_CS   = 13
TFT_DC   = 14
TFT_RST  = 15
TFT_BL   = 16
TFT_MOSI = 11
TFT_MISO = 12
TFT_SCLK = 10

Touch:
TOUCH_SDA = 6
TOUCH_SCL = 7
TOUCH_RST = 17
TOUCH_INT = 18

IMPORTANT:
- GPIO16 is LCD backlight.
- GPIO17 is touch reset.
- GPIO18 is touch interrupt.
- Never use GPIO16 as touch reset.
- Add an explicit TFT_MISO definition for GPIO12.
- Do not reuse SD_MISO semantically for the LCD.

LCD SPI

The LCD uses SPI1.

Initialize SPI1 exactly like the working demo:

SPI1.setRX(TFT_MISO);
SPI1.setCS(TFT_CS);
SPI1.setSCK(TFT_SCLK);
SPI1.setTX(TFT_MOSI);
SPI1.begin();
SPI1.beginTransaction(SPISettings(66500000, MSBFIRST, SPI_MODE0));

Do NOT use the global SPI object for the LCD.

LCD RESET

Use the exact timing from the working demo:

digitalWrite(TFT_RST, HIGH);
delay(100);
digitalWrite(TFT_RST, LOW);
delay(100);
digitalWrite(TFT_RST, HIGH);
delay(100);

Do not use the current 20/20/50 ms sequence.

LCD INITIALIZATION REGISTER SEQUENCE

The display is an ST7789-based 320x240 LCD.

Use the register initialization from demo-rp2030/LCD_2in8.cpp.

Do not invent, optimize, or alter register values.

Required sequence:

0x29
0x11

0x3A -> 0x05

0xB2 -> 0C 0C 00 33 33
0xB7 -> 75
0xBB -> 1A
0xC0 -> 2C
0xC2 -> 01 FF
0xC3 -> 13
0xC4 -> 20
0xC6 -> 0F
0xD0 -> A4 A1
0xD6 -> A1

0xE0 ->
D0 0D 14 0D 0D 09 38 44 4E 3A 17 18 2F 30

0xE1 ->
D0 09 0F 08 07 14 37 44 4D 38 15 16 2C 2E

0x21
0x29
0x2C

For horizontal orientation the sequence must contain:

lcdWriteCmd(0x11);
delay(120);
lcdWriteCmd(0x36);
lcdWriteData(0x00);

The existing modified register sequence in HardwareWaveshare.cpp must be replaced if it differs from the working demo.

LCD COMMAND/DATA TRANSFER

Keep the low-level implementation equivalent to the working demo.

Command:

digitalWrite(TFT_DC, LOW);
digitalWrite(TFT_CS, LOW);
SPI1.transfer(cmd);
digitalWrite(TFT_CS, HIGH);

Data:

digitalWrite(TFT_DC, HIGH);
digitalWrite(TFT_CS, LOW);
SPI1.transfer(data);
digitalWrite(TFT_CS, HIGH);

Remove unnecessary 10 microsecond delays between individual GPIO/SPI operations.

BOARD POWER AND BACKLIGHT

The Waveshare board uses BAT_EN on GPIO26.

Initialize:

pinMode(26, OUTPUT);
digitalWrite(26, HIGH);

Backlight:

pinMode(TFT_BL, OUTPUT);
digitalWrite(TFT_BL, LOW);

After LCD initialization:

analogWrite(TFT_BL, 255);

Do not use GPIO16 for touch reset.

TOUCH

Do not redesign the touch subsystem.

Only correct the Waveshare GPIO mapping:

SDA = 6
SCL = 7
RST = 17
INT = 18

Touch initialization must never toggle GPIO16.

Do not migrate Wire to Wire1 unless this is absolutely required for an existing functional dependency. Do not turn this task into a touch/IMU refactoring.

DISPLAY WINDOW

The LCD is 320x240 in horizontal orientation.

Window commands must follow the working demo:

0x2A -> Xstart, Xend-1
0x2B -> Ystart, Yend-1
0x2C

Preserve existing drawing functions when they are already compatible.

VISUAL TEST

Inspect service_box/src/main.cpp.

If Hardware.init() only clears the LCD to black and nothing is subsequently rendered, add a temporary obvious visual test after initialization, such as a white fill or the existing renderStartPage().

The purpose is to verify physically that the LCD works.

Do not implement the final Service Box UI as part of this task.

DOCUMENTATION

Update service_box/hardware_map.md so that it matches the real Waveshare hardware.

LCD:
CS   = GP13
DC   = GP14
RST  = GP15
BL   = GP16
MOSI = GP11
MISO = GP12
SCLK = GP10

Touch:
SDA  = GP6
SCL  = GP7
RST  = GP17
INT  = GP18

FILES TO INSPECT BEFORE MODIFYING

service_box/src/hal/HardwareWaveshare.cpp
service_box/src/hal/HardwareInterface.h
service_box/src/main.cpp
service_box/hardware_map.md
service_box/platformio.ini

Compare the implementation against:

demo-rp2030/DEV_Config.h
demo-rp2030/DEV_Config.cpp
demo-rp2030/LCD_2in8.cpp
demo-rp2030/RP2350-Touch-LCD-2.8.ino

VALIDATION

After making the changes:

1. Build the waveshare_rp2350 target.
2. Verify there are no GPIO conflicts between LCD and touch.
3. Verify the LCD remains on SPI1.
4. Verify TFT_BL is GPIO16.
5. Verify touch reset is GPIO17.
6. Verify touch interrupt is GPIO18.
7. Verify TFT_MISO is GPIO12.
8. Report every modified file.
9. Explain briefly what was changed.
10. Report the build result.

If the build fails, fix only errors caused by this task.

Do not make unrelated changes.