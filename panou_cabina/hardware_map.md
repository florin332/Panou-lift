Hardware Map — Single-Elevator Cabin Panel / Marble Pico

MCU: Raspberry Pi Pico / RP2040
Display: ILI9341, 240×320

GPIO	Funcție	Direcție	Observații
GP0	UART1 TX	OUT	Comanda alarma rezervata; conectat la RXD1 CH9121; formatul nu este definit
GP1	UART1 RX	IN	Conectat la TXD1 CH9121; neutilizat
GP4	UART2 TX	OUT	Conectat la RXD2 CH9121; neutilizat
GP5	UART2 RX	IN	Receptie date ascensor; conectat la TXD2 CH9121
GP6	OVERLOAD IN	IN	Intrare suprasarcină
GP7	ALARM IN	IN	Intrare alarmă
GP8	I2S BCLK	OUT	MAX98357A bit clock
GP9	I2S LRCLK	OUT	MAX98357A word select / left-right clock
GP10	I2S DATA	OUT	MAX98357A audio data
GP18	LCD CLK	OUT	ILI9341 SPI clock
GP19	LCD MOSI	OUT	ILI9341 SPI MOSI
GP20	LCD RESET	OUT	ILI9341 reset
GP21	LCD DC	OUT	ILI9341 data/command
GP22	LCD CS	OUT	ILI9341 chip select
GP26	LCD BACKLIGHT	OUT	Control backlight
