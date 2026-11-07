# Alocarea Pinilor GPIO - Panou Cabina

> Revizie W5500: pinii LCD pe SPI0 sunt pastrati. GP0/GP1/GP4/GP5, folositi anterior pentru CH9121 local, sunt eliberati; W5500 este alocat pe SPI1.

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

---

## 2. UI - Interfață Utilizator
## 2. Ethernet W5500 (SPI1) - NOU
| Pin | GPIO | Funcție |
|-----|------|---------|
| MISO | 12 | SPI1 RX |
| CS | 13 | Chip Select |
| SCK | 14 | SPI1 SCK |
| MOSI | 15 | SPI1 TX |

RESET si INT raman nealocate pana la confirmarea modulului W5500 ales.

### Verificare conflicte
- **Fara conflict** cu LCD-ul: ILI9341 ramane pe SPI0; W5500 foloseste SPI1.
- **Fara conflict** cu I2S: semnalele audio raman pe GP8-GP10.
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
| I2S_BCLK  | 8  | Bit Clock |
| I2S_LRCLK | 9  | Left/Right Clock |
| I2S_DATA  | 10 | Serial Data |

---

## Rezumat Pinuri folosite
- **Total GPIO-uri utilizate:** 16
- **Pinuri libere (teoretic):** 15 (pe Raspberry Pi Pico care are 30 GPIO în total)

### Pinuri alocate
6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 18, 19, 20, 21, 22, 26

### Pinuri disponibile
0, 1, 2, 3, 4, 5, 16, 17, 23, 24, 25, 27, 28, 29
