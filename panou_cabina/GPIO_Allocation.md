# Alocarea Pinilor GPIO - Panou Cabina

> Revizie audio/SD: LCD-ul si cititorul SD onboard impart SPI0; W5500 ramane pe SPI1. GP0/GP1/GP4/GP5, folositi anterior pentru CH9121 local, sunt eliberati.

## Rezumat
Maparea fizică pentru panoul de cabină cu un singur ecran ILI9341 pe Raspberry Pi Pico.

---

## 1. Ecran ILI9341 (SPI0)
| Pin | GPIO | Funcție |
|-----|------|---------|
| CS  | 22   | Chip Select |
| DC  | 21   | Data/Command Select |
| RST | 20   | Reset |
| MOSI| 19   | Master Out Slave In (SPI Data) |
| SCK | 18   | Serial Clock (SPI Clock) |

## 1a. Card SD onboard Marble Pico (SPI0 partajat)
| Semnal | GPIO | Funcție |
|--------|------|---------|
| MISO / DT0 | 16 | Date SD catre RP2040 |
| CS / DT3 | 17 | Chip Select card SD |
| SCK / CLK | 18 | Ceas SPI partajat cu LCD |
| MOSI / CMD | 19 | Date RP2040 catre SD; partajat cu LCD |

Cardul SD si LCD-ul folosesc SPI0 cu CS-uri separate (SD GP17, LCD GP22). Tinta este SD la maximum 20 MHz si LCD la 40 MHz; CS-urile ambelor dispozitive trebuie mentinute inactive (HIGH) inainte de initializarea perifericelor.

---

## 2. Ethernet W5500 (SPI1)
| Pin | GPIO | Funcție |
|-----|------|---------|
| MISO | 12 | SPI1 RX |
| CS | 13 | Chip Select |
| SCK | 14 | SPI1 SCK |
| MOSI | 15 | SPI1 TX |

Tinta este ca W5500 sa fie accesat numai de Core 0, independent de SPI0.

RESET si INT raman nealocate pana la confirmarea modulului W5500 ales.

### Verificare conflicte
- LCD-ul si SD-ul impart SPI0 si necesita acces serializat; numai Core 1 va accesa aceasta magistrala in arhitectura tinta.
- W5500 ramane pe SPI1, fara partajare cu LCD-ul sau SD-ul; accesul este rezervat Core 0 in arhitectura tinta.
- I2S ramane pe GP8-GP10 si este rezervat procesarii audio pe Core 0 in arhitectura tinta.
- **Modificare:** conexiunile UART locale CH9121 (GP0/GP1/GP4/GP5) sunt eliminate si pinii devin liberi. CH9121-urile echipamentelor remote nu ocupa GPIO pe panou.

## 3. UI - Interfață Utilizator
| Funcție | GPIO | Descriere |
|---------|------|-----------|
| BACKLIGHT | 26 | Control iluminare spate ecran |

---

## 4. UART - Comunicație Serie
| Funcție | GPIO | Descriere |
|---------|------|-----------|
| UART1_TX | 0 | Liber - UART CH9121 local eliminat |
| UART1_RX | 1 | Liber - UART CH9121 local eliminat |
| UART2_TX | 4 | Liber - UART CH9121 local eliminat |
| DATA_RX | 5 | Liber - UART CH9121 local eliminat |

---

## 5. Intrări Digitale (Inputs)
| Funcție | GPIO | Descriere |
|---------|------|-----------|
| RESET | 11 | Intrare cu pull-up; buton normal deschis la GND, 4 apăsări în 2 s pentru reset |
| ALARM | 7 | Intrare semnal alarmă |
| OVERLOAD | 6 | Intrare detectare supraîncărcare |

---

## 6. Audio (I2S)
| Funcție | GPIO | Descriere |
|---------|------|-----------|
| I2S_BCLK  | 8  | Bit Clock; tinta PIO + DMA |
| I2S_LRCLK | 9  | Left/Right Clock |
| I2S_DATA  | 10 | Serial Data; mono 16 kHz, 16-bit, duplicat pe L/R |

## 7. Arhitectura dual-core tinta
- **Core 0:** procesare audio in timp real, consumul bufferului audio, tratarea intreruperilor GPIO ALARM/OVERLOAD/RESET si accesul la W5500 prin SPI1. Core 0 nu acceseaza SPI0.
- **Core 1:** acces exclusiv la SPI0 pentru SD si LCD; valideaza fisierele si alimenteaza bufferul audio, cu prioritate pentru citirile SD.
- Transferul audio intre nuclee se face printr-un buffer circular in RAM, cu protocol de sincronizare sigur pentru producator/consumator pe nuclee diferite.
- **Stare curenta:** aceasta separare nu este implementata. `main.cpp` apeleaza `Application::runCore1()` si `Application::runCore0()` secvential din aceeasi `loop()`; denumirile metodelor nu indica executie pe nuclee hardware distincte.

---

## Rezumat Pinuri folosite
- **Total GPIO-uri utilizate:** 18
- **Pinuri libere (teoretic):** 13 (pe Raspberry Pi Pico care are 30 GPIO în total)

### Pinuri alocate
6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 26

### Pinuri disponibile
0, 1, 2, 3, 4, 5, 23, 24, 25, 27, 28, 29
