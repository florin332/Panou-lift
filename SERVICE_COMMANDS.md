# Service and Magic Box Commands

Documentatie de referinta pentru comenzile trimise prin USB Serial catre Pico.

## Conectare

- Portul Pico: verifica portul USB detectat de Windows, de exemplu `COM23`
- Baud rate: `115200`
- Line ending: `Both NL & CR`
- Trimite o singura comanda pe linie si asteapta raspunsul inaintea urmatoarei comenzi.
- O comanda necunoscuta primeste `ERR 01 UNKNOWN_CMD`.
- Daca exista deja o comanda in asteptare, se primeste `ERR 02 BUSY`.

## Service mode

| Comanda scurta | Comanda veche | Explicatie |
|---|---|---|
| `mb_in` | `SRV ENTER` | Intra in Service Mode si activeaza chenarul de service pe TFT-uri. |
| `mb_out` | `SRV EXIT` | Iese din Service Mode si revine la randarea normala. |

Raspunsuri asteptate:

```text
ACK SRV ENTER
ACK SRV EXIT
```

Comenzile de mai jos necesita mai intai `mb_in`. Altfel se primeste:

```text
ERR 03 NOT_IN_SERVICE
```

## Communication

### Afisare pe TFT-uri

```text
mb_com
```

Citeste snapshot-ul comun si afiseaza datele liftului 1 pe TFT-ul stang si datele liftului 2 pe TFT-ul drept.

Afisarea include:

- `Pos :` - pozitia curenta, cu `P` pentru parter
- `Dst :` - destinatia; devine `--` cand `S/J` este `--`
- `S/J :` - `UP`, `DWN` sau `--`
- `Svc :` - `OK`, `Def.`, `Rev.` sau `No ser.`
- `Ocp :` - `YES` sau `NO`

Raspuns:

```text
OK mb_com L1=DATA L2=DATA
```

### Status pe LCD-ul Magic Box

```text
mb_status
```

Trimite catre Magic Box starea celor doua lifturi si numarul erorilor de pachete.

Exemplu:

```text
OK STATUS L1=OK L2=No ser. ERR1=0 ERR2=2
```

### Diagnostic pe LCD-ul Magic Box

```text
mb_diag
```

Trimite sumarul de erori de comunicatie, coliziuni seqlock si motivul ultimei resetari.

Exemplu:

```text
OK DIAG ERR1=0 ERR2=0 SEQ=0 RESET=1
```

## Display

### Test TFT

```text
mb_display_test 1
mb_display_test 2
```

Afiseaza `PASS` pe TFT-ul selectat.

Comenzi vechi echivalente:

```text
DISP TEST 1
DISP TEST 2
```

### Iesire din testul display / mb_com

```text
mb_com_out
```

Comanda veche echivalenta:

```text
TEST EXIT
```

Raspuns:

```text
ACK mb_com_out
```

### Reinitializare TFT

Forma intentionata:

```text
mb_display_reinit 1
mb_display_reinit 2
mb_display_reinit BOTH
```

Comenzi vechi echivalente:

```text
DISP REINIT 1
DISP REINIT 2
DISP REINIT BOTH
```

Observatie: in versiunea curenta, handler-ul `DISP REINIT` trimite raspunsul `OK`, dar reinitializarea hardware TFT este inca `TODO`. Aliasul `mb_display_reinit` necesita corectarea offsetului intern al parserului inainte de folosire sigura.

## PICO / MCU

Toate rezultatele sunt trimise pe serial catre LCD-ul Magic Box.

| Comanda | Comanda veche | Explicatie |
|---|---|---|
| `mb_runtime` | `MCU UPTIME` | Afiseaza uptime-ul Pico in secunde. |
| `mb_resets` | `MCU RESETS` | Afiseaza numarul de boot-uri/resetari memorate. |
| `mb_wdt` | `MCU WDT` | Afiseaza starea WDT; valoarea este momentan simulata ca activata. |
| `mb_lastreset` | `MCU LASTRESET` | Afiseaza motivul ultimei resetari. |
| `mb_temp` | `MCU TEMP` | Afiseaza temperatura interna a MCU. |
| `mb_stack 0` | `MCU STACK 0` | Afiseaza USED/FREE/HIGH WATER pentru Core 0; valorile sunt momentan simulate. |
| `mb_stack 1` | `MCU STACK 1` | Afiseaza USED/FREE/HIGH WATER pentru Core 1; valorile sunt momentan simulate. |

Exemple de raspunsuri:

```text
OK MCU UPTIME 123
OK MCU RESETS 4
OK MCU WDT ENABLED
OK MCU LASTRESET POR
OK MCU TEMP 31.4
OK MCU STACK 0 USED=2048 FREE=4096 HW=8192
```

## Secventa de test recomandata

```text
mb_in
mb_status
mb_diag
mb_com
mb_com_out
mb_display_test 1
mb_com_out
mb_display_test 2
mb_com_out
mb_runtime
mb_resets
mb_wdt
mb_lastreset
mb_temp
mb_stack 0
mb_stack 1
mb_out
```

## Protocolul RS485 al lifturilor

Comenzile Magic Box de mai sus folosesc USB Serial si nu modifica parserul RS485.

Parserul pentru lifturi este in `src/Core/Services/Protocol.cpp`:

- `Serial2` receptioneaza datele liftului 1.
- `Serial1` receptioneaza datele liftului 2.
- `parseazaPachet()` valideaza si transforma pachetul in `LiftState`.
