#ifndef HARDWARE_INTERFACE_H
#define HARDWARE_INTERFACE_H

#include <Arduino.h>

// ============================================================================
// IDENTIFICARE TEXT HARDWARE (Folosește flag-urile din platformio.ini)
// ============================================================================
#ifdef SERVICEBOX_MARBLE
    #define MY_BOARD_NAME "GroundStudio Marble Pico"
#elif defined(SERVICEBOX_WAVESHARE)
    #define MY_BOARD_NAME "Waveshare RP2350 Touch LCD"
#else
    #define MY_BOARD_NAME "Necunoscut"
#endif

// ============================================================================
// CONFIGURAȚIE CORESPUNZĂTOARE PINILOR
// ============================================================================
#ifdef SERVICEBOX_MARBLE
    // JONGLEZ CU PINII PE MARBLE PICO (SPI1 pentru display/touch, SPI0 pentru SD)
    #define TFT_CS   9   
    #define TFT_DC   1   
    #define TFT_RST  2   
    #define TFT_BL   13  
    
    #define TFT_MOSI 11  
    #define TFT_SCK  10  
    #define TOUCH_CS 8   
    #define TOUCH_MISO 12  

    #define SD_CS   5   
    #define SD_MOSI 3   
    #define SD_MISO 4   
    #define SD_SCLK 6   

    #define SERIAL_TX_PIN 20
    #define SERIAL_RX_PIN 21
    #define DISPLAY_UART  Serial2

#elif defined(SERVICEBOX_WAVESHARE)
    // DISPLAY WAVESHARE ST7789T3
    #define TFT_CS   13
    #define TFT_DC   14
    #define TFT_RST  15
    #define TFT_BL   22
    #define TFT_MOSI 11
    #define TFT_SCLK 10
    #define TFT_MISO -1

    // TOUCH CAPACITIV CST328 (Magistrală I2C0)
    #define TOUCH_SDA 6
    #define TOUCH_SCL 7
    #define TOUCH_INT 17
    #define TOUCH_RST 16

    // IMU QMI8658
    #define IMU_SDA  6
    #define IMU_SCL  7
    #define IMU_INT1 23
    #define IMU_INT2 24

    // MICROSD (SPI1)
    #define SD_CS   8
    #define SD_MOSI 11
    #define SD_MISO 12
    #define SD_SCLK 10

    #define SERIAL_TX_PIN 4
    #define SERIAL_RX_PIN 5
    #define DISPLAY_UART  Serial2
#endif

// ============================================================================
// INTERFAȚĂ HARDWARE (HAL)
// ============================================================================
class HardwareInterface {
public:
    virtual void init() = 0;
    virtual void updateTouch() = 0;
    virtual bool isScreenTouched() = 0;
    virtual int getTouchX() = 0;
    virtual int getTouchY() = 0;
    virtual void sendProtocolData(uint8_t* data, uint16_t len) = 0;
    virtual ~HardwareInterface() {}
};

extern HardwareInterface& Hardware;

#endif // HARDWARE_INTERFACE_H

