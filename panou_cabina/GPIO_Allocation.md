# Alocarea Pinilor GPIO - Panou Cabina

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
| Funcție | GPIO | Descriere |
|---------|------|-----------|
| BACKLIGHT | 26 | Control iluminare spate ecran |

---

## 3. UART - Comunicație Serie
| Funcție | GPIO | Descriere |
|---------|------|-----------|
| ALARM_TX | 0 | Transmisie alarme |
| UART1_RX | 1 | Recepție UART1 |
| UART2_TX | 4 | Transmisie UART2 |
| DATA_RX | 5 | Recepție date |

---

## 4. Intrări Digitale (Inputs)
| Funcție | GPIO | Descriere |
|---------|------|-----------|
| ALARM | 7 | Intrare semnal alarmă |
| OVERLOAD | 6 | Intrare detectare supraîncărcare |

---

## 5. Audio (I2S)
| Funcție | GPIO | Descriere |
|---------|------|-----------|
| I2S_BCLK  | 8  | Bit Clock |
| I2S_LRCLK | 9  | Left/Right Clock |
| I2S_DATA  | 10 | Serial Data |

---

## Rezumat Pinuri folosite
- **Total GPIO-uri utilizate:** 15
- **Pinuri libere (teoretic):** 15 (pe Raspberry Pi Pico care are 30 GPIO în total)

### Pinuri alocate
0, 1, 4, 5, 6, 7, 8, 9, 10, 18, 19, 20, 21, 22, 26

### Pinuri disponibile
2, 3, 11, 12, 13, 14, 15, 16, 17, 23, 24, 25, 27, 28, 29
