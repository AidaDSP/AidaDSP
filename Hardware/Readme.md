# Hardware infos

__Leds__
- LED1 goes to MP1 digi I/O of DSP (see exp. connector);
- LED2 goes to MP10
- LED3 goes to MP11


__Dipswitch I2C Addresses__

Dip1 (Addr0) | Dip2 (Addr1) | I2C DSP Address (W)
------------ | ------------ | -------------------
On | On | 0x68
On | Off | 0x6A
Off | On | 0x6C
Off | Off | 0x6E
