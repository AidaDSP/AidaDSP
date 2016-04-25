# Hardware specs & additional informations

### Aida DSP "La Prima"

__Leds__
- LED1 goes to MP1 digi I/O of DSP (see exp. connector)
- LED2 goes to MP10
- LED3 goes to MP11

__Dipswitch I2C Addresses__

Dip1 (Addr0) | Dip2 (Addr1) | I2C DSP Address (W)
------------ | ------------ | -------------------
Off | Off | 0x68
Off | On | 0x6A
On | Off | 0x6C
On | On | 0x6E

__Used Pins VS Platforms__

Arduino Uno/Mega/Due | TivaC (TM4C123) | Function
-------------------- | --------------- | -------- 
SCL1 | SCL(0) | I2C SCL
SDA1 | SDA(0) | I2C SDA
2 | PA_6 | ENCB
3 | PA_7 | ENCA
4 | PF_4 (PUSH1) | ENC_PUSH
11 | PB_7 | RESET
12 | PB_6 | SBOOT

__Board Specifications__

Parameter | Value
-------------------- | -----
Sample Rate | 48-96-192kHz selectable 
Bit Depth | 24-bit 
Analog Audio Connections | 1 stereo mini jack in 2 stereo mini jack out
Input Level | 2.0Vrms (5.66Vpp, 8.23dBu)
Input Impedance | 20kohm
Output Level | 0.9Vrms (2.5Vpp, 1.30dBu) 
Output Impedance | High (expected load >=20k)
Dynamic Range | 100dB ADC, 100dB DAC A-weighted
Digital Connections | I2S, TDM
GPIOs | See pinout expansion connector 
Dimensions | 105mm x 54mm x 35mm
Power Supply | 5.0Vdc, powered by Arduino/TivaC

### Schematics
* At the moment we cannot publish our schematics, we are strong supporters
of open source so in the future we'll make them available to anyone. In the meantime,
forgive us, we can send schematics to our customers if they need them. Contact us at max@aidadsp.com







