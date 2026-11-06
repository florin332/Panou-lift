# Analiză Migrare CH9121 → WIZNET W5500

> Stare implementată: CH9121 local și recepția UART au fost eliminate. W5500 folosește SPI1 pe GP12/13/14/15, DHCP și MAC `3C:AB:72:52:68:12`. Panoul ascultă TCP pe 2000 (encoder), 2004 (transmitere echipament) și 2005 (service). Core 0 deține W5500; Core 1 accesează canalele prin cozi. Comenzile de service au parser comun pentru USB și TCP. Formatele GSM/SSC30KQ și accesul web nu sunt implementate aici.

Secțiunile de mai jos păstrează analiza inițială de dinaintea migrării; recomandările de pinout și pașii enumerați acolo sunt istorice, nu descriu starea curentă.

## 1. Situația Pre-Migrare (CH9121)

### Arhitectură Comunicație
- **CH9121**: Modul Ethernet serial USB
  - Interfață: 2× UART (transparent bridge)
  - Funcție: Converte date UART → Ethernet TCP/IP și invers
  - Rol în proiect: Connector între Panou Cabină și Sistem Central

### Mapare Pinuri CH9121 (RP2040 → CH9121)
| GPIO | UART | Direcție | Conectare CH9121 | Folosire Actuală |
|------|------|----------|------------------|------------------|
| GP0  | UART1 TX | OUT | RXD1 (CH9121) | Comanda alarma (REZERVAT) |
| GP1  | UART1 RX | IN | TXD1 (CH9121) | NEUTILIZAT |
| GP4  | UART2 TX | OUT | RXD2 (CH9121) | NEUTILIZAT |
| GP5  | UART2 RX | IN | TXD2 (CH9121) | **Receptie date ascensor** (ACTIV) |

### Configurație UART Curentă
```cpp
// Protocol.cpp
Serial1.setTX(Pins::UART::ALARM_TX);  // GP0
Serial1.setRX(-1);                    // Dezactivat
Serial2.setTX(-1);                    // Dezactivat
Serial2.setRX(Pins::UART::DATA_RX);   // GP5
Serial1.begin(Config::Protocol::SERIAL_BAUD);
Serial2.begin(Config::Protocol::SERIAL_BAUD);
```

**Baud Rate**: Configurat în `Config::Protocol::SERIAL_BAUD`

### Fluxul Comunicație Curent
```
[Sistem Central] ←→ [CH9121 - Ethernet] ←→ [UART via GP5] → [RP2040] → [Panou Cabină]
                                               GP0 →
```

---

## 2. Caracteristicile W5500

### Ce este W5500
- **IC Ethernet Controller**: SPI-based Ethernet processor
- **Protocol**: 10/100Mbps Ethernet cu implementare TCP/IP hardware
- **Avantaje**:
  - Viteză mai mare decât UART serial
  - Suport TCP/UDP/ICMP native (socket-based)
  - Buffer intern (16KB RX + 16KB TX)
  - Consum putere mai redus
  - Arquitectură mai robust (MAC + PHY integrate)
  
- **Dezavantaje**:
  - Necesită driver SPI dedicat (0-25MHz)
  - Stack IP mai complex (PIO stack, nu mai transparent)
  - Configurare inițială mai elaborată
  - Compatibilitate înapoi: trebuie adapt protocolul de comunicație

### W5500 vs CH9121
| Criteriu | CH9121 | W5500 |
|----------|--------|-------|
| Interfață | 2× UART | 1× SPI |
| Pini RP2040 | 4 (UART) | 4-5 (SPI) |
| Protocol | Transparent serial | TCP/UDP sockets |
| Latență | ~10ms (UART) | ~1-5ms (SPI/Ethernet) |
| Viteză | 115.2 kbps max | 10-100 Mbps |
| Complexitate Software | Joasă | Medie-Înaltă |

---

## 3. Impactul Migrării

### 3.1 Modificări Hardware

#### Pini Necesari pentru W5500 (SPI)
```
RP2040 → W5500
GP18 (SPI0 SCK)   → SCLK
GP19 (SPI0 MOSI)  → MOSI  
GP16 (SPI0 MISO)  → MISO  [** CONFLICT - acum folosit LCD SCK]
GPIO_CS (new)     → CS    [GPIO liber de ales]
GPIO_RST (new)    → RST   [GPIO liber de ales]
GPIO_INT (new)    → INT   [Optional, GPIO liber]
```

**⚠️ CONFLICT HARDWARE**: 
- W5500 necesită **SPI0** (SCLK/MOSI/MISO)
- LCD (ILI9341) folosește deja **SPI0**: SCLK=GP18, MOSI=GP19
- **PROBLEMA**: W5500 și LCD nu pot partaja SPI0 direct fără multiplexing software

#### Opțiuni Rezolvare:
1. **SPI Sharing (Multiplexing)**
   - Ambele dispozitive pe SPI0
   - CS line selectează deviceul activ (W5500 vs LCD)
   - Punct critic: interblocare, latenţă LCD crește

2. **SPI1 pentru W5500**
   - RP2040 are SPI1: GP10(MOSI), GP11(MISO), GP14(SCK)
   - GP10 este deja OCUPAT de I2S (Audio)
   - Necesită remap GPIO (conflict mai grav)

3. **Redesign Hardware** (RECOMAND)
   - Scop LCD pe SPI0 (status quo - bun)
   - Pune W5500 pe SPI1 cu GPIO remap
   - Alternativ: schimbă LCD pe GPIO banc diferit

#### Pini Disponibili pentru CS/RST/INT
Din fișierul [GPIO_Allocation.md](GPIO_Allocation.md):
- **Disponibili**: 2, 3, 11, 12, 13, 14, 15, 16, 17, 23, 24, 25, 27, 28, 29
- **Sugestie**: 
  - W5500_CS = GP17  
  - W5500_RST = GP24
  - W5500_INT = GP25

---

### 3.2 Modificări Software - Protocol

#### A) Nivel Low: Transport Layer
**Curent (UART/CH9121)**:
```cpp
// Transparent serial: bytes interni UART → CH9121 → Ethernet TCP
Serial2.print(buffer);  // Trimite pe UART GP5
// CH9121 șterge automat formatare, paseaza pe Ethernet
```

**Nou (W5500 SPI)**:
```cpp
// Explicit socket management (must implement)
uint8_t socket = 0;
w5500_send_data(socket, buffer, length);  // SPI transactions
// Implementare stack TCP/UDP necesară
```

#### B) Nivel Mid: Structură Pachete
**Protocol actual** (din Protocol.cpp):
- Format: `START_MARKER + fields + CRC + END_MARKER`
- Exemplu: `<LIFT:1,POS:45,STAT:IDLE*XY>\r\n`
- CRC-8-CCITT cu verificare de sekare
- Timeout cadru: `Config::Protocol::PACKET_TIMEOUT_MS`

**Cu W5500**:
- Pachetele trec prin Ethernet → TCP/UDP stream
- Pierdere pachet: posibilă (TCP garantează livrare, UDP nu)
- CRC deja verificat de controller IP hardware
- **Decizie**: Păstraţi protocolul, doar transport se schimbă?
  - **Recomandare**: Păstraţi CRC și format, dar fie schimbaţi terminator seria, fie adăugaţi TCP frame length

#### C) Nivel High: API Aplicație
**Curent**:
```cpp
// din Protocol.h
void init();
void update(SharedPanel &localPanel);
void commCountEnable();
```

**Cerințe noi**:
```cpp
// Adăugat:
bool w5500_init(uint8_t cs_pin, uint8_t rst_pin);
bool w5500_connect(const char *server_ip, uint16_t port);
bool w5500_send_packet(const uint8_t *data, uint16_t len);
bool w5500_receive_packet(uint8_t *buffer, uint16_t &len);
bool w5500_is_connected();
void w5500_disconnect();
```

---

### 3.3 Fișiere Afectate

| Fișier | Schimbări Necesare | Severitate |
|--------|-------------------|-----------|
| [src/Core/Models/Pins.h](src/Core/Models/Pins.h) | Adăugă W5500 pini (CS/RST/INT); posibil remap SPI1 | **ALTA** |
| [src/Core/Models/Config.h](src/Core/Models/Config.h) | Adăugă param W5500 (server IP, port, SPI freq) | **MEDIE** |
| [src/Core/Services/Protocol.cpp](src/Core/Services/Protocol.cpp) | Rewrite UART → W5500 SPI; păstrează logică parsare | **CRITICĂ** |
| [src/Core/Services/Protocol.h](src/Core/Services/Protocol.h) | Adăugă W5500 API (init, connect, send, recv) | **MEDIE** |
| [platformio.ini](platformio.ini) | Adăugă librărie W5500 (WIZ_Ethernet) | **JOASĂ** |

---

## 4. Pași Migrare Recomandați

### Fază 1: Pregătire Hardware & Librării
1. **Selectează pini liberi** pentru W5500 CS/RST/INT
2. **Adaugă în platformio.ini**:
   ```ini
   lib_deps = 
       https://github.com/WIZnet/wiznet-iot-shield-mbed-kr
       # sau
       Ethernet_Manager  ; versiune stability proven
   ```
3. **Crează hardware test file** (W5500 SPI communication)

### Fază 2: Adăugă Driver W5500 minimal
1. Crează [src/Drivers/W5500.h](src/Drivers/W5500.h) cu API:
   - `init()`, `connect()`, `send()`, `recv()`, `poll()`
2. Test pe REPL: SPI comun, citire chip version
3. Test configurare socket TCP

### Fază 3: Adapte Protocol Layer
1. **Păstrează parser Protocol.cpp** (format cadru)
2. Swap transport: 
   - Înlocuiește `Serial2.print()` cu `w5500_send()`
   - Înlocuiește `Serial2.available()` cu `w5500_poll()`
3. Handle timeout & reconecție

### Fază 4: Testing & Rollback
1. **Păstrează CH9121 ca fallback** (sus pe senzori digitali?)
2. Test dual-stack: CH9121 AND W5500 în paralel pe faza inițială
3. Comutare soft: flag Runtime pt care stack folosi

### Fază 5: Optimizări
1. Buffer size tuning (W5500 are 16KB)
2. SPI clock optimization (test 8MHz → 25MHz)
3. Latency measurements (UART → Ethernet)

---

## 5. Riscuri & Considerații

### Risk: Interfață SPI Congestie
- **Severitate**: 🔴 ALTA
- **Descriere**: LCD + W5500 pe SPI0 crează congestie
- **Mitigare**: Implementează SPI queue, prioritizare CS

### Risk: Pierdere Compatibilitate Service Box
- **Severitate**: 🟡 MEDIE
- **Descriere**: CH9121 e transparent; W5500 necesită IP config
- **Mitigare**: Adapte Service Box cu suport W5500 simultan

### Risk: Timeout Protocol
- **Severitate**: 🟡 MEDIE
- **Descriere**: TCP latency ≠ UART delay
- **Mitigare**: Rezigiștri timeout constants din Config

### Risk: Power Consumption
- **Severitate**: 🟢 JOASĂ
- **Descriere**: W5500 e mai eficient, dar PHY Ethernet mai greu
- **Mitigare**: Verify cu Power Supply Budget

---

## 6. Estimări

| Activitate | Timp | Complexitate |
|------------|------|-------------|
| Alegere pini + schematic check | 30 min | Joasă |
| Setup platformio + librării | 30 min | Joasă |
| Creare driver W5500 SPI minimal | 2h | Medie |
| Adapte Protocol.cpp | 1.5h | Medie |
| Testing comunicație | 2h | Medie |
| Debug SPI/TCP issues | 3h | Înaltă |
| Integration full | 1h | Joasă |
| **TOTAL** | **~10h** | - |

---

## 7. Recomandații Finale

1. ✅ **Verifică cu Hardware Design**: Conflict SPI0 este dealbreaker
2. ✅ **Solver GPIO conflict** ÎNAINTE de coding
3. ✅ **Păstrează Protocol.h API** neschimbat (client code isolation)
4. ✅ **Implementează fallback** CH9121 o perioadă (dual-stack)
5. ✅ **Test incremential**: SPI comun → socket TCP → full frame handling
6. ✅ **Documentează mapping** IP protocol ↔ panou (Service Box side)

---

## Concluzii

| Aspect | Verdict |
|--------|---------|
| Feasibilitate | ✅ Posibil, cu rework SPI GPIO |
| Effort | 🟡 ~10h (asumând SPI resolved) |
| Benefit | ✅ Viteză 100×, robustețe, future-proof IP stack |
| Risk | 🟡 MEDIE (SPI contention, protocol testing) |
| Prioritate | 🟡 Mediu - faceți doar dacă cerință =Ethernet nativ requerida |
