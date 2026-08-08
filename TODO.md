# TODO – Panou Lift

## 🔴 Prioritate mare

- [ ] Monitorizare stabilitate TFT1 / TFT2 pe termen lung

## 🟠 De urmărit în mediul real

- [x] V10.29 testat în mediu real
- [x] Verificare `Custom_ST7735`
- [x] Verificare `rowStart` / `colStart`
- [ ] Monitorizare stabilitate TFT1 / TFT2 pe termen lung
- [x] Timeout comunicație RS485 `500 ms` – OK pentru moment
- [ ] Watchdog Hardware (Critic pentru un lift)

## 🟡 Funcționalități

- [ ] Meniu Service – intrare
- [ ] Meniu Service – navigare
- [ ] Meniu Service – revenire la ecranul principal
- [ ] Meniu Developer – intrare
- [ ] Meniu Developer – navigare
- [ ] Meniu Developer – revenire la ecranul principal

## 🟢 Îmbunătățiri ulterioare

- [ ] Diagnostic mai bun pentru pierderile de pachete
- [ ] Monitorizare / jurnalizare evenimente RS485
- [ ] Curățare cod experimental rămas din versiunile anterioare
- [ ] Sistematizare / completare config
- [ ] Row start si Col start independent pt fiecare lcd
- [ ] Watchdog Hardware (Critic pentru un lift)

## ✅ Rezolvate

- [x] Flicker NORMAL ↔ SVC=3 eliminat prin citirea continuă UART + timeout bazat pe `millis()`
- [x] Upload PlatformIO / autodetect Pico
- [x] Configurare pin RCWL
- [x] Inițializare TFT la pornire
- [x] `Custom_ST7735` verificat
- [x] `rowStart` / `colStart` verificate
