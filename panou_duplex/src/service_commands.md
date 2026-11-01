# Comenzi Service — Panou Duplex

Comenzile se trimit pe serial ca linii terminate cu `CR` sau `LF`. Parserul este case-sensitive și ignoră spațiile de la începutul liniei. Baud rate-ul configurat este `115200`.

Specificația țintă (sursa de adevăr) este `service-box2/src/service_commands.md`; acest document reflectă protocolul aplicat pe **panou duplex** și lista TODO de implementare.

## Schemă de numerotare

| Grup | Interval | Conținut |
| --- | --- | --- |
| 0 — Comenzi principale | `mb_00` – `mb_01` | Intrare / ieșire Service Mode |
| 1 — Flash firmware | `mb_02` | Reset în bootloader USB pentru programare |
| 2 — Setări persistente | `mb_03` – `mb_05` | Etaj panou, default-uri |

Parserul recunoaște **exclusiv** forma `mb_NN`; nu există aliasuri.

**Convenție de răspuns:** confirmări cu `ACK_NN` (`NN` = numărul comenzii); interogările răspund cu `OK ...`.

Comenzile marcate **necesită Service activ** răspund cu `ERR 03 NOT_IN_SERVICE` dacă Service Mode nu este activ. Coada de comenzi acceptă o singură comandă în așteptare; o comandă queued primită cât timp coada este ocupată răspunde cu `ERR 02 BUSY`. O comandă nerecunoscută răspunde cu `ERR 01 UNKNOWN_CMD`.

## Grup 0 — Comenzi principale

| Comandă | Cerință | Răspuns / efect |
| --- | --- | --- |
| `mb_01` | Oricând | Activează Service Mode; trimite identificatorul `D_<hardware>_<major>.<minor>` (curent `D_01_10.28`). |
| `mb_00` | Oricând | Dezactivează Service Mode; `ACK_00`. |

## Grup 1 — Flash firmware

| Comandă | Cerință | Răspuns / efect |
| --- | --- | --- |
| `mb_02` | Service activ | `ACK_02`, apoi reset în bootloader USB (dispozitivul se re-enumerază ca volum MSC `RPI-RP2`, pentru flash-uire firmware de pe Service Box). Secvența e atomică: ACK + `Serial.flush()`, ~100 ms pentru golirea FIFO-ului USB, apoi `noInterrupts()` și `reset_usb_boot(0, 0)` direct, fără alte așteptări. Confirmarea reală pentru box este detach-ul CDC. |

## Grup 2 — Setări persistente (flash protejat)

Valorile sunt salvate în ultimul sector de flash (EEPROM emulat), în afara imaginii de firmware, deci supraviețuiesc update-urilor `.uf2`. Argumentul numeric poate fi trimis simplu (`mb_03 3`) sau încadrat de markerele protocolului (`mb_03 <3>`, `mb_03<3>`) — markerele sunt eliminate la parsare. Toate comenzile din această secțiune necesită Service activ.

| Comandă | Efect / răspuns |
| --- | --- |
| `mb_03` | Interoghează etajul unde e montat panoul: `OK ETJ <n>`. |
| `mb_03 <n>` | Setează etajul panoului (`0`–`17`) și îl salvează în flash; `ACK_03 <n>`. Valori invalide: `ERR 08 INVALID_PARAM`. |
| `mb_04` | Rezervat (gain audio, doar cabină); pe duplex răspunde `ERR 01 UNKNOWN_CMD`. |
| `mb_05` | Revine la default-urile din `Config.h` (etaj + standby) și le persistă; `ACK_05`. |

## Erori comune

| Răspuns | Semnificație |
| --- | --- |
| `ERR 01 UNKNOWN_CMD` | Comandă necunoscută, neimplementată pe acest tip de panou sau scrisă cu altă capitalizare. |
| `ERR 02 BUSY` | Coada de comenzi este ocupată. |
| `ERR 03 NOT_IN_SERVICE` | Comanda necesită Service Mode activ. |
| `ERR 08 INVALID_PARAM` | Parametru în afara intervalului valid. |

## TODO implementare

Modificări necesare în `src/Core/Services/ServiceProtocol.cpp` și `src/Presentation/ServiceMenu.cpp` pentru alinierea la specificație:

- [x] **`mb_02` (critic):** parserul acceptă azi doar `SRV BOOTSEL` / `mb_boot` și răspunde `ACK SRV BOOTSEL`. De adăugat recunoaștere `mb_02` și răspuns `ACK_02`; de eliminat formele vechi. Box-ul trimite deja exclusiv `mb_02` — **flash-ul din box nu funcționează până la această modificare**. Markerii de diagnosticare `DBG P0/S1/S3/S4` rămân neschimbați (box-ul îi ignoră la așteptarea ACK-ului).
- [x] **`mb_01` / `mb_00`:** păstrate, dar `mb_00` trebuie să răspundă `ACK_00` (azi `ACK SRV EXIT`). De eliminat aliasurile `SRV ENTER` / `SRV EXIT`.
- [ ] **`mb_03`:** redenumire `mb_etj` → `mb_03`; interogare `OK ETJ <n>`, setare `ACK_03 <n>` (azi `ACK ETJ <n>`).
- [ ] **`mb_05`:** redenumire `mb_defaults` → `mb_05`, răspuns `ACK_05` (azi `ACK DEFAULTS`).
- [ ] **Standby timeout:** `mb_standby` existent nu are echivalent în spec — de decis: mapare pe `mb_04` (rezervat gain-ului la cabină) sau comandă separată într-un grup viitor.
- [ ] **De eliminat din parser** până la definirea grupurilor viitoare: `COMM TEST`/`mb_com`, `mb_com_out`, `mb_status`, `mb_diag`, `mb_com_status`, `mb_com_status_out`, `comm_count_*`, `com_analyze_*`, `DISP TEST/REINIT`, `TEST EXIT`, `MCU *`. Vor răspunde temporar `ERR 01 UNKNOWN_CMD`.
