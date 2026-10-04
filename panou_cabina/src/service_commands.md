# Comenzi Service

Comenzile se trimit pe serial ca linii terminate cu `CR` sau `LF`. Parserul este case-sensitive și ignoră spațiile de la începutul liniei. Baud rate-ul configurat este `115200`.

Comenzile marcate **necesită Service activ** răspund cu `ERR 03 NOT_IN_SERVICE` dacă Service Mode nu este activ. Coada de comenzi acceptă o singură comandă în așteptare; o comandă queued primită cât timp coada este ocupată răspunde cu `ERR 02 BUSY`. O comandă nerecunoscută răspunde cu `ERR 01 UNKNOWN_CMD`.

## Mod Service

| Comandă | Alias | Cerință | Răspuns / efect |
| --- | --- | --- | --- |
| `SRV ENTER` | `mb_in` | Oricând | Activează Service Mode; `ACK SRV ENTER`. |
| `SRV EXIT` | `mb_out` | Oricând | Dezactivează Service Mode; `ACK SRV EXIT`. |

## Comunicație lifturi

| Comandă | Alias | Cerință | Răspuns / efect |
| --- | --- | --- | --- |
| `COMM TEST` | `mb_com` | Service activ | Afișează pe ILI9341 poziția, destinația, direcția, starea service și ocuparea ascensorului unic. Ecranul rămâne pe test până la ieșire; `OK mb_com L1=DATA`. |
| `mb_com_out` | - | Service activ | Închide testul de comunicație și restaurează pagina principală Service; `ACK mb_com_out`. |
| `mb_status` | - | Service activ | Trimite starea service a ascensorului, de forma `OK STATUS L1=...`. |
| `mb_com_status` | - | Service activ | Trimite contoarele RX ale ascensorului: cadre, valide, timeout, erori de format, CRC și date, plus `ON`/`OFF`. Format: `OK COMM STATUS L1=... ON|OFF`. |
| `mb_com_status_out` | - | Oricând | Nu schimbă ecranul; răspunde `ACK COMM STATUS OUT`. |
| `mb_diag` | - | Service activ | Trimite `OK DIAG SEQ=<coliziuni> RESET=<motiv>`. |

### Contoare RX

Aceste comenzi sunt executate direct în parserul serial de pe Core 1 și necesită Service Mode activ. În afara Service Mode răspund cu `ERR 03 NOT_IN_SERVICE`.

| Comandă | Efect / răspuns |
| --- | --- |
| `comm_count_enable` | Pornește numărarea; `ACK COMM COUNT ON`. |
| `comm_count_disable` | Oprește numărarea și resetează contoarele; `ACK COMM COUNT OFF`. |
| `comm_count_reset` | Resetează contoarele fără să schimbe starea ON/OFF; `ACK COMM COUNT RESET`. |

### Analiză brută RX

Ambele comenzi necesită Service Mode activ. Captura începe cu următorul marker START primit după enable. Cadrele complete sunt retransmise fără modificarea octeților, cu prefixul `RAW L1:` și newline la final. Cadrele întrerupte de resynchronizare sau timeout sunt prefixate cu `RAW L1 PARTIAL:`. Captura este limitată la 96 octeți per cadru; depășirea este indicată cu `[TRUNCATED]`. La disable se emite și orice cadru parțial rămas.

| Comandă | Efect / răspuns |
| --- | --- |
| `com_analyze_enable` | Activează retransmiterea datelor seriale recepționate; `ACK COM ANALYZE ON`. |
| `com_analyze_disable` | Oprește retransmiterea; `ACK COM ANALYZE OFF`. |

## Display

| Comandă | Alias | Parametri | Cerință | Răspuns / efect |
| --- | --- | --- | --- | --- |
| `DISP TEST <id>` | `mb_display_test <id>` | `1` | Service activ | Afișează testul pe ILI9341; `OK DISP TEST 1`. Alt ID: `ERR 05 INVALID_DISPLAY`. |
| `DISP REINIT <id>` | `mb_display_reinit <id>` | `1` | Service activ | Răspunde `OK DISP REINIT 1`. În implementarea actuală nu reinițializează efectiv controlerul display. |
| `TEST EXIT` | `mb_test_out` | - | Service activ | Închide testul de display și restaurează pagina principală Service; `ACK mb_com_out`. |

## Informații MCU

Toate comenzile din această secțiune necesită Service activ.

| Comandă | Alias | Răspuns |
| --- | --- | --- |
| `MCU UPTIME` | `mb_runtime` | `OK MCU UPTIME <secunde>`. |
| `MCU RESETS` | `mb_resets` | `OK MCU RESETS <număr>`. |
| `MCU WDT` | `mb_wdt` | `OK MCU WDT ENABLED` în implementarea actuală. |
| `MCU LASTRESET` | `mb_lastreset` | `OK MCU LASTRESET <POR|BOR|WDT|SYSREQ|UNKNOWN>`. |
| `MCU TEMP` | `mb_temp` | `OK MCU TEMP <grade Celsius>`. |
| `MCU STACK <core>` | `mb_stack <core>` | `core` trebuie să fie `0` sau `1`; răspunde cu `OK MCU STACK <core> USED=... FREE=... HW=...`. Alt ID: `ERR 07 INVALID_CORE`. |

## Erori comune

| Răspuns | Semnificație |
| --- | --- |
| `ERR 01 UNKNOWN_CMD` | Comandă necunoscută sau scrisă cu altă capitalizare. |
| `ERR 02 BUSY` | Coada de comenzi este ocupată. |
| `ERR 03 NOT_IN_SERVICE` | Comanda necesită Service Mode activ. |
| `ERR 04 UNHANDLED` | Tip de comandă fără handler. |
| `ERR 05 INVALID_DISPLAY` | ID display invalid. |
| `ERR 06 SEQLOCK` | Nu s-a putut citi un snapshot coerent din memoria partajată. |
| `ERR 07 INVALID_CORE` | ID core diferit de `0` sau `1`. |
