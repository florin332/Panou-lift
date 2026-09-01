#include <Arduino.h>
#include "hal/HardwareInterface.h"

// Removed old local UI class includes since rendering is now fully handled via unified HAL bridges
// // #include "ui/StartupScreen.h"
// // StartupScreen* startupScreen = nullptr;

UiPage currentPage = PAGE_START;
bool refreshPageNeeded = true;
unsigned long sdCheckTimer = 0;

void setup() {
    // Start local monitoring port for PC verification debugging
    Serial.begin(115200);
    delay(500);

    // Initialize decoupled framework structures matching active compilation target (SPI, I2C, LCD, Touch)
    Hardware.init();

    Serial.println("\n==============================================");
    Serial.print("Host System Application Core Active On: ");
    
    // Using the native PlatformIO framework injected macro directly
    #if defined(BOARD_NAME)
        Serial.println(BOARD_NAME);
    #else
        Serial.println("RP2040/RP2350 Board");
    #endif
    
    Serial.println("==============================================");
}

void loop() {
    // Process background USB pipelines asynchronously (runs timeouts, serial parsing, and retransmission retries)
    Hardware.updateCommEngine();
    
    // Poll hardware interface input wrappers (captures raw touches from XPT2046 or CST328 and scales coordinates)
    Hardware.updateTouch();

    int currentX = Hardware.getTouchX();
    int currentY = Hardware.getTouchY();
    bool isScreenActive = Hardware.isScreenTouched();

    // ========================================================================
    // UI APPLICATION STATE MACHINE ROUTING
    // ========================================================================
    if (currentPage == PAGE_START) {
        
        // 1. Initial Frame Base Canvas Render
        if (refreshPageNeeded) {
            // Trigger base page outline rendering block inside the active HAL implementation
            Hardware.renderStartPage(true, "SYSTEM STANDBY", 0xFCE0); // 0xFCE0 = Yellow
            refreshPageNeeded = false;
        }

        // 2. Direct Evaluation of Local Touch Grid Interaction Bounds
        // Button bounds matching layout spec: X[50 to 190], Y[190 to 235]
        if (isScreenActive && currentX >= 50 && currentX <= 190 && currentY >= 190 && currentY <= 235) {
            if (Hardware.getCommState() == COMM_IDLE) {
                Serial.println("[UI Touch Event] START Pressed. Sending mb_in initialization token...");
                
                // Update local status zone text directly without wiping the entire screen elements
                Hardware.renderStartPage(false, "CONNECTING...", 0x07FF); // 0x07FF = Cyan
                
                // Dispatch raw master request string over virtual serial pipeline link
                Hardware.sendCommand("mb_in");
            }
        }

        // 3. Evaluate Asynchronous Protocol Communication States
        switch (Hardware.getCommState()) {
            case COMM_TRANSACTION_SUCCESS:
                // Verify if client returned valid authorization response string
                if (Hardware.getLastResponse() == "ACK SRV ENTER") {
                    Serial.println("[Comm Handshake Success] Handshake confirmed by client panel.");
                    Hardware.renderStartPage(false, "CONNECTED!", 0x07E0); // 0x07E0 = Bright Green
                    delay(800); // Visual hold confirmation window for the technician
                    
                    // Route state machine execution context into the main diagnostics dashboard view
                    currentPage = PAGE_DASHBOARD;
                    refreshPageNeeded = true;
                }
                Hardware.clearCommState();
                break;

            case COMM_CRITICAL_FAILURE:
                // 150ms transmission timeout reached or max retry budget exhausted
                Serial.println("[Comm Handshake Error] Failed to establish communication link with panel.");
                Hardware.renderStartPage(false, "COMM LINK FAULT!", 0xF800); // 0xF800 = Red
                Hardware.clearCommState();
                break;

            default:
                break;
        }

        // 4. Periodic Storage Updates (Poll metrics background loop every 5000ms)
        if (millis() - sdCheckTimer > 5000) {
            if (Hardware.getCommState() == COMM_IDLE) {
                // Background runtime execution hooks can pull real hardware metrics here
                Hardware.renderStartPage(false, "SYSTEM READY", 0xFFFF); // White
            }
            sdCheckTimer = millis();
        }
    } 
    else if (currentPage == PAGE_DASHBOARD) {
        if (refreshPageNeeded) {
            Serial.println("[UI Navigation] Dashboard Screen Loaded Successfully.");
            refreshPageNeeded = false;
        }
    }
}
