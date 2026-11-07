Hardware Map — Single-Elevator Cabin Panel / Marble Pico

MCU: Raspberry Pi Pico / RP2040
Display: ILI9341, 240×320

GPIO	Funcție	Direcție	Observații
GP11	RESET BUTTON IN	IN	Buton normal deschis la GND; patru apăsări în 2 s resetează panoul
GP6	OVERLOAD IN	IN	Intrare suprasarcină
GP7	ALARM IN	IN	Intrare alarmă
GP8	I2S BCLK	OUT	MAX98357A bit clock
GP9	I2S LRCLK	OUT	MAX98357A word select / left-right clock
GP10	I2S DATA	OUT	MAX98357A audio data
GP12	SPI1 MISO	IN	W5500 data către RP2040
GP13	W5500 CS	OUT	Selectare circuit W5500
GP14	SPI1 SCK	OUT	Ceas W5500
GP15	SPI1 MOSI	OUT	Date RP2040 către W5500
GP18	LCD CLK	OUT	ILI9341 SPI clock
GP19	LCD MOSI	OUT	ILI9341 SPI MOSI
GP20	LCD RESET	OUT	ILI9341 reset
GP21	LCD DC	OUT	ILI9341 data/command
GP22	LCD CS	OUT	ILI9341 chip select
GP26	LCD BACKLIGHT	OUT	Control backlight

Ethernetul folosește W5500 pe SPI1, cu DHCP și MAC `3C:AB:72:52:68:12`. Panoul ascultă TCP pe porturile `2000` (date encoder), `2004` (transmisie către echipamentul GSM/SSC30KQ) și `2005` (service). Pinii UART anteriori CH9121 sunt eliberați. Reset și INT W5500 nu sunt conectate în maparea curentă.
