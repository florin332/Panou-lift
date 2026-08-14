// transmitter_bun_v5_8_4
/*
  Transmițător Lift v5.8.3 — Dual Counter Diagnostic
  ====================================================
  Baza: v5.8.2 cu două contoare separate:
    - txFrames : număr total de cadre transmise pe Serial1
    - txBursts : număr de evenimente/heartbeat-uri care au declanșat TX

  În BOTH:
    1 burst = legacy + CRC = 2 frames
    1 rafală de 3 × BOTH = 3 bursts = 6 frames

  Diagnostic:
    txFrames ar trebui să fie întotdeauna txBursts × N,
    unde N = 1 (LEGACY/CRC) sau 2 (BOTH).

  Filozofie:
    - Event-driven, quiet line
    - Rafală 3 × la schimbare, heartbeat la 2 s
    - Dacă stare nouă în timpul rafalei, rafala se resetează
*/

#include <Arduino.h>
#include <U8g2lib.h>
#include <SPI.h>
#include <Wire.h>

// Display OLED SH1106 128x64, I2C, rotație 180°
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R2, /* reset=*/ U8X8_PIN_NONE);

// Pinii pentru stație (destinație) — 18 etaje
const int staPins[18] = {26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43};

// Pinii pentru poziție — 18 etaje
const int posPins[18] = {44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 69, 68, 67, 66, 65, 64, 63, 62};

// Pinii auxiliari
const int swPin   = 10;
const int testPin = 11;
const int relPin  = 56;
const int svcPin  = 57;
const int perPin  = 58;
const int occPin  = 59;
const int upPin   = 60;
const int dwnPin  = 61;

// Array-uri pentru citirea intrărilor
int sta[18];
int pos[18];

// ================================================================
// Configurare transmisie
// ================================================================
enum TxMode {
  LEGACY = 0,
  CRC    = 1,
  BOTH   = 2
};

const TxMode TX_MODE = BOTH;

// Timing
const unsigned long BURST_GAP_MS   = 10;
const unsigned long HEARTBEAT_MS   = 2000;
const uint8_t       BURST_COUNT    = 3;

// ================================================================
// CRC-8 (polinom 0x07)
// ================================================================
uint8_t crc8(const char *data) {
  uint8_t crc = 0x00;
  while (*data) {
    crc ^= (uint8_t)*data++;
    for (uint8_t i = 0; i < 8; i++) {
      crc = (crc & 0x80) ? ((crc << 1) ^ 0x07) : (crc << 1);
    }
  }
  return crc;
}

void buildLegacyFrame(char *buffer, int Pos, int Des, int Ocp, int Sj, int Svc) {
  sprintf(buffer, "<%02d,%02d,%d,%d,%d>", Pos, Des, Ocp, Sj, Svc);
}

void buildCrcFrame(char *buffer, int Pos, int Des, int Ocp, int Sj, int Svc) {
  char payload[16];
  sprintf(payload, "%02d,%02d,%d,%d,%d", Pos, Des, Ocp, Sj, Svc);
  uint8_t crc = crc8(payload);
  sprintf(buffer, "{%s,%02X}", payload, crc);
}

// ================================================================
// Contoare diagnostic
// ================================================================
uint32_t txFrames = 0;   // cadre efectiv transmise pe Serial1
// TX diagnostics:
// txFrames = individual serial frames sent.
// txBursts = transmission bursts/events started.
uint32_t txBursts = 0;   // evenimente/heartbeat-uri care au declanșat TX

void transmitFrames(int Pos, int Des, int Ocp, int Sj, int Svc) {
  char legacyBuffer[24];
  char crcBuffer[24];

  txBursts++;  // un apel = un burst (eveniment sau heartbeat)

  switch (TX_MODE) {
    case LEGACY:
      buildLegacyFrame(legacyBuffer, Pos, Des, Ocp, Sj, Svc);
      Serial.println(legacyBuffer);
      Serial1.println(legacyBuffer);
      txFrames += 1;
      break;

    case CRC:
      buildCrcFrame(crcBuffer, Pos, Des, Ocp, Sj, Svc);
      Serial.println(crcBuffer);
      Serial1.println(crcBuffer);
      txFrames += 1;
      break;

    case BOTH:
      buildLegacyFrame(legacyBuffer, Pos, Des, Ocp, Sj, Svc);
      Serial.println(legacyBuffer);
      Serial1.println(legacyBuffer);

      buildCrcFrame(crcBuffer, Pos, Des, Ocp, Sj, Svc);
      Serial.println(crcBuffer);
      Serial1.println(crcBuffer);
      txFrames += 2;
      break;
  }
}

// ================================================================
// Variabile stare transmisie
// ================================================================
int txPos = 0, txDes = 0, txOcp = 0, txSj = 0, txSvc = 0;
uint8_t burstRemaining = 0;
unsigned long lastBurstMillis = 0;
unsigned long lastTxMillis = 0;

void setup() {
  u8g2.begin();
  u8g2.enableUTF8Print();
  Serial.begin(9600);
  Serial1.begin(9600);

  pinMode(swPin,   INPUT_PULLUP);
  pinMode(perPin,  INPUT_PULLUP);
  pinMode(occPin,  INPUT_PULLUP);
  pinMode(upPin,   INPUT_PULLUP);
  pinMode(dwnPin,  INPUT_PULLUP);
  pinMode(testPin, INPUT_PULLUP);
  pinMode(relPin,  INPUT_PULLUP);
  pinMode(svcPin,  INPUT_PULLUP);

  pinMode(13, INPUT_PULLUP);
  pinMode(12, INPUT_PULLUP);
  pinMode(9,  INPUT_PULLUP);
  pinMode(8,  INPUT_PULLUP);
  pinMode(7,  INPUT_PULLUP);
  pinMode(6,  INPUT_PULLUP);
  pinMode(5,  INPUT_PULLUP);
  pinMode(4,  INPUT_PULLUP);
  pinMode(3,  INPUT_PULLUP);
  pinMode(2,  INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
  pinMode(15, INPUT_PULLUP);
  pinMode(16, INPUT_PULLUP);
  pinMode(17, INPUT_PULLUP);
  pinMode(22, INPUT_PULLUP);
  pinMode(23, INPUT_PULLUP);
  pinMode(24, INPUT_PULLUP);
  pinMode(25, INPUT_PULLUP);

  for (int i = 0; i < 18; i++) {
    pinMode(staPins[i], INPUT_PULLUP);
    pinMode(posPins[i], INPUT_PULLUP);
  }

  // Splash screen
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_streamline_computers_devices_electronics_t);
  u8g2.setCursor(10, 32);
  u8g2.print("\u0035");
  u8g2.setFont(u8g2_font_streamline_interface_essential_setting_t);
  u8g2.setCursor(100, 32);
  u8g2.print("\u0035");
  u8g2.setFont(u8g2_font_logisoso16_tr);
  u8g2.setCursor(50, 30);
  u8g2.print("v5.8");
  u8g2.setFont(u8g2_font_logisoso16_tr);
  u8g2.setCursor(10, 60);
  u8g2.print("0740.317.707");
  u8g2.sendBuffer();
  delay(5000);
  u8g2.clearBuffer();
}

void loop() {
  static int Des = 0;
  static int Pos = 0;
  static int lastPos = 0;
  static int Ocp = 0;
  static int Sumsta = 0;
  static int Rev = 0;

  int Sj = 0;
  int sumpos = 0;
  int sumsta = 0;
  int Svc = 0;
  int Rel = 0;
  int Display = 0;
  int Test = 0;

  unsigned long now = millis();

  // ========== 1. CITIRE INTRĂRI ==========
  for (int i = 0; i < 18; i++) {
    sta[i] = digitalRead(staPins[i]);
    pos[i] = !digitalRead(posPins[i]);

    sumpos += pos[i];
    sumsta += sta[i];

    if (sta[i] == 0) { Des = i; }
    if (pos[i] == 1) { Pos = i; }
  }

  Sumsta = sumsta;

  // ========== 2. LOGICĂ POZIȚIE ==========
  if (sumpos == 0) {
    Pos = lastPos;
  } else if (sumpos == 1) {
    lastPos = Pos;
  } else if (sumpos == 18) {
    Pos = lastPos;
  } else {
    Pos = lastPos;
  }

  // ========== 3. CITIRE INTRĂRI AUXILIARE ==========
  if (digitalRead(upPin) == 1 && digitalRead(dwnPin) == 0) {
    Sj = 2;
  } else if (digitalRead(upPin) == 0 && digitalRead(dwnPin) == 1) {
    Sj = 1;
  } else {
    Sj = 0;
  }

  Ocp = (digitalRead(occPin) == 0) ? 1 : 0;
  Rev = (digitalRead(svcPin) == 0) ? 1 : 0;
  Test = (digitalRead(testPin) == 0) ? 1 : 0;
  Rel = (digitalRead(relPin) == 0) ? 1 : 0;

  // ========== 4. DETERMINARE STATUS SERVICE ==========
  bool brtTriggered = (sumpos == 18) && (sumsta == 18);

  if (brtTriggered)      { Svc = 1; }
  else if (Rev == 1)     { Svc = 2; }
  else if (Rel == 1)     { Svc = 3; }
  else                   { Svc = 5; }

  // ========== 5. AFIȘAJ ==========
  Display = digitalRead(swPin);
  u8g2.clearBuffer();

  // Contoare diagnostic — colț dreapta-sus, vizibile în toate modurile
  u8g2.setFont(u8g2_font_helvR08_tf);
  u8g2.setCursor(78, 10);
  u8g2.print("F");
  u8g2.print(txFrames);
  u8g2.setCursor(78, 20);
  u8g2.print("B");
  u8g2.print(txBursts);

  switch (Svc) {
    case 1: {
      u8g2.setFont(u8g2_font_logisoso16_tr);
      if (Test == 0) {
        u8g2.setCursor(27, 22);
        u8g2.print("SERVICE");
        u8g2.setCursor(17, 40);
        u8g2.setFont(u8g2_font_helvR10_tf);
        u8g2.println("no signal from");
        u8g2.setCursor(10, 58);
        u8g2.println("Pos & Sta relays");
      } else {
        u8g2.setCursor(35, 22);
        u8g2.print("TEST svc1");
        u8g2.setFont(u8g2_font_helvR10_tf);
        u8g2.setCursor(2, 40);
        u8g2.print("pos.st.oc.u/d.svc");
        u8g2.setCursor(2, 58);
        u8g2.print("POS="); u8g2.print(Pos);
        u8g2.print(" DST="); u8g2.print(Des);
      }
      break;
    }

    case 2: {
      u8g2.setFont(u8g2_font_logisoso16_tr);
      if (Test == 0) {
        u8g2.setCursor(35, 22);
        u8g2.print("REVIZIE");
        u8g2.setFont(u8g2_font_helvR10_tf);
        u8g2.setCursor(2, 40);
        u8g2.print("pos.st.oc.u/d.svc");
        u8g2.setCursor(2, 58);
        u8g2.print("POS="); u8g2.print(Pos);
        u8g2.print(" DST="); u8g2.print(Des);
      } else {
        u8g2.setCursor(35, 22);
        u8g2.print("TEST  rev");
        u8g2.setFont(u8g2_font_helvR10_tf);
        u8g2.setCursor(2, 40);
        u8g2.print("pos.st.oc.u/d.svc");
        u8g2.setCursor(2, 58);
        u8g2.print("POS="); u8g2.print(Pos);
        u8g2.print(" DST="); u8g2.print(Des);
      }
      break;
    }

    case 3: {
      u8g2.setFont(u8g2_font_logisoso16_tr);
      if (Test == 0) {
        u8g2.setCursor(27, 22);
        u8g2.print("SERVICE");
        u8g2.setCursor(17, 40);
        u8g2.setFont(u8g2_font_helvR10_tf);
        u8g2.println("no signal from");
        u8g2.setCursor(10, 58);
        u8g2.println("  Com relays");
      } else {
        u8g2.setCursor(35, 22);
        u8g2.print("TEST  svc3");
        u8g2.setFont(u8g2_font_helvR10_tf);
        u8g2.setCursor(2, 40);
        u8g2.print("pos.st.oc.u/d.svc");
        u8g2.setCursor(2, 58);
        u8g2.print("POS="); u8g2.print(Pos);
        u8g2.print(" DST="); u8g2.print(Des);
      }
      break;
    }

    case 5: {
      if (Test == 0) {
        if (Display == LOW) {
          u8g2.setFont(u8g2_font_logisoso16_tr);
          u8g2.setCursor(6, 20);
          u8g2.print("Pos:");
          u8g2.print(Pos);
          u8g2.setCursor(77, 20);
          u8g2.print("Dst:");
          if (Sumsta == 17) {
            u8g2.print(Des);
          } else {
            u8g2.print("--");
          }
          u8g2.setFont(u8g2_font_helvR08_tf);
          u8g2.setCursor(32, 35);
          u8g2.print("ocp.        up / dwn");
          u8g2.setFont(u8g2_font_open_iconic_all_2x_t);
          u8g2.setCursor(32, 58);
          if (Ocp == 1) {
            u8g2.print("\u0081");
          } else {
            u8g2.print("\u0079");
          }
          u8g2.setCursor(88, 58);
          if (Sj == 1) {
            u8g2.print("\u008f");
          } else if (Sj == 2) {
            u8g2.print("\u008e");
          } else {
            u8g2.print("\u0079");
          }
        }
      } else {
        u8g2.setFont(u8g2_font_logisoso16_tr);
        u8g2.setCursor(25, 22);
        u8g2.print("TEST  nor");
        u8g2.setFont(u8g2_font_helvR10_tf);
        u8g2.setCursor(2, 40);
        u8g2.print("pos. st. oc. u/d. svc");
        u8g2.setCursor(2, 58);
        u8g2.print("POS="); u8g2.print(Pos);
        u8g2.print(" DST="); u8g2.print(Des);
      }
      break;
    }
  }

  u8g2.sendBuffer();

  // ========== 6. LOGICĂ TRANSMISIE EVENT-DRIVEN ==========
  bool stateChanged = (Pos != txPos) || (Des != txDes) || (Ocp != txOcp) ||
                      (Sj != txSj)   || (Svc != txSvc);

  if (stateChanged) {
    txPos = Pos;
    txDes = Des;
    txOcp = Ocp;
    txSj = Sj;
    txSvc = Svc;

    burstRemaining = BURST_COUNT;
    transmitFrames(txPos, txDes, txOcp, txSj, txSvc);
    burstRemaining--;

    lastTxMillis = now;
    lastBurstMillis = now;
  }
  else if (burstRemaining > 0 && (now - lastBurstMillis >= BURST_GAP_MS)) {
    transmitFrames(txPos, txDes, txOcp, txSj, txSvc);
    burstRemaining--;
    lastBurstMillis = now;
    lastTxMillis = now;
  }
  else if (burstRemaining == 0 && (now - lastTxMillis >= HEARTBEAT_MS)) {
    transmitFrames(txPos, txDes, txOcp, txSj, txSvc);
    lastTxMillis = now;
  }
}
