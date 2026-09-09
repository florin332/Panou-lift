Iată detalierea comenzilor seriale implementate în BatteryStub.cpp:

bat critical
Setează nivelul bateriei exact la pragul _thresholdCritical (default: 10%).
Dezactivează încărcarea.
Iată detalierea comenzilor seriale implementate în `BatteryStub.cpp`:

### `bat critical`
- Setează nivelul bateriei exact la pragul `_thresholdCritical` (default: `10%`).
- Dezactivează încărcarea.
- Stare rezultată: **`BatteryState::CRITICAL`**.
- Efect în UI: pagina Startup blochează butonul START; dacă sistemul era în altă pagină, `main.cpp` revine forțat la Battery Check.

### `bat low`
- Setează nivelul bateriei exact la pragul `_thresholdLow` (default: `20%`).
- Dezactivează încărcarea.
- Stare rezultată: **`BatteryState::LOW`**.
- Efect în UI: pagina Startup blochează butonul START (avertizare, fără teste noi), dar testele în desfășurare continuă.

### `bat medium`
- Setează nivelul bateriei exact la pragul `_thresholdMedium` (default: `60%`).
- Dezactivează încărcarea.
- Stare rezultată: **`BatteryState::NORMAL`** (deoarece nivelul este deasupra pragului LOW).
- Efect în UI: Battery Check permite avansarea automată către START după timeout.

### `bat good`
- Setează nivelul bateriei la mijlocul intervalului MEDIUM–FULL (default: `(60 + 100) / 2 = 80%`).
- Dezactivează încărcarea.
- Stare rezultată: **`BatteryState::NORMAL`**.
- Efect în UI: Battery Check permite avansarea automată către START după timeout.

### `bat normal`
- Setează nivelul bateriei la mijlocul intervalului LOW–MEDIUM (default: `(20 + 60) / 2 = 40%`).
- Dezactivează încărcarea.
- Stare rezultată: **`BatteryState::NORMAL`**.
- Efect în UI: Battery Check permite avansarea automată către START după timeout.

### `bat full`
- Setează nivelul bateriei exact la pragul `_thresholdFull` (default: `100%`).
- Activează încărcarea.
- Stare rezultată: **`BatteryState::CHARGING`**.
- Efect în UI: Battery Check NU avansează automat; rămâne în Battery Check ca să indice încărcarea. START este permis (CHARGING ≠ LOW/CRITICAL).

---

### Observații
- Toate comenzile persistă nivelul și starea de încărcare în EEPROM.
- Comanda `bat thresholds <c> <l> <m> <f>` permite modificarea runtime a celor 4 praguri; comenzile de mai sus reacționează imediat la noile valori.
- Starea finală este determinată de `BatteryStub::getState()` în ordinea: `CHARGING` → `CRITICAL` → `LOW` → `NORMAL`.
- „Dezactivează încărcarea” este doar un fag intern in stub: simulează că bateria nu mai este conectată la încărcător.