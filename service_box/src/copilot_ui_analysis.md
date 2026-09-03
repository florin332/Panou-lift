# Analiză UI – Implementare Battery Check

> Sursă cerință: [copilot_ui_update.txt](copilot_ui_update.txt)  
> Specificație autoritară: [ui_requirements.md](ui_requirements.md)

## 1. Rezumatul cerinței

Secvența de pornire trebuie modificată astfel:

```
BOOT
  ↓
BATTERY CHECK
  ↓
START SCREEN
  ↓
MAIN MENU
```

**Pagina Battery Check:**
- Pagină informativă, fără controale / touchscreen.
- În regim normal: afișează `BATTERY LEVEL` și `XX %`, timp de 5 secunde, apoi trece automat la START SCREEN.
- Culoarea procentului indică nivelul: roșu (scăzut), galben (mediu), verde (bun); pragurile vor fi stabilite ulterior.
- În timpul încărcării (`CHARGING`): nu se afișează procentul, nu se pornește temporizarea de 5 secunde, pagina rămâne activă până la terminarea încărcării.
- După terminarea încărcării se revine la comportamentul normal + temporizare 5s + START SCREEN.

**START SCREEN:**
- Se păstrează exact conținutul existent (`SERVICE BOX`, `PANOURI ASCENSOARE`, versiune, buton START, info SD opțional).
- Nu se adaugă diagnostice/progress bars/etc.

**MAIN MENU:**
- Se păstrează destinația actuală după START; nu se implementează alte pagini noi.

**Arhitectură:**
- UI primește starea bateriei abstract: `percentage` + `charging`.
- Conversia tensiune → procent și detectarea încărcării rămân în HAL / aplicație.
- Nu se duplică logica UI între display-uri.
- Nu se modifică HAL decât dacă este strict necesar pentru datele cerute de UI.

---

## 2. Starea actuală a codului

### 2.1. Main loop / state machine ([main.cpp](main.cpp))

- Variabila globală `UiPage currentPage` este inițializată cu `PAGE_START`.
- Nu există o pagină intermediară `PAGE_BATTERY_CHECK`.
- Mecanismul de schimbare a paginilor se face prin:
  - `currentPage = <valoare>;`
  - `refreshPageNeeded = true;`
- Randarea START SCREEN se face apelând `Hardware.renderStartPage(forceRedraw, statusMsg, statusColor)`.
- După apăsarea START și handshake, se trece în `PAGE_DASHBOARD` (care joacă rolul de ecran principal / MAIN MENU).

### 2.2. Enum-ul paginilor ([hal/HardwareInterface.h](hal/HardwareInterface.h))

```cpp
enum UiPage {
    PAGE_START,
    PAGE_DASHBOARD,
    PAGE_TEST_SUITE
};
```

Lipsește `PAGE_BATTERY_CHECK`.

### 2.3. Clasa StartupScreen ([ui/StartupScreen.h](ui/StartupScreen.h), [ui/StartupScreen.cpp](ui/StartupScreen.cpp))

- Implementează conținutul cerut pentru START SCREEN (titlu, subtitlu, versiune, buton START, info SD).
- Este **neutilizată** în prezent în `main.cpp` (includerea este comentată).
- Folosește `Adafruit_GFX*`, ceea ce nu se potrivește direct cu implementarea actuală Waveshare care randează prin Pico-SDK (`spi_write_blocking`), fără obiect Adafruit_GFX.

### 2.4. HAL Waveshare ([hal/HardwareWaveshare.cpp](hal/HardwareWaveshare.cpp))

- Folosește SPI1 + Pico SDK pentru LCD ST7789T3.
- Nu există:
  - citire ADC pentru baterie;
  - metodă care să raporteze procentul bateriei;
  - metodă care să raporteze starea de încărcare (`charging`).
- Există doar `BAT_EN` (GPIO 26) folosit pentru „board power enable” (ieșire, HIGH).
- Randare actuală: `renderStartPage()` doar curăță ecranul și desenează o bandă de status colorată; nu randează conținutul START SCREEN cerut de specificație.
- Touch nu este implementat (funcții stub).

### 2.5. HAL Marble ([hal/HardwareMarble.cpp](hal/HardwareMarble.cpp))

- Implementează touch XPT2046 și SD.
- **Nu implementează** metodele virtuale:
  - `sendCommand`
  - `updateCommEngine`
  - `getCommState`
  - `getLastResponse`
  - `clearCommState`
  - `renderStartPage`
- Nu există funcții pentru baterie / charging.
- Aceasta produce erori de compilare pentru targetul `marble_pico` în starea actuală.

---

## 3. Ce trebuie modificat

### 3.1. Enum pagini ([hal/HardwareInterface.h](hal/HardwareInterface.h))

Adăugare:

```cpp
enum UiPage {
    PAGE_BATTERY_CHECK,
    PAGE_START,
    PAGE_DASHBOARD,
    PAGE_TEST_SUITE
};
```

`main.cpp` va porni cu `currentPage = PAGE_BATTERY_CHECK`.

### 3.2. Interfață HAL pentru baterie ([hal/HardwareInterface.h](hal/HardwareInterface.h))

Trebuie adăugate metode abstracte (sau cu fallback) pentru a furniza datele abstracte cerute de UI:

```cpp
virtual int getBatteryPercentage() = 0;   // 0..100
virtual bool isBatteryCharging() = 0;
```

Alternative discutate:
- `virtual BatteryStatus getBatteryStatus()` – struct cu `percentage` + `charging`.
- Ambele sunt acceptabile; varianta cu două metode este cea mai simplă și directă.

### 3.3. Implementare baterie în HAL-uri

**Waveshare:**
- Trebuie identificat pinul ADC pentru măsurarea tensiunii bateriei și pinul GPIO pentru detectarea încărcării (schema hardware / documentație Waveshare RP2350-Touch-LCD-2.8).
- Până la identificare, se poate implementa un stub care returnează `percentage = 100`, `charging = false`, pentru a permite compilarea și testarea UI.
- **Nu se inventează** conversia tensiune → procent; se așteaptă informația hardware.

**Marble:**
- Similar, trebuie identificat dacă placa are măsurare baterie / charging.
- Dacă nu există, implementare stub care returnează valori implicite.

### 3.4. Randare pagină Battery Check

Având în vedere că Waveshare randează direct prin Pico SDK (nu Adafruit_GFX), iar Marble are touch/XPT dar nu are randare grafică Adafruit configurată în HAL, cea mai puțin invazivă abordare este:

**Opțiune recomandată A (păstrează arhitectura actuală):**
- Adaugă în `HardwareInterface` metoda:
  ```cpp
  virtual void renderBatteryCheckPage(bool forceRedraw, int percentage, bool charging) = 0;
  ```
- Implementează metoda în ambele HAL-uri:
  - Waveshare: folosește `lcdFill`, `lcdFillRect`, `lcdSetWindow` + text desenat manual (primitive existente).
  - Marble: poate folosi tot `Adafruit_GFX`/ILI9341 dacă se decide adăugarea unui obiect display în HAL; altfel stub care înregistrează pe serial.

**Opțiune B (reutilizare StartupScreen):**
- Reactivează clasa `StartupScreen` pentru START SCREEN.
- Creezi o clasă `BatteryCheckScreen` similară.
- Necesită ca HAL să expună un `Adafruit_GFX*` (ceea ce Waveshare nu face în prezent) → implică modificări mai mari în HAL.

**Concluzie:** Opțiunea A este minimală și respectă specificația de a nu rescrie arhitectura.

### 3.5. Logică temporizare în [main.cpp](main.cpp)

Pentru `PAGE_BATTERY_CHECK`:

```cpp
if (currentPage == PAGE_BATTERY_CHECK) {
    if (refreshPageNeeded) {
        Hardware.renderBatteryCheckPage(true, Hardware.getBatteryPercentage(), Hardware.isBatteryCharging());
        refreshPageNeeded = false;
        batteryCheckTimer = millis();
    }

    bool charging = Hardware.isBatteryCharging();

    // Dacă starea de încărcare s-a schimbat, re-randează.
    if (charging != lastChargingState) {
        Hardware.renderBatteryCheckPage(true, Hardware.getBatteryPercentage(), charging);
        lastChargingState = charging;
        if (!charging) batteryCheckTimer = millis();  // reîncepe cele 5s când se termină încărcarea
    }

    if (!charging && (millis() - batteryCheckTimer >= 5000)) {
        currentPage = PAGE_START;
        refreshPageNeeded = true;
    }
}
```

Pagina Battery Check nu procesează touch.

### 3.6. START SCREEN

- Se păstrează logica existentă din `main.cpp` pentru `PAGE_START`.
- Randarea trebuie să includă conținutul cerut de specificație. În starea actuală `Hardware.renderStartPage()` desenează doar o bandă de status.
- Pentru conformitate completă, `renderStartPage()` trebuie extins în HAL pentru a desena: `SERVICE BOX`, `PANOURI ASCENSOARE`, versiune, buton START, info SD.
- Dacă se dorește reutilizare, se poate folosi clasa `StartupScreen`, dar necesită refactoring HAL (opțiune mai mare).

---

## 4. Probleme / blocaje identificate

1. **Detectarea încărcării (`charging`) nu este disponibilă în HAL.**
   - Nu există pin GPIO / ADC definit pentru charging în `HardwareInterface` sau în HAL-uri.
   - Nu se poate implementa conversie/măsurare fără schema hardware Waveshare / Marble.
   - **Recomandare:** implementare stub temporară (`return false`) până la definirea pinului corect.

2. **Procentul bateriei nu este disponibil în HAL.**
   - Nu există citire ADC / conversie tensiune → procent.
   - **Recomandare:** stub temporar (`return 100`) până la integrarea hardware.

3. **HAL Marble are metode virtuale neimplementate.**
   - `renderStartPage`, `sendCommand`, `updateCommEngine`, `getCommState`, `getLastResponse`, `clearCommState`.
   - Build-ul targetului `marble_pico` va eșua în starea actuală; trebuie completate măcar cu stub-uri.

4. **Waveshare HAL nu implementează conținutul START SCREEN cerut.**
   - Randarea actuală este doar o bandă de status. Pentru conformitate cu specificația UI trebuie adăugat text/buton/info SD.

5. **Clasa `StartupScreen` este nefolosită.**
   - Poate fi păstrată pentru viitor sau eliminată dacă se alege abordarea HAL-direct.

6. **Touch pe Waveshare este stub.**
   - Nu afectează pagina Battery Check (non-interactivă), dar afectează START SCREEN (buton START).
   - Nu face obiectul acestei sarcini, dar este o problemă notată.

---

## 5. Fișiere care vor fi modificate (propunere minimă)

| Fișier | Modificare |
|--------|------------|
| [main.cpp](main.cpp) | Adaugă `PAGE_BATTERY_CHECK` în state machine; implementează temporizarea 5s; pornește cu `PAGE_BATTERY_CHECK`. |
| [hal/HardwareInterface.h](hal/HardwareInterface.h) | Adaugă `PAGE_BATTERY_CHECK` în enum; adaugă metode `getBatteryPercentage()` și `isBatteryCharging()`; adaugă `renderBatteryCheckPage()`. |
| [hal/HardwareWaveshare.cpp](hal/HardwareWaveshare.cpp) | Implementează `getBatteryPercentage()`, `isBatteryCharging()` (stub/propriu), `renderBatteryCheckPage()`; extinde `renderStartPage()` la conținutul specificat (opțional dar necesar pentru conformitate). |
| [hal/HardwareMarble.cpp](hal/HardwareMarble.cpp) | Implementează metodele virtuale lipsă (măcar stub) + `getBatteryPercentage()`, `isBatteryCharging()`, `renderBatteryCheckPage()`. |
| [platformio.ini](platformio.ini) | Posibil ajustări de librării / define-uri, dar probabil nu este necesar pentru această schimbare. |

---

## 6. Plan de implementare recomandat

1. **Pasul 0 (pregătire):** decide pinii / metoda reală pentru baterie și charging; dacă nu sunt disponibili, implementează stub-uri documentate.
2. **Pasul 1:** extinde `HardwareInterface.h` cu enum + metode abstracte.
3. **Pasul 2:** implementează metodele în `HardwareWaveshare.cpp` (randare + stub baterie).
4. **Pasul 3:** implementează metodele în `HardwareMarble.cpp` (randare + stub baterie + completare metode lipsă).
5. **Pasul 4:** modifică `main.cpp` pentru fluxul `PAGE_BATTERY_CHECK → PAGE_START → PAGE_DASHBOARD`.
6. **Pasul 5:** build `waveshare_rp2350` (și `marble_pico` dacă este necesar) și corectează erorile cauzate de modificări.
7. **Pasul 6:** verificare vizuală pe hardware Waveshare.

---

## 7. Decizii de design

- Se preferă **extinderea HAL cu metode de randare** (`renderBatteryCheckPage`) în loc de reactivarea `StartupScreen`, deoarece Waveshare nu folosește `Adafruit_GFX` și rescrierea HAL pentru a expune un obiect grafic ar fi mai invazivă.
- Pragurile de culoare (roșu/galben/verde) vor fi definite în UI (`main.cpp` sau o mică structură de UI), nu în HAL, conform specificației.
- Temporizarea de 5 secunde este gestionată în UI (`main.cpp`), nu în HAL.
- Detectarea încărcării și procentul bateriei rămân în HAL; UI doar afișează.

---

## 8. Build

**Nu au fost efectuate modificări încă.** Build-ul în starea actuală:
- Pentru `waveshare_rp2350`: compilează probabil cu succes (HAL Waveshare este complet față de interfață), dar nu există Battery Check.
- Pentru `marble_pico`: **va eșua** din cauza metodelor virtuale neimplementate în `HardwareMarble.cpp`.

După implementarea propusă se va re-rula build și se va raporta rezultatul.
