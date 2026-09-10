
Iată detalierea comenzilor seriale implementate în `BatteryStub.cpp`:

### `bat 10`
- STUB Setează nivelul bateriei exact la pragul `_thresholdCritical` (default: `10%`).
- Stare rezultată: **`BatteryState::CRITICAL`**.
- Efect în UI: pagina Startup blochează butonul START; dacă sistemul era în altă pagină, `main.cpp` revine forțat la Battery Check.

### `bat 30`
- STUB Setează nivelul bateriei exact la pragul `_thresholdLow` (default: `30%`).
- Stare rezultată: **`BatteryState::LOW`**.
- Efect în UI: pagina Startup blochează butonul START (avertizare, fără teste noi), dar testele în desfășurare continuă.

### `bat 60` - folosit numai pentru stub
- Setează nivelul bateriei exact la pragul `_thresholdMedium` (default: `60%`).
- Stare rezultată: **`BatteryState::NORMAL`** (deoarece nivelul este deasupra pragului LOW).
- Efect în UI: Battery Check permite avansarea automată către START după timeout.

### `bat 80`  - folosit numai pentru stub
- Setează nivelul bateriei la mijlocul intervalului MEDIUM–FULL (default: `(60 + 100) / 2 = 80%`).
- Stare rezultată: **`BatteryState::NORMAL`**.
- Efect în UI: Battery Check permite avansarea automată către START după timeout.

### `bat 40` - folosit numai pentru stub
- Setează nivelul bateriei la mijlocul intervalului LOW–MEDIUM (default: `(20 + 60) / 2 = 40%`).
- Dezactivează încărcarea NUMAI IN STUB !!
- Stare rezultată: **`BatteryState::NORMAL`**.
- Efect în UI: Battery Check permite avansarea automată către START după timeout.

### `bat 100`
- Setează nivelul bateriei exact la pragul `_thresholdFull` (default: `100%`).
- Stare rezultată: **`BatteryState::CHARGING`**.
- Efect în UI: Battery Check NU avansează automat; rămâne în Battery Check ca să indice încărcarea. START este permis (CHARGING ≠ LOW/CRITICAL).

---

### Observații
- Toate comenzile persistă nivelul și starea de încărcare în EEPROM.
- Comanda `bat thresholds <c> <l> <m> <f>` permite modificarea runtime a celor 4 praguri; comenzile de mai sus reacționează imediat la noile valori.
- Starea finală este determinată de `BatteryStub::getState()` în ordinea: `CHARGING` → `CRITICAL` → `LOW` → `NORMAL`.
- „Dezactivează încărcarea” este doar un flag intern in stub: simulează că bateria nu mai este conectată la încărcător.