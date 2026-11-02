# Comenzi Service — Panou Duplex

Comenzile se trimit pe serial ca linii terminate cu `CR` sau `LF`. Parserul este case-sensitive și ignoră spațiile de la începutul liniei. Baud rate-ul configurat este `115200`.

Specificația țintă (sursa de adevăr) este `service-box2/src/service_commands.md`; acest document reflectă protocolul aplicat pe **panou duplex** și lista TODO de implementare.

## Schemă de numerotare

| Grup | Interval | Conținut |
| --- | --- | --- |
| 0 — Comenzi principale | `mb_00` – `mb_01` | Intrare / ieșire Service Mode |
| 1 — Flash firmware | `mb_02` | Reset în bootloader USB pentru programare |
| 2 — Setări persistente | `mb_11` – `mb_30` | Etaj panou, standby ecran, al doilea apel, default-uri |
| 3 — LCD service | `mb_16` – `mb_18` | Reinițializare și test culori LCD1/LCD2 |

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
| `mb_02` | Service activ | `ACK_02`, apoi reset în bootloader USB (dispozitivul se re-enumerază ca volum MSC `RPI-RP2`, pentru flash-uire firmware de pe Service Box). Execuția rulează pe core0: ACK, ~170 ms pentru golirea FIFO-ului USB (fără `Serial.flush()` — deadlock pe mutex-ul TinyUSB), oprirea core1 (`multicore_reset_core1`), pornirea tick-ului watchdog și `reset_usb_boot(0, 0)`. Confirmarea reală pentru box este apariția volumului `RPI-RP2`. |

## Grup 3 — LCD service

| Comandă | Cerință | Răspuns / efect |
| --- | --- | --- |
| `mb_16 <x>` | Service activ | Reinițializează controlerul LCD1 pentru `x=1`, LCD2 pentru `x=2` sau ambele pentru `x=3`; `ACK_16 <x>`. Alte valori: `ERR 08 INVALID_PARAM`. |
| `mb_17 <x>` | Service activ | Pornește testul continuu de culori pe LCD1 pentru `x=1`, LCD2 pentru `x=2` sau ambele pentru `x=3`; fiecare culoare durează 3 s, ordinea este red/green/blue/white/black, iar eticheta alternează sus/jos după fiecare tură completă; `ACK_17 <x>`. |
| `mb_18` | Service activ | Oprește testul pe toate LCD-urile și revine la afișarea datelor; `ACK_18`. |

## Grup 2 — Setări persistente (flash protejat)

Valorile sunt salvate în ultimul sector de flash (EEPROM emulat), în afara imaginii de firmware, deci supraviețuiesc update-urilor `.uf2`. Argumentul numeric poate fi trimis simplu (`mb_11 3`) sau încadrat de markerele protocolului (`mb_11 <3>`, `mb_11<3>`) — markerele sunt eliminate la parsare. Toate comenzile din această secțiune necesită Service activ.

| Comandă | Efect / răspuns |
| --- | --- |
| `mb_11` | Interoghează etajul unde e montat panoul: `OK ETJ <n>`. |
| `mb_11 <n>` | Setează etajul panoului (`0`–`FLOORS-1`) și îl salvează în flash; `ACK_11 <n>`. Valori invalide: `ERR 08 INVALID_PARAM`. |
| `mb_12` | Rezervat (gain audio, doar cabină); pe duplex răspunde `ERR 01 UNKNOWN_CMD`. |
| `mb_13` | Interoghează permisiunea al doilea apel: `OK SECOND CALL <x>` (`0` = blocat, `1` = permis). |
| `mb_13 <x>` | Setează permisiunea al doilea apel (`0`/`1`) și o persistă în flash; `ACK_13 <x>`. Valori invalide: `ERR 08 INVALID_PARAM`. |
| `mb_14` | Interoghează timeout-ul standby ecran: `OK STANDBY <ms>`. |
| `mb_14 <ms>` | Setează timeout-ul standby ecran (ms) și îl persistă în flash; `ACK_14 <ms>`. Valori invalide: `ERR 08 INVALID_PARAM`. |
| `mb_15` | Interoghează ordinea cabinelor pe LCD-uri: `OK CAB ORDER <x>` (`0` = direct: cabina 1 pe LCD1/SPI0, cabina 2 pe LCD2/SPI1; `1` = invers). |
| `mb_15 <x>` | Setează ordinea cabinelor (`0`/`1`) și o persistă în flash; `ACK_15 <x>`. Se aplică fără reboot, cu redesenare imediată a ambelor ecrane. Valori invalide: `ERR 08 INVALID_PARAM`. |
| `mb_30` | Revine la default-urile din `Config.h` (etaj + standby + second call + ordine cabine) și le persistă; `ACK_30`. |

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
- [x] **`mb_11`:** redenumit `mb_etj` → `mb_11`; interogare `OK ETJ <n>`, setare `ACK_11 <n>`.
- [x] **`mb_13`:** permisiune al doilea apel (SecondCall) persistentă; interogare `OK SECOND CALL <x>`, setare `ACK_13 <x>`, default `0` (blocat). Logica apăsării lungi (3 s) este în `LiftController`.
- [x] **`mb_14`:** standby timeout mapat pe primul slot liber din grupul 2 (fostul `mb_standby`); interogare `OK STANDBY <ms>`, setare `ACK_14 <ms>`.
- [x] **`mb_15`:** ordine cabine pe LCD-uri (CabOrder) persistentă; interogare `OK CAB ORDER <x>`, setare `ACK_15 <x>`, default `0` (direct: cabina1→LCD1). La schimbare se invalidează ambele ecrane pentru redesenare imediată; meniul de diagnostic rămâne fix pe LCD1.
- [x] **`mb_30`:** redenumit `mb_defaults` → `mb_30`, răspuns `ACK_30`.
- [x] **Eliminat din parser** comenzile legacy: `COMM TEST`/`mb_com`, `mb_com_out`, `mb_status`, `mb_diag`, `mb_com_status`, `mb_com_status_out`, `comm_count_*`, `com_analyze_*`, `DISP TEST/REINIT`, `TEST EXIT`, `MCU *`. Răspund acum `ERR 01 UNKNOWN_CMD`. Handlerii asociați din `ServiceMenu.cpp` (teste display, COMM, MCU) au fost eliminați.
