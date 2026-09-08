// ============================================================================
// Service Box - branch graphic_ui
//
// Acest branch contine dezvoltarea meniului / graficii UI.
// Implementarea HAL a fost eliminata; accesul la hardware este conditionat
// pe target:
//
//   SERVICEBOX_WAVESHARE = hardware REAL
//     LCD ST7789T3 (SPI1) + touch CST328 (I2C1) + calibrare axe in EEPROM,
//     portate din wv_2350_lcd / branch tester_port (secțiuni [PORTABLE],
//     validate pe hardware: "tested-ok").
//
//   SERVICEBOX_MARBLE = PROVIZORIU simulator Serial (src/sim/)
//     pana la integrarea driverelor Marble de pe branch-ul hardware dedicat.
//
// Structura paginilor implementate:
//   PAGE_START      - ecranul de pornire (titlu + buton START + status SD)
//   PAGE_DASHBOARD  - placeholder, in dezvoltare
// ============================================================================

#include <Arduino.h>

#if defined(SERVICEBOX_WAVESHARE)

// ============================================================
// WAVESHARE RP2350 - HARDWARE REAL
// Port din wv_2350_lcd / tester_port (commit "tested-ok")
// ============================================================

#include <SPI.h>
#include <EEPROM.h>
#include <string.h>

#include "hardware/spi.h"
#include "hardware/gpio.h"

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

#include "bsp/bsp_i2c.h"
#include "bsp/bsp_cst328.h"

// Forward declarations
bool readRawPoint(int32_t &rx, int32_t &ry);

// ---------------- LCD pins [PORTABLE - tester_port] ----------------
// Coincid cu hardware_map.md §2.1 (LCD ST7789T3)
#define LCD_SCLK   10
#define LCD_MOSI   11
#define LCD_MISO   12
#define LCD_CS     13
#define LCD_DC     14
#define LCD_RST    15
#define LCD_BL     16

// ---------------- LCD geometry [PORTABLE - tester_port] ----------------
#define LCD_WIDTH  240
#define LCD_HEIGHT 320

// ---------------- Touch CST328 [PORTABLE - tester_port] ----------------
// Coincide cu hardware_map.md §2.2 (I2C GP6/GP7, vezi bsp_i2c.h)
#define TOUCH_WIDTH  LCD_WIDTH
#define TOUCH_HEIGHT LCD_HEIGHT

// ---------------- Buton recalibrare hardware [PORTABLE - tester_port] ------
// Tinut apasat 2 secunde -> porneste fluxul de calibrare axe
// NOTA: GP29 nu figureaza in hardware_map.md - vezi todo.md / Agent Proposals
#define RECALIB_BUTTON 29

// ---------------- Calibrare touch [PORTABLE - tester_port] ----------------
// Masina de stari persistenta (fiecare pas: salvare + reboot):
//   stage 3 = calibrat complet (mod normal)
//   stage 1 = asteapta linia orizontala (invX/swapXY)
//   stage 2 = asteapta linia verticala (invY)
// recalibrating = 1 cat timp fluxul de recalibrare e activ
#define CALIB_MAGIC  0x43535433  // "CST3"
#define CALIB_VER    2

struct CalibFlags {
    uint32_t magic;
    uint16_t version;
    uint8_t  swapXY;
    uint8_t  invX;
    uint8_t  invY;
    uint8_t  stage;          // 1 = ORIENT_X, 2 = ORIENT_Y, 3 = gata
    uint8_t  recalibrating;  // 1 = flux recalibrare in desfasurare
};

static CalibFlags calib;

// Acces rapid la flag-uri
#define calib_swapXY ((bool)calib.swapXY)
#define calib_invX   ((bool)calib.invX)
#define calib_invY   ((bool)calib.invY)

void saveCalib()
{
    calib.magic   = CALIB_MAGIC;
    calib.version = CALIB_VER;
    EEPROM.put(0, calib);
    EEPROM.commit();
}

void loadCalib()
{
    EEPROM.begin(32);
    EEPROM.get(0, calib);

    if (calib.magic != CALIB_MAGIC || calib.version != CALIB_VER)
    {
        // Prima pornire: default validat pe hardware (tester_port), calibrat complet
        calib.swapXY = 0;
        calib.invX   = 1;
        calib.invY   = 1;
        calib.stage  = 3;
        calib.recalibrating = 0;
        saveCalib();
    }
}

// Stergere setari vechi + armare flux recalibrare (pasul 1 urmeaza)
void resetCalibForRecalibration()
{
    calib.swapXY = 0;
    calib.invX   = 0;
    calib.invY   = 0;
    calib.stage  = 1;
    calib.recalibrating = 1;
    saveCalib();
}

// ---------------- Obiect display (Adafruit) [PORTABLE - tester_port] -------
Adafruit_ST7789 display(&SPI1, LCD_CS, LCD_DC, LCD_RST);

// ---------------- Stare touch [PORTABLE - tester_port] ----------------
static bsp_cst328_info_t cst328_info;
static bsp_cst328_data_t touch_data;

static bool     touch_pressed  = false;
static uint16_t touch_x        = 0;
static uint16_t touch_y        = 0;
static uint16_t touch_pressure = 0;

// Coordonate brute (inainte de corectii swap/inv)
static uint16_t raw_x = 0;
static uint16_t raw_y = 0;

// Hold la eliberare: pastram coordonatele cateva cadre pentru a absorbi
// zerourile spre contrand atingerii
static uint8_t release_hold = 0;
#define RELEASE_HOLD_FRAMES 4

// ============================================================
// Corectie coordonate touch [PORTABLE - tester_port]
// Aplica swapXY / invX / invY pe coordonatele brute CST328.
// ============================================================
void applyTouchCorrection(uint16_t rx, uint16_t ry, uint16_t &ox, uint16_t &oy)
{
    int32_t cx = rx;
    int32_t cy = ry;

    if (calib_swapXY)
    {
        int32_t t = cx;
        cx = cy;
        cy = t;
    }

    if (calib_invX)
    {
        cx = (TOUCH_WIDTH - 1) - cx;
    }

    if (calib_invY)
    {
        cy = (TOUCH_HEIGHT - 1) - cy;
    }

    ox = constrain(cx, 0, LCD_WIDTH - 1);
    oy = constrain(cy, 0, LCD_HEIGHT - 1);
}

// ============================================================
// Reboot (dupa salvarea calibrarii) [PORTABLE - tester_port]
// ============================================================
void forceReboot()
{
    delay(1500);
    rp2040.reboot();
}

// ============================================================
// Citire punct touch brut [PORTABLE - tester_port]
// Polling direct pe I2C - NU depinde de ISR/read_data_done.
// (ISR-ul BSP consuma flag-ul dupa prima citire si nu rearmeaza
//  pana la o noua apasare - ar bloca measureSwipe().)
// ============================================================
bool readRawPoint(int32_t &rx, int32_t &ry)
{
    uint8_t buf[6];

    // Registrul FLAG_AND_NUM (0xd005): bitii 0-3 = numar puncte
    bsp_i2c_read_reg16(CST328_DEVICE_ADDR, CST328_TOUCH_FLAG_AND_NUM, &buf[0], 1);
    uint8_t points = buf[0] & 0x0F;

    if (points == 0)
        return false;

    // Citim primul punct: ID + XH + YH + XLYL + PRESSURE (5 bytes de la 0xd000)
    bsp_i2c_read_reg16(CST328_DEVICE_ADDR, CST328_1ST_TOUCH_ID, &buf[0], 5);

    // ID trebuie sa fie 0x06 in nibble-ul jos
    if ((buf[0] & 0x0F) != 0x06)
        return false;

    rx = ((uint16_t)buf[1] << 4) | ((buf[3] & 0xF0) >> 4);
    ry = ((uint16_t)buf[2] << 4) | (buf[3] & 0x0F);

    return true;
}

// ============================================================
// Masurare linie trasata [PORTABLE - tester_port]
// Start = prima atingere, end = ultimul punct inainte de eliberare.
// Returneaza false daca miscarea e prea scurta (< 60 px).
// ============================================================
bool measureSwipe(int32_t &diffX, int32_t &diffY)
{
    // Asteapta atingerea (timeout 30 s)
    int32_t start_rx = 0, start_ry = 0;
    unsigned long timeout = millis() + 30000;

    while (!readRawPoint(start_rx, start_ry))
    {
        if (millis() > timeout)
            return false;
        delay(10);
    }

    // Urmeaza degetul pana la eliberare; retine ultimul punct
    int32_t end_rx = start_rx;
    int32_t end_ry = start_ry;
    int32_t rx, ry;

    while (readRawPoint(rx, ry))
    {
        end_rx = rx;
        end_ry = ry;
        delay(10);
    }

    diffX = end_rx - start_rx;
    diffY = end_ry - start_ry;

    // Respinge atingeri simple (fara miscare reala)
    if (abs(diffX) < 60 && abs(diffY) < 60)
        return false;

    return true;
}

// ============================================================
// Asteapta eliberarea degetului de pe ecran [PORTABLE - tester_port]
// ============================================================
void waitTouchRelease()
{
    while (true)
    {
        bsp_cst328_read();
        if (!bsp_cst328_get_touch_data(&touch_data) || touch_data.points == 0)
            break;
        delay(10);
    }
}

// ============================================================
// Ecrane calibrare axe (minime - doar instructiuni pentru tehnician)
// ============================================================
void renderCalibOrientX()
{
    display.fillScreen(ST77XX_BLACK);

    display.setCursor(10, 12);
    display.setTextColor(ST77XX_CYAN);
    display.setTextSize(2);
    display.println("LINIA 1/2: ORIZONTALA");

    display.setTextSize(1);
    display.setTextColor(ST77XX_WHITE);
    display.setCursor(10, 45);
    display.println("Trageti o linie dreapta continuu");
    display.setCursor(10, 58);
    display.println("de la STANGA la DREAPTA pe ecran.");

    // Sageata orizontala
    display.drawLine(40, 150, 180, 150, ST77XX_YELLOW);
    display.drawLine(40, 151, 180, 151, ST77XX_YELLOW);
    display.drawLine(160, 130, 180, 151, ST77XX_YELLOW);
    display.drawLine(160, 131, 180, 152, ST77XX_YELLOW);
    display.drawLine(160, 171, 180, 151, ST77XX_YELLOW);
    display.drawLine(160, 170, 180, 150, ST77XX_YELLOW);
}

void renderCalibOrientY()
{
    display.fillScreen(ST77XX_BLACK);

    display.setCursor(10, 12);
    display.setTextColor(ST77XX_CYAN);
    display.setTextSize(2);
    display.println("LINIA 2/2: VERTICALA");

    display.setTextSize(1);
    display.setTextColor(ST77XX_WHITE);
    display.setCursor(10, 45);
    display.println("Trageti o linie dreapta continuu");
    display.setCursor(10, 58);
    display.println("de SUS in JOS pe ecran.");

    // Sageata verticala
    display.drawLine(120, 100, 120, 220, ST77XX_YELLOW);
    display.drawLine(121, 100, 121, 220, ST77XX_YELLOW);
    display.drawLine(100, 200, 120, 220, ST77XX_YELLOW);
    display.drawLine(101, 200, 121, 220, ST77XX_YELLOW);
    display.drawLine(140, 200, 120, 220, ST77XX_YELLOW);
    display.drawLine(139, 200, 119, 220, ST77XX_YELLOW);
}

void renderCalibStepSaved(const char *msg)
{
    display.fillScreen(ST77XX_BLACK);

    display.setCursor(10, 120);
    display.setTextColor(ST77XX_CYAN);
    display.setTextSize(2);
    display.println(msg);

    display.setCursor(10, 150);
    display.setTextSize(1);
    display.setTextColor(ST77XX_GREEN);
    display.println("Salvat. Repornire...");
}

void renderCalibResult()
{
    display.fillScreen(ST77XX_BLACK);

    display.setCursor(10, 12);
    display.setTextColor(ST77XX_CYAN);
    display.setTextSize(2);
    display.println("AXE DETECTATE");

    display.setTextSize(1);
    display.setTextColor(ST77XX_WHITE);

    display.setCursor(10, 50);
    display.print("SwapXY = ");
    display.println(calib.swapXY);
    display.setCursor(10, 65);
    display.print("InvX   = ");
    display.println(calib.invX);
    display.setCursor(10, 80);
    display.print("InvY   = ");
    display.println(calib.invY);

    display.setCursor(10, 110);
    display.setTextColor(ST77XX_GREEN);
    display.println("Salvat in flash.");
    display.setCursor(10, 125);
    display.println("Repornire...");
}

void renderSwipeTooShort()
{
    display.fillRect(0, 200, LCD_WIDTH, 30, ST77XX_BLACK);
    display.setCursor(10, 208);
    display.setTextSize(1);
    display.setTextColor(ST77XX_RED);
    display.println("Miscare prea scurta. Reincercati.");
}

// ============================================================
// Pornire flux calibrare axe (buton software sau hardware)
// [PORTABLE - tester_port]
// ============================================================
void startAxisCalibration()
{
    resetCalibForRecalibration();

    display.fillScreen(ST77XX_BLACK);
    display.setCursor(10, 140);
    display.setTextSize(2);
    display.setTextColor(ST77XX_CYAN);
    display.println("RECALIBRARE...");

    forceReboot(); // nu revine - reboot in stage 1
}

// ============================================================
// Buton hardware recalibrare (GP29) [PORTABLE - tester_port]
// Tinut apasat 2 secunde -> porneste fluxul de calibrare.
// Apelat in loop() inainte de logica aplicatiei.
// ============================================================
void checkRecalibButton()
{
    // Doar in modul normal (calibrare completa)
    if (calib.stage != 3)
        return;

    if (digitalRead(RECALIB_BUTTON) == HIGH)
        return;

    unsigned long pressStart = millis();
    bool validHold = true;

    while (millis() - pressStart < 2000)
    {
        if (digitalRead(RECALIB_BUTTON) == HIGH)
        {
            validHold = false;
            break;
        }
        delay(20);
    }

    if (validHold)
    {
        display.fillScreen(0x7800);  // MAROON
        display.setCursor(15, 120);
        display.setTextSize(2);
        display.setTextColor(ST77XX_WHITE);
        display.print("ELIBERATI BUTONUL...");

        while (digitalRead(RECALIB_BUTTON) == LOW)
        {
            delay(10);
        }
        delay(100);

        startAxisCalibration();
    }
}

// ============================================================
// Input unificat - Waveshare (touch real CST328)
// ============================================================
void inputUpdate()
{
    bsp_cst328_read();

    bool got_touch = bsp_cst328_get_touch_data(&touch_data) && touch_data.points > 0;

    if (got_touch)
    {
        raw_x = touch_data.coords[0].x;
        raw_y = touch_data.coords[0].y;
        touch_pressure = touch_data.coords[0].pressure;
        touch_pressed = true;
        release_hold = RELEASE_HOLD_FRAMES;

        applyTouchCorrection(raw_x, raw_y, touch_x, touch_y);
    }
    else if (release_hold > 0)
    {
        // Cadru fara date - poate fi glitch -> mentinem starea
        release_hold--;
    }
    else
    {
        touch_pressed = false;
    }
}

#elif defined(SERVICEBOX_MARBLE)

// ============================================================
// MARBLE PICO - PROVIZORIU simulator Serial
// Driverele reale Marble (ILI9341 + XPT2046) se integreaza
// de pe branch-ul hardware dedicat. Vezi src/sim/.
// ============================================================

#include "sim/MenuSimulator.h"

#define LCD_WIDTH  SIM_SCREEN_WIDTH   // PROVIZORIU
#define LCD_HEIGHT SIM_SCREEN_HEIGHT  // PROVIZORIU

// Input unificat - Marble (PROVIZORIU: touch simulat prin Serial)
void inputUpdate()
{
    simTouch.update(); // PROVIZORIU
}

// PROVIZORIU - no-op pe Marble; pe Waveshare asteapta eliberarea degetului
void waitTouchRelease()
{
}

#endif // target selection

#include "ui/StartupScreen.h"

// ============================================================================
// Pagini UI (comune, independente de hardware)
// ============================================================================
enum UiPage {
    PAGE_START,
    PAGE_DASHBOARD
};

// ============================================================================
// Instante display unificate
// ============================================================================
#if defined(SERVICEBOX_WAVESHARE)
Adafruit_GFX& activeDisplay = display;
#else
Adafruit_GFX& activeDisplay = simDisplay; // PROVIZORIU - simulator pe Marble
#endif

// Pagina START (implementata anterior, decuplata de HAL)
StartupScreen startupScreen(&activeDisplay);

// ============================================================================
// Pagina DASHBOARD (placeholder - doar desen, fara functionalitate)
// ============================================================================
static void renderDashboard(Adafruit_GFX* tft) {
    tft->fillScreen(0x0000); // Black

    tft->setTextColor(0xFFFF); // White
    tft->setTextSize(3);
    tft->setCursor(20, 45);
    tft->print("DASHBOARD");

    tft->setTextColor(0x7BEF); // Gray
    tft->setTextSize(1);
    tft->setCursor(20, 95);
    tft->print("placeholder - in dezvoltare");

    // Buton BACK (placeholder pentru navigare inapoi la START)
    // Geometry: X[50..190], Y[270..305]
    tft->fillRoundRect(50, 270, 140, 35, 6, 0x1967);  // Deep Blue
    tft->drawRoundRect(50, 270, 140, 35, 6, 0xFFFF);  // White outline
    tft->setTextColor(0xFFFF);
    tft->setTextSize(2);
    tft->setCursor(50 + 45, 270 + 10);
    tft->print("BACK");
}

// Verifica daca touch-ul curent loveste butonul BACK din dashboard
static bool dashboardBackHit(int x, int y, bool touched) {
    return touched && x >= 50 && x <= 190 && y >= 270 && y <= 305;
}

// ============================================================================
// Stare aplicatie
// ============================================================================
UiPage currentPage = PAGE_START;
bool refreshPageNeeded = true;

// ============================================================================
// Arduino setup
// ============================================================================
void setup() {
    Serial.begin(115200);
    delay(500);

#if defined(SERVICEBOX_WAVESHARE)

    // --------------------------------------------------------
    // WAVESHARE: initializare hardware real [PORTABLE - tester_port]
    // --------------------------------------------------------

    // Incarcare flag-uri calibrare din flash (sau default validat)
    loadCalib();

    // Backlight pornit
    pinMode(LCD_BL, OUTPUT);
    digitalWrite(LCD_BL, HIGH);

    // Buton hardware recalibrare (GP29, activ LOW)
    pinMode(RECALIB_BUTTON, INPUT_PULLUP);

    // SPI1 pe pinii LCD (SCLK=10, MOSI=11, MISO=12)
    SPI1.setSCK(LCD_SCLK);
    SPI1.setTX(LCD_MOSI);
    SPI1.setRX(LCD_MISO);

    // Initializare ST7789T3
    // SPI_MODE3 = CPOL_1 / CPHA_1, validat pe hardware (tester_port)
    display.init(LCD_WIDTH, LCD_HEIGHT, SPI_MODE3);
    display.setSPISpeed(80000000);
    display.invertDisplay(true);   // echivalentul comenzii 0x21 - obligatoriu ST7789T3
    display.setRotation(0);

    // Initializare touch CST328 (I2C1: SDA=GP6, SCL=GP7, 400 kHz)
    bsp_i2c_init();

    cst328_info.width = LCD_WIDTH;
    cst328_info.height = LCD_HEIGHT;
    cst328_info.rotation = 0;

    bsp_cst328_init(&cst328_info);

    Serial.println();
    Serial.println("==============================================");
    Serial.println("Service Box - graphic_ui / WAVESHARE RP2350");
    Serial.print("Calibrare: stage=");
    Serial.print(calib.stage);
    Serial.print(" swapXY=");
    Serial.print(calib.swapXY);
    Serial.print(" invX=");
    Serial.print(calib.invX);
    Serial.print(" invY=");
    Serial.println(calib.invY);
    Serial.println("==============================================");

    // Afisare pagina conform stadiului de calibrare
    if (calib.stage == 1)
    {
        renderCalibOrientX();
    }
    else if (calib.stage == 2)
    {
        renderCalibOrientY();
    }
    else
    {
        startupScreen.init();
    }

#elif defined(SERVICEBOX_MARBLE)

    // --------------------------------------------------------
    // MARBLE: PROVIZORIU simulator (pana la integrarea driverelor reale)
    // --------------------------------------------------------
    Serial.println();
    Serial.println("==============================================");
    Serial.println("Service Box - graphic_ui / MARBLE PICO");
    Serial.println("Display/touch SIMULATE - PROVIZORIU");
    Serial.println("Touch simulat prin Serial:");
    Serial.println("  t <x> <y>  = apasare la coordonate");
    Serial.println("  up         = eliberare");
    Serial.println("==============================================");

    startupScreen.init();

#endif
}

// ============================================================================
// Loop principal
// ============================================================================
void loop() {

#if defined(SERVICEBOX_WAVESHARE)

    // --------------------------------------------------------
    // WAVESHARE: flux calibrare axe (stage 1/2) - are prioritate
    // --------------------------------------------------------
    if (calib.stage == 1)
    {
        // PAS 1/2: linie orizontala STANGA -> DREAPTA
        int32_t diffX, diffY;

        if (measureSwipe(diffX, diffY))
        {
            // swapXY: miscarea dominanta trebuie sa fie pe axa X
            calib.swapXY = (abs(diffX) >= abs(diffY)) ? 0 : 1;

            // invX: miscarea pe X (dupa swap) trebuie sa creasca
            int32_t dominantX = calib.swapXY ? diffY : diffX;
            calib.invX = (dominantX > 0) ? 0 : 1;

            calib.stage = 2;
            saveCalib();

            renderCalibStepSaved("PAS 1/2 OK");
            forceReboot();  // nu revine - reboot in stage 2
        }
        else
        {
            renderSwipeTooShort();
        }
        return;
    }

    if (calib.stage == 2)
    {
        // PAS 2/2: linie verticala SUS -> JOS
        int32_t diffX, diffY;

        if (measureSwipe(diffX, diffY))
        {
            // invY: miscarea pe Y (dupa swap) trebuie sa creasca
            int32_t dominantY = calib.swapXY ? diffX : diffY;
            calib.invY = (dominantY > 0) ? 0 : 1;

            calib.stage = 3;
            calib.recalibrating = 0;
            saveCalib();

            renderCalibResult();
            forceReboot();  // nu revine - reboot in mod normal
        }
        else
        {
            renderSwipeTooShort();
        }
        return;
    }

    // stage == 3: mod normal - buton recalibrare are prioritate
    checkRecalibButton();

#endif // SERVICEBOX_WAVESHARE

    // --------------------------------------------------------
    // Input unificat (touch real pe Waveshare / simulat pe Marble)
    // --------------------------------------------------------
    inputUpdate();

#if defined(SERVICEBOX_WAVESHARE)
    int  currentX       = touch_x;
    int  currentY       = touch_y;
    bool isScreenActive = touch_pressed;
#else
    int  currentX       = simTouch.getX();       // PROVIZORIU - Marble
    int  currentY       = simTouch.getY();       // PROVIZORIU - Marble
    bool isScreenActive = simTouch.isTouched();  // PROVIZORIU - Marble
#endif

    // ========================================================================
    // UI APPLICATION STATE MACHINE ROUTING
    // ========================================================================
    if (currentPage == PAGE_START) {

        // 1. Initial Frame Base Canvas Render
        if (refreshPageNeeded) {
            startupScreen.render(true);
            refreshPageNeeded = false;
        }

        // 2. Evaluare interactiune buton START (press/release cu feedback vizual)
        if (startupScreen.update(currentX, currentY, isScreenActive)) {
            Serial.println("[UI Navigation] START confirmat -> PAGE_DASHBOARD");
            currentPage = PAGE_DASHBOARD;
            refreshPageNeeded = true;
        }
    }
    else if (currentPage == PAGE_DASHBOARD) {

        if (refreshPageNeeded) {
            renderDashboard(&activeDisplay);
            refreshPageNeeded = false;
            Serial.println("[UI Navigation] Dashboard afisat.");
        }

        // Navigare inapoi la START prin butonul BACK
        if (dashboardBackHit(currentX, currentY, isScreenActive)) {
            Serial.println("[UI Navigation] BACK -> PAGE_START");
            waitTouchRelease();  // Waveshare: evita re-trigger; Marble: no-op
            currentPage = PAGE_START;
            refreshPageNeeded = true;
        }
    }
}
