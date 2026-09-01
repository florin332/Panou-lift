#ifndef HARDWARE_INTERFACE_H
#define HARDWARE_INTERFACE_H

#include <Arduino.h>

// Extended UI Pages tracking state
enum UiPage {
    PAGE_START,
    PAGE_DASHBOARD,
    PAGE_TEST_SUITE
};

enum CommState {
    COMM_IDLE,
    COMM_AWAITING_REPLY,
    COMM_TIMEOUT_ERROR,
    COMM_TRANSACTION_SUCCESS,
    COMM_CRITICAL_FAILURE
};

// ============================================================================
// GLOBAL FALLBACK LOGIC FOR VIRTUAL PORT PIPELINE
// ============================================================================
#ifndef DISPLAY_SERIAL
    #define DISPLAY_SERIAL Serial // Forces native USB CDC fallback globally
#endif

// ============================================================================
// HARDWARE PIN CONSTRAINTS MAPPINGS
// ============================================================================
#ifdef SERVICEBOX_MARBLE
    #define TFT_CS      9   
    #define TFT_DC      1   
    #define TFT_RST     2   
    #define TFT_BL      13  
    #define TFT_MOSI    11  
    #define TFT_SCK     10  
    #define TOUCH_CS    8   
    #define TOUCH_MISO  12  
    #define SD_CS       5   
    #define SD_MOSI     3   
    #define SD_MISO     4   
    #define SD_SCLK     6   
#elif defined(SERVICEBOX_WAVESHARE)
    #define TFT_CS      13
    #define TFT_DC      14
    #define TFT_RST     15
    #define TFT_BL      16
    #define TFT_MOSI    11
    #define TFT_SCLK    10
    #define TOUCH_SDA   6
    #define TOUCH_SCL   7
    #define TOUCH_INT   17
    #define TOUCH_RST   16
    #define IMU_SDA     6
    #define IMU_SCL     7
    #define IMU_INT1    23
    #define IMU_INT2    24
    #define SD_CS       8
    #define SD_MOSI     11
    #define SD_MISO     12
    #define SD_SCLK     10
#endif

// ============================================================================
// UNIVERSAL HARDWARE INTERACTION INTERFACE (HAL)
// ============================================================================
class HardwareInterface {
public:
    virtual void init() = 0;
    virtual void updateTouch() = 0;
    virtual bool isScreenTouched() = 0;
    virtual int getTouchX() = 0;
    virtual int getTouchY() = 0;
    virtual void sendProtocolData(uint8_t* data, uint16_t len) = 0;
    
    // Non-blocking Command Sequencer abstract hooks
    virtual void sendCommand(const char* cmdString) = 0;
    virtual void updateCommEngine() = 0;
    virtual CommState getCommState() = 0;
    virtual String getLastResponse() = 0;
    virtual void clearCommState() = 0;

    // UI Layout rendering abstraction bridges
    virtual void renderStartPage(bool forceRedraw, const char* statusMsg, uint16_t statusColor) = 0;

    virtual ~HardwareInterface() {}
};

extern HardwareInterface& Hardware;

#endif // HARDWARE_INTERFACE_H




