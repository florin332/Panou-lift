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
//   SERVICEBOX_MARBLE = hardware REAL
//     LCD ILI9341 (SPI1) + touch XPT2046 (SPI1, magistrala comuna) +
//     calibrare touch in EEPROM, portate din tester-touch-lcd /
//     branch corectii_cod (doar Model 1 BlueTab, sectiuni [PRELUARE]).
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
// MARBLE PICO - HARDWARE REAL (Model 1 - Blue Tab)
// Port din tester-touch-lcd / corectii_cod, sectiunile [PRELUARE].
// HAL-ul testerului (lib/TesterHAL) NU este preluat - doar logica
// Model 1 BlueTab, inline aici. Pinii coincid cu hardware_map.md §1.1/§1.2.
// ============================================================

#include <SPI.h>
#include <EEPROM.h>
#include <string.h>

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>

// ---------------- LCD pins [PRELUARE - Model 1 BlueTab] ----------------
// Coincid cu hardware_map.md §1.1 (LCD ILI9341)
static const int8_t PIN_TFT_CS   = 13;
static const int8_t PIN_TFT_RST  = 14;
static const int8_t PIN_TFT_DC   = 6;
static const int8_t PIN_TFT_MOSI = 11;
static const int8_t PIN_TFT_LED  = 4;
static const int8_t PIN_TFT_SCK  = 10;
static const int8_t PIN_TFT_MISO = 12;

// Touch XPT2046 - coincid cu hardware_map.md §1.2
static const int8_t PIN_TCH_CS   = 9;
static const int8_t PIN_TCH_IRQ  = 8;

// ---------------- LCD geometry ----------------
#define LCD_WIDTH  240
#define LCD_HEIGHT 320

// ---------------- Buton recalibrare [PRELUARE] -----------------------------
// Marble: GP5 (ca in tester-touch-lcd; input de pe placa, INPUT_PULLUP,
// activ LOW). NOTA: GP5 nu figureaza in hardware_map.md §1 - vezi
// todo.md / Agent Proposals (AP-001)
#define RECALIB_BUTTON 5

// ---------------- Praguri touch si calibrare [PRELUARE] --------------------
// NOTA: biblioteca upstream calculeaza z = z1 + 4095 - z2 (unitati ADC raw)
// si aplica intern Z_THRESHOLD=300 in touched(); tester-touch-lcd (validat
// hardware) folosea aceleasi praguri 200/300 cu upstream.
#define Z_TOUCH_MIN   200
#define Z_SAMPLE_MIN  300
#define CALIB_MAGIC   0x544C4344  // calibrare proprie acestui model
#define CALIB_VER     6

// ---------------- Structura calibrare [PRELUARE] ---------------------------
// Impachetata: layout determinist in EEPROM, fara padding
struct __attribute__((packed)) CalibData {
    uint32_t magic;
    uint16_t version;
    int32_t xmin, xmax, ymin, ymax;
    uint8_t swapXY, invX, invY, stage;
};

CalibData calib;

// Masina de stari calibrare: stage 1=orientare X, 4=orientare Y,
// 2=4 puncte, 3=calibrat complet
int calibPointIndex = 0;
int32_t calibX[4], calibY[4];

// Poarta non-blocanta: ignora touch pana la eliberare completa
bool awaitingRelease = false;
unsigned long releaseSince = 0;

// Stare non-blocanta colectare esantioane calibrare
enum CollectState { COLLECT_IDLE, COLLECT_SAMPLING };
CollectState collectState = COLLECT_IDLE;
int32_t calibSamplesX[12], calibSamplesY[12];
int sampleCount = 0;

// Timeout asteptare atingere punct calibrare
unsigned long calibIdleSince = 0;
const unsigned long CALIB_POINT_TIMEOUT_MS = 10000;

// Esantioane detectie directie swipe (etapele de orientare)
int32_t swipeStartX[2], swipeStartY[2];
int32_t swipeEndX[2], swipeEndY[2];
int swipeCount = 0;

// ---------------- Instante drivere (specifice acestui model) ----------------
// Biblioteca upstream PaulStoffregen/XPT2046_Touchscreen (din platformio.ini):
// magistrala SPI se aloca la begin() - begin(SPI1), nu prin constructor.
static Adafruit_ILI9341 tft(&SPI1, PIN_TFT_DC, PIN_TFT_CS, PIN_TFT_RST);
static XPT2046_Touchscreen ts(PIN_TCH_CS, PIN_TCH_IRQ);

Adafruit_GFX &display = tft;

// Valori brute default de calibrare (pana la prima calibrare)
#define DEF_XMIN 3700
#define DEF_XMAX 370
#define DEF_YMIN 300
#define DEF_YMAX 3700

// ============================================================
// Init hardware [PRELUARE - din LcdModel1_BlueTab.cpp]
// ============================================================
void halDisplayInit() {
    pinMode(PIN_TFT_CS, OUTPUT);
    digitalWrite(PIN_TFT_CS, HIGH);

    pinMode(PIN_TFT_LED, OUTPUT);
    digitalWrite(PIN_TFT_LED, HIGH);   // backlight ON

    SPI1.setTX(PIN_TFT_MOSI);
    SPI1.setSCK(PIN_TFT_SCK);
    SPI1.setRX(PIN_TFT_MISO);
    SPI1.begin();

    SPI1.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    SPI1.endTransaction();

    tft.begin();
    tft.invertDisplay(true);   // Model 1 Blue Tab
    tft.setRotation(0);
}

void halTouchInit() {
    pinMode(PIN_TCH_CS, OUTPUT);
    digitalWrite(PIN_TCH_CS, HIGH);

    ts.begin(SPI1);    // upstream: aloca magistrala SPI1 (validat in tester)
    ts.setRotation(0);
}

// Selectie/deselectie chip display (izolare pe magistrala comuna)
void halDisplaySelect()   { digitalWrite(PIN_TFT_CS, LOW); }
void halDisplayDeselect() { digitalWrite(PIN_TFT_CS, HIGH); }

// Citire punct touch, cu izolarea display-ului gestionata intern
struct HalTouchPoint {
    int32_t x, y, z;
    bool pressed;
};

HalTouchPoint halTouchRead() {
    HalTouchPoint hp;

    digitalWrite(PIN_TFT_CS, HIGH);   // izoleaza display-ul
    digitalWrite(PIN_TCH_CS, LOW);

    TS_Point p = ts.getPoint();

    digitalWrite(PIN_TCH_CS, HIGH);

    hp.x = p.x;
    hp.y = p.y;
    hp.z = p.z;
    hp.pressed = (p.z > Z_TOUCH_MIN);
    return hp;
}

bool halTouchIrqActive() {
    return digitalRead(PIN_TCH_IRQ) == LOW;
}

// ============================================================
// Functii de baza touch si mapare [PRELUARE]
// ============================================================
int safeMap(int v, int fl, int fh, int tl, int th) {
    if (fh == fl) {
        return tl;  // Protectie la diviziune cu zero (calibrare corupta)
    }
    return tl + (v - fl) * (th - tl) / (fh - fl);
}

void loadCalib() {
    EEPROM.begin(64);
    EEPROM.get(0, calib);

    if (calib.magic != CALIB_MAGIC || calib.version != CALIB_VER) {
        calib.magic   = 0;
        calib.xmin    = DEF_XMIN;
        calib.xmax    = DEF_XMAX;
        calib.ymin    = DEF_YMIN;
        calib.ymax    = DEF_YMAX;
        calib.swapXY  = 0;
        calib.invX    = 0;
        calib.invY    = 0;
        calib.stage   = 3;
    }
}

bool saveCalib() {
    calib.magic   = CALIB_MAGIC;
    calib.version = CALIB_VER;
    EEPROM.put(0, calib);
    return EEPROM.commit();
}

void forceReboot() {
    delay(500);
    rp2040.reboot();
}

void processRawTouch(int32_t rx, int32_t ry, int32_t &cx, int32_t &cy, bool useConfig) {
    cx = rx;
    cy = ry;

    if (useConfig && calib.swapXY) {
        int32_t t = cx;
        cx = cy;
        cy = t;
    }

    if (useConfig && calib.invX) {
        cx = 4095 - cx;
    }

    if (useConfig && calib.invY) {
        cy = 4095 - cy;
    }
}

void mapTouch(int32_t rx, int32_t ry, int &ox, int &oy) {
    int32_t cx, cy;
    processRawTouch(rx, ry, cx, cy, true);

    ox = constrain(safeMap(cx, calib.xmin, calib.xmax, 0, LCD_WIDTH - 1), 0, LCD_WIDTH - 1);
    oy = constrain(safeMap(cy, calib.ymin, calib.ymax, 0, LCD_HEIGHT - 1), 0, LCD_HEIGHT - 1);
}

// ============================================================
// Functii desenare [PRELUARE - doar drawCrosshair]
// ============================================================
void drawCrosshair(int cx, int cy, uint16_t clr) {
    halDisplaySelect();

    display.drawCircle(cx, cy, 10, clr);
    display.drawLine(cx - 16, cy, cx - 6, cy, clr);
    display.drawLine(cx + 6, cy, cx + 16, cy, clr);
    display.drawLine(cx, cy - 16, cx, cy - 6, clr);
    display.drawLine(cx, cy + 6, cx, cy + 16, clr);
    display.fillCircle(cx, cy, 2, clr);

    halDisplayDeselect();
}

// ============================================================
// Ecrane calibrare [PRELUARE]
// Pozitii crosshair-uri (25/215 X, 85/300 Y) legate de formula
// de extrapolare din finalul etapei 2 - NU se schimba.
// ============================================================
void renderPageOrientX() {
    halDisplaySelect();

    display.fillScreen(0x0000); // BLACK

    display.setCursor(10, 12);
    display.setTextColor(0x07FF); // CYAN
    display.setTextSize(2);
    display.println("LINIA 1/2: ORIZONTALA");

    display.setTextSize(1);
    display.setTextColor(0xFFFF); // WHITE
    display.setCursor(10, 45);
    display.println("Trageti o linie dreapta continuu");
    display.setCursor(10, 58);
    display.println("de la STANGA la DREAPTA pe ecran.");

    display.setTextSize(2);
    display.setTextColor(0xFFE0); // YELLOW

    // Sageata orizontala (dublu contur)
    display.drawLine(40,  150, 180, 150, 0xFFE0);
    display.drawLine(40,  151, 180, 151, 0xFFE0);
    display.drawLine(160, 130, 180, 151, 0xFFE0);
    display.drawLine(160, 131, 180, 152, 0xFFE0);
    display.drawLine(160, 171, 180, 151, 0xFFE0);
    display.drawLine(160, 170, 180, 150, 0xFFE0);

    halDisplayDeselect();
}

void renderPageOrientY() {
    halDisplaySelect();

    display.fillScreen(0x0000);

    display.setCursor(10, 12);
    display.setTextColor(0x07FF);
    display.setTextSize(2);
    display.println("LINIA 2/2: VERTICALA");

    display.setTextSize(1);
    display.setTextColor(0xFFFF);
    display.setCursor(10, 45);
    display.println("Trageti o linie dreapta continuu");
    display.setCursor(10, 58);
    display.println("de SUS in JOS pe ecran.");

    // Sageata verticala (dublu contur)
    display.drawLine(120, 100, 120, 220, 0xFFE0);
    display.drawLine(121, 100, 121, 220, 0xFFE0);
    display.drawLine(100, 200, 120, 220, 0xFFE0);
    display.drawLine(101, 200, 121, 220, 0xFFE0);
    display.drawLine(140, 200, 120, 220, 0xFFE0);
    display.drawLine(139, 200, 119, 220, 0xFFE0);

    halDisplayDeselect();
}

void renderCalibPointScreen(int idx) {
    halDisplaySelect();

    display.fillScreen(0x0000);

    display.setCursor(10, 10);
    display.setTextColor(0x07FF);
    display.setTextSize(2);
    display.print("PAS 2: COORDONATE ");
    display.print(idx + 1);
    display.println("/4");

    halDisplayDeselect();

    const int TX[] = {25, 215, 215, 25};
    const int TY[] = {85, 85, 300, 300};

    for (int i = 0; i < idx; i++) {
        drawCrosshair(TX[i], TY[i], 0x7BEF); // DARKGREY
    }

    drawCrosshair(TX[idx], TY[idx], 0xFFE0); // YELLOW
}

// ============================================================
// Input unificat - Marble (touch real XPT2046)
// ============================================================
// Variabile touch expuse codului comun
// O singura citire SPI per iteratie de loop; valorile brute (raw) sunt
// refolosite de etapele de calibrare, cele mapate (pixeli) de UI.
static int32_t marbleRawX = 0, marbleRawY = 0, marbleRawZ = 0;
static bool    marblePressed = false;
static int     marbleTouchX = 0;
static int     marbleTouchY = 0;

void inputUpdate()
{
    HalTouchPoint pt = halTouchRead();

    marbleRawX    = pt.x;
    marbleRawY    = pt.y;
    marbleRawZ    = pt.z;
    marblePressed = pt.pressed;

    // Mapare in pixeli doar cand calibrarea este completa
    if (marblePressed && calib.stage == 3) {
        mapTouch(pt.x, pt.y, marbleTouchX, marbleTouchY);
    }
}

// Asteapta eliberarea degetului (echivalent functional cu varianta
// Waveshare; folosit de handler-ele de butoane din UI)
void waitTouchRelease()
{
    while (halTouchRead().pressed) {
        delay(10);
    }
}

#endif // target selection

#include "ui/StartupScreen.h"
#include "ui/BatteryCheckScreen.h"

// STUB / PROVIZORIU pentru testarea Battery Check pe graphic_ui.
// Se elimină la integrarea implementării reale de baterie.
#include "battery/BatteryStub.h"

// ============================================================================
// Pagini UI (comune, independente de hardware)
// ============================================================================
enum UiPage {
    PAGE_BATTERY_CHECK,
    PAGE_START,
    PAGE_DASHBOARD
};

// ============================================================================
// Instante display unificate
// ============================================================================
#if defined(SERVICEBOX_WAVESHARE)
Adafruit_GFX& activeDisplay = display;
#else
Adafruit_GFX& activeDisplay = display; // Marble: ILI9341 real (tft)
#endif

// STUB / PROVIZORIU pentru testarea Battery Check.
BatteryStub batteryStub;

// Pagina Battery Check (independentă de implementarea concretă a bateriei)
BatteryCheckScreen batteryCheckScreen(&activeDisplay, batteryStub);

// Pagina START (implementata anterior, decuplata de HAL)
StartupScreen startupScreen(&activeDisplay, batteryStub);

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
UiPage currentPage = PAGE_BATTERY_CHECK;
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
        // BOOT -> Battery Check (prima pagină UI afișată)
        batteryStub.begin();
        batteryCheckScreen.init();
    }

#elif defined(SERVICEBOX_MARBLE)

    // --------------------------------------------------------
    // MARBLE: initializare hardware real (Model 1 BlueTab) [PRELUARE]
    // --------------------------------------------------------

    // Buton recalibrare (GP5, INPUT_PULLUP, activ LOW)
    pinMode(RECALIB_BUTTON, INPUT_PULLUP);

    // Incarcare calibrare din EEPROM
    loadCalib();

    // Init hardware (pini, SPI1, display, touch)
    halDisplayInit();
    halTouchInit();
    delay(50);

    // CRITIC: Curatarea fortata (flush) a bufferului tactil rezidual
    // inainte de a evalua stadiul - fara asta, la pornire pot aparea
    // atingeri fantoma care declanseaza etape gresite
    for (int i = 0; i < 5; i++) {
        halTouchRead();
        delay(20);
    }

    Serial.println();
    Serial.println("==============================================");
    Serial.println("Service Box - graphic_ui / MARBLE PICO (real)");
    Serial.print("Calibrare: stage=");
    Serial.print(calib.stage);
    Serial.print(" swapXY=");
    Serial.print(calib.swapXY);
    Serial.print(" invX=");
    Serial.print(calib.invX);
    Serial.print(" invY=");
    Serial.println(calib.invY);
    Serial.println("==============================================");

    // Reluarea etapei de calibrare dupa reboot
    // (procedura trece prin reboot-uri intre etape)
    if (calib.stage == 1) {
        renderPageOrientX();
    }
    else if (calib.stage == 4) {
        renderPageOrientY();
    }
    else if (calib.stage == 2) {
        calibPointIndex = 0;
        renderCalibPointScreen(0);
    }
    else {
        // BOOT -> Battery Check (prima pagină UI afișată)
        batteryStub.begin();
        batteryCheckScreen.init();
    }

#endif
}

// ============================================================================
// Loop principal
// ============================================================================
void loop() {

    // STUB / PROVIZORIU: procesare comenzi seriale pentru testare baterie.
    batteryStub.update();

    // Citire input - o singura data per iteratie de loop
    inputUpdate();

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

#elif defined(SERVICEBOX_MARBLE)

    // --------------------------------------------------------
    // MARBLE: flux calibrare (stage 1/4/2) - are prioritate [PRELUARE]
    // Foloseste ultima citire touch (facuta de inputUpdate() la inceput
    // de loop) - evitam dubla citire SPI per iteratie.
    // --------------------------------------------------------
    HalTouchPoint pt;
    pt.x = marbleRawX;
    pt.y = marbleRawY;
    pt.z = marbleRawZ;
    bool pressed = marblePressed;

    if (calib.stage == 1) {
        // PAS 1: swipe orizontal STANGA -> DREAPTA (swapXY + invX)
        if (pressed) {
            if (swipeCount < 2) {
                swipeStartX[swipeCount] = pt.x;
                swipeStartY[swipeCount] = pt.y;
            }
            swipeEndX[0] = swipeEndX[1];
            swipeEndX[1] = pt.x;
            swipeEndY[0] = swipeEndY[1];
            swipeEndY[1] = pt.y;
            swipeCount++;
        }
        else if (swipeCount > 0) {
            if (swipeCount >= 3) {
                long diffX = (long)((swipeEndX[0] + swipeEndX[1]) / 2)
                           - (long)((swipeStartX[0] + swipeStartX[1]) / 2);
                long diffY = (long)((swipeEndY[0] + swipeEndY[1]) / 2)
                           - (long)((swipeStartY[0] + swipeStartY[1]) / 2);

                calib.swapXY = (abs(diffX) > abs(diffY)) ? 0 : 1;

                long dominantX = calib.swapXY ? diffY : diffX;
                calib.invX = (dominantX > 0) ? 0 : 1;

                calib.stage = 4;
                saveCalib();
                forceReboot();
            }
            swipeCount = 0;
        }
        delay(10);
        return;
    }
    else if (calib.stage == 4) {
        // PAS 4: swipe vertical SUS -> JOS (invY)
        if (pressed) {
            if (swipeCount < 2) {
                swipeStartX[swipeCount] = pt.x;
                swipeStartY[swipeCount] = pt.y;
            }
            swipeEndX[0] = swipeEndX[1];
            swipeEndX[1] = pt.x;
            swipeEndY[0] = swipeEndY[1];
            swipeEndY[1] = pt.y;
            swipeCount++;
        }
        else if (swipeCount > 0) {
            if (swipeCount >= 3) {
                long diffX = (long)((swipeEndX[0] + swipeEndX[1]) / 2)
                           - (long)((swipeStartX[0] + swipeStartX[1]) / 2);
                long diffY = (long)((swipeEndY[0] + swipeEndY[1]) / 2)
                           - (long)((swipeStartY[0] + swipeStartY[1]) / 2);

                long dominantY = calib.swapXY ? diffX : diffY;
                calib.invY = (dominantY > 0) ? 0 : 1;

                calib.stage = 2;
                saveCalib();
                forceReboot();
            }
            swipeCount = 0;
        }
        delay(10);
        return;
    }
    else if (calib.stage == 2) {
        // PAS 2: atingerea a 4 puncte in colturi (xmin/xmax/ymin/ymax)
        if (collectState == COLLECT_IDLE) {
            if (pt.z >= Z_SAMPLE_MIN) {
                collectState = COLLECT_SAMPLING;
                sampleCount = 0;
                calibIdleSince = 0;
            }
            else {
                if (calibIdleSince == 0) {
                    calibIdleSince = millis();
                }
                else if (millis() - calibIdleSince >= CALIB_POINT_TIMEOUT_MS) {
                    renderCalibPointScreen(calibPointIndex);

                    halDisplaySelect();
                    display.setCursor(10, 250);
                    display.setTextSize(1);
                    display.setTextColor(0xF800, 0x0000); // RED on BLACK
                    display.print("Timeout! Atingeti din nou punctul. ");
                    halDisplayDeselect();

                    calibIdleSince = millis();
                }
            }
        }
        else if (pt.z >= Z_SAMPLE_MIN) {
            // Colecteaza cate un esantion pe iteratia loop-ului (~10 ms)
            if (sampleCount < 12) {
                calibSamplesX[sampleCount] = pt.x;
                calibSamplesY[sampleCount] = pt.y;
                sampleCount++;
            }
        }
        else {
            // Degetul a fost ridicat: finalizeaza punctul curent
            if (sampleCount >= 5) {
                int32_t sumX = 0;
                int32_t sumY = 0;
                // Media esantioanelor, fara primele/ultimele 2 (margi zgomotoase)
                for (int i = 2; i < sampleCount - 2; i++) {
                    sumX += calibSamplesX[i];
                    sumY += calibSamplesY[i];
                }

                int32_t mx, my;
                processRawTouch(sumX / (sampleCount - 4), sumY / (sampleCount - 4), mx, my, true);

                calibX[calibPointIndex] = mx;
                calibY[calibPointIndex] = my;
                calibPointIndex++;

                if (calibPointIndex >= 4) {
                    int32_t loX = calibX[0];
                    int32_t hiX = calibX[0];
                    int32_t loY = calibY[0];
                    int32_t hiY = calibY[0];

                    for (int i = 1; i < 4; i++) {
                        if (calibX[i] < loX) loX = calibX[i];
                        if (calibX[i] > hiX) hiX = calibX[i];
                        if (calibY[i] < loY) loY = calibY[i];
                        if (calibY[i] > hiY) hiY = calibY[i];
                    }

                    int32_t deltaX = hiX - loX;
                    int32_t deltaY = hiY - loY;

                    // Extrapolare la margini (legata de pozitiile crosshair 25/215, 85/300)
                    calib.xmin = loX - (25 * deltaX) / 190;
                    calib.xmax = hiX + (25 * deltaX) / 190;
                    calib.ymin = loY - (85 * deltaY) / 215;
                    calib.ymax = hiY + (20 * deltaY) / 215;

                    calib.stage = 3;
                    saveCalib();
                    forceReboot();
                }
                else {
                    renderCalibPointScreen(calibPointIndex);
                }
            }
            collectState = COLLECT_IDLE;
        }
        delay(10);
        return;
    }

    // --------------------------------------------------------
    // MARBLE stage==3: buton recalibrare GP5 (2s hold) [PRELUARE]
    // Masina non-blocanta cu 3 faze: repaus -> cronometrare -> eliberare
    // --------------------------------------------------------
    if (calib.stage == 3) {
        static uint8_t recalibPhase = 0;
        static unsigned long recalibStart = 0;
        bool btnLow = (digitalRead(RECALIB_BUTTON) == LOW);

        if (recalibPhase == 0 && btnLow) {
            recalibPhase = 1;
            recalibStart = millis();
        }
        else if (recalibPhase == 1) {
            if (!btnLow) {
                recalibPhase = 0; // eliberat inainte de 2s
            }
            else if (millis() - recalibStart >= 2000) {
                halDisplaySelect();
                display.fillScreen(0x7800); // MAROON
                display.setCursor(15, 120);
                display.setTextSize(2);
                display.setTextColor(0xFFFF);
                display.print("ELIBERATI BUTONUL...");
                halDisplayDeselect();
                recalibPhase = 2;
            }
        }
        else if (recalibPhase == 2 && !btnLow) {
            calib.magic = 0;
            calib.stage = 1;
            saveCalib();
            forceReboot();
        }
    }

    // Poarta non-blocanta: dupa o actiune, ignora touch pana la eliberare
    if (awaitingRelease) {
        if (pressed) {
            releaseSince = 0;
        }
        else if (releaseSince == 0) {
            releaseSince = millis();
        }
        else if (millis() - releaseSince >= 50) {
            awaitingRelease = false;
            releaseSince = 0;
        }

        if (awaitingRelease) {
            delay(10);
            return;
        }
    }

#endif // SERVICEBOX_MARBLE

    // Coordonate unificate pentru UI (setate de inputUpdate() la inceput de loop)
#if defined(SERVICEBOX_WAVESHARE)
    int  currentX       = touch_x;
    int  currentY       = touch_y;
    bool isScreenActive = touch_pressed;
#else
    int  currentX       = marbleTouchX;   // Marble: touch real XPT2046
    int  currentY       = marbleTouchY;
    bool isScreenActive = marblePressed;
#endif

    // ========================================================================
    // UI APPLICATION STATE MACHINE ROUTING
    // ========================================================================
    if (currentPage == PAGE_BATTERY_CHECK) {

        // Procesăm întâi logica paginii (touch, timeout, citire baterie),
        // apoi desenăm. Astfel, o comandă serială care schimbă nivelul este
        // vizibilă imediat în aceeași iterație de loop.
        batteryCheckScreen.update(currentX, currentY, isScreenActive);

        if (refreshPageNeeded) {
            batteryCheckScreen.render(true);
            refreshPageNeeded = false;
        }
        else {
            batteryCheckScreen.render();
        }

        if (batteryCheckScreen.shouldAdvance()) {
            Serial.println("[UI Navigation] Battery Check OK -> PAGE_START");
            currentPage = PAGE_START;
            startupScreen.init();
            refreshPageNeeded = true;
        }
    }
    else if (currentPage == PAGE_START) {

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
            startupScreen.init();
            refreshPageNeeded = true;
        }
    }
}
