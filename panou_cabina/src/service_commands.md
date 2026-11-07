# Comenzi Service — Panou Cabină

Comenzile se trimit pe serial ca linii terminate cu `CR` sau `LF`. Parserul este case-sensitive și ignoră spațiile de la începutul liniei. Baud rate-ul configurat este `115200`.

Specificația țintă (sursa de adevăr) este `service-box2/src/service_commands.md`; acest document reflectă protocolul aplicat pe **panou cabină** și lista TODO de implementare.

## Schemă de numerotare

| Grup | Interval | Conținut |
| --- | --- | --- |
| 0 — Comenzi principale | `mb_00` – `mb_01` | Intrare / ieșire Service Mode |
| 1 — Flash firmware | `mb_02` | Reset în bootloader USB pentru programare |
| 2 — Setări persistente | `mb_03` – `mb_05` | Număr ascensor, gain audio, default-uri |
| 6 — COMM și contoare RX | `mb_61` – `mb_65` | Afișare și control contoare de comunicație |

Parserul recunoaște **exclusiv** forma `mb_NN`; nu există aliasuri.

**Convenție de răspuns:** confirmări cu `ACK_NN` (`NN` = numărul comenzii); interogările răspund cu `OK ...`.

Comenzile marcate **necesită Service activ** răspund cu `ERR 03 NOT_IN_SERVICE` dacă Service Mode nu este activ. Coada de comenzi acceptă o singură comandă în așteptare; o comandă queued primită cât timp coada este ocupată răspunde cu `ERR 02 BUSY`. O comandă nerecunoscută răspunde cu `ERR 01 UNKNOWN_CMD`.

## Grup 0 — Comenzi principale

| Comandă | Cerință | Răspuns / efect |
| --- | --- | --- |
| `mb_01` | Oricând | Activează Service Mode; identifică panoul, de exemplu `C_01_1.01` (tip, variantă hardware, variantă firmware). |
| `mb_00` | Oricând | Dezactivează Service Mode; `ACK_00`. |

## Grup 1 — Flash firmware

| Comandă | Cerință | Răspuns / efect |
| --- | --- | --- |
| `mb_02` | Service activ | `ACK_02`, apoi reset în bootloader USB (dispozitivul se re-enumerază ca volum MSC `RPI-RP2`, pentru flash-uire firmware de pe Service Box). ACK-ul poate lipsi dacă resetul survine înainte de golirea FIFO-ului USB; confirmarea reală pentru box este detach-ul CDC. |

## Grup 2 — Setări persistente (flash protejat)

Valorile sunt stocate în zona de flash protejată (ultimul sector, în afara imaginii de firmware) și **nu se pierd la update-ul de firmware prin .uf2**. Toate comenzile din această secțiune necesită Service Mode activ și sunt executate direct în parserul serial.

Argumentul numeric poate fi trimis simplu (`mb_04 0.60`) sau încadrat de markerele protocolului (`mb_04 <0.60>`, `mb_04<0.60>`) — markerele sunt eliminate la parsare.

| Comandă | Efect / răspuns |
| --- | --- |
| `mb_03` | Interoghează numărul ascensorului: `OK LIFT <n>`. |
| `mb_03 <n>` | Setează numărul ascensorului (`1`–`65535`) și îl salvează în flash; `ACK_03 <n>`. Valori invalide: `ERR 08 INVALID_PARAM`. |
| `mb_04` | Interoghează gain-ul audio: `OK GAIN <x.xx>`. |
| `mb_04 <x.xx>` | Setează gain-ul audio (`0.00`–`4.00`), îl aplică imediat pe ieșirea I2S și îl salvează în flash; `ACK_04 <x.xx>`. Valori invalide: `ERR 08 INVALID_PARAM`. |
| `mb_05` | Revine la default-urile din `Config.h` (număr ascensor + gain) și le persistă; `ACK_05`. |

## Grup 6 — COMM și contoare RX

Panoul de cabină are un singur canal RX, deci Service Box trimite ținta `1`.

| Comandă | Efect / răspuns |
| --- | --- |
| `mb_61 1` | Afișează pe LCD contoarele RX `Total`, `Valid`, `Timeout`, `Format`, `CRC`, `Data`; `ACK_61 1`. Necesită Service activ. |
| `mb_62 1` | Închide pagina contoarelor și revine la afișarea normală; `ACK_62 1`. Necesită Service activ. |
| `mb_63 1` | Pornește numărarea; `ACK_63 1`. |
| `mb_64 1` | Oprește numărarea fără să șteargă valorile; `ACK_64 1`. |
| `mb_65 1` | Resetează valorile fără să schimbe starea ON/OFF; `ACK_65 1`. |

## Erori comune

| Răspuns | Semnificație |
| --- | --- |
| `ERR 01 UNKNOWN_CMD` | Comandă necunoscută, neimplementată pe acest tip de panou sau scrisă cu altă capitalizare. |
| `ERR 02 BUSY` | Coada de comenzi este ocupată. |
| `ERR 03 NOT_IN_SERVICE` | Comanda necesită Service Mode activ. |
| `ERR 08 INVALID_PARAM` | Parametru în afara intervalului valid (număr ascensor sau gain). |

## TODO implementare

Modificări necesare în `src/Core/Services/ServiceProtocol.cpp` și `src/Presentation/ServiceMenu.cpp` pentru alinierea la specificație:

- [ ] **`mb_02` (critic):** parserul acceptă azi doar `SRV BOOTSEL` / `mb_boot` și răspunde `ACK SRV BOOTSEL`. De adăugat recunoaștere `mb_02` și răspuns `ACK_02`; de eliminat formele vechi. Box-ul trimite deja exclusiv `mb_02` — **flash-ul din box nu funcționează până la această modificare**. Markerii de diagnosticare `DBG P0/S1/S3/S4` rămân neschimbați (box-ul îi ignoră la așteptarea ACK-ului).
- [ ] **`mb_01` / `mb_00`:** păstrate, dar `mb_00` trebuie să răspundă `ACK_00` (azi `ACK SRV EXIT`). De eliminat aliasurile `SRV ENTER` / `SRV EXIT`.
- [ ] **`mb_03`:** redenumire `mb_lift` → `mb_03`; interogare `OK LIFT <n>`, setare `ACK_03 <n>` (azi `ACK LIFT <n>`).
- [ ] **`mb_04`:** redenumire `mb_gain` → `mb_04`; setare `ACK_04 <x.xx>` (azi `ACK GAIN <x.xx>`). Formatarea manuală `X.XX` (newlib-nano fără `%f`) rămâne.
- [ ] **`mb_05`:** redenumire `mb_defaults` → `mb_05`, răspuns `ACK_05`. Atenție: handlerul actual oprește redarea audio și reaplică gain-ul — comportamentul se păstrează.
- [ ] **De eliminat din parser** până la definirea grupurilor viitoare: `COMM TEST`/`mb_com`, `mb_com_out`, `mb_diag`, `mb_com_status`, `mb_com_status_out`, `MCU *`. Vor răspunde temporar `ERR 01 UNKNOWN_CMD`.
