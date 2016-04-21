### Aida DSP "La Prima"

Aida DSP "La Prima" is a shield for Arduino and Energia (TM4C123) with a 24bit/48-96-192kHz DSP on it.

### Salient specs
- Compact design size 105x54x35mm (Arduino shield form factor)
- Low noise design for audio applications
- 2 mono inputs 4 mono outputs 
- Encoder, Button and Leds soldered on board for practical usage
- Expansion connector with available signals from DSP
- Available open source examples for Arduino Uno/Mega/Due and Energia (TM4C123)

### How it works
DSP is programmed with Sigma Studio. This is an extremely intuitive graphical design 
tool, with a smooth learning curve. When you've tweaked your audio algorithm enough
you can generate an .xml file which contains DSP firmware and then with our [Java tool](https://github.com/AidaDSP/AidaDSP/tree/master/Software/Java/AidaHeaderFileGenerator/bin)
you can generate a C header file to be used in the Arduino IDE.

Now you can code a sketch that communicates with the DSP in real time to change its cells (audio algorithm blocks) using [Aida DSP official Arduino API library](https://github.com/AidaDSP/AidaDSP).

Basically you can manage your UI (user interface: buttons, faders, etc.) with Arduino and then
let the DSP do the whole hard work of processing audio with superb quality. For example you can use the DSP
to generate a sine tone and change its frequency and amplitude with a pot wired to Arduino's analog input. 
Check out our [examples](https://github.com/AidaDSP/AidaDSP/tree/master/Software/Examples) on Github!

### Typical applications
- 1st and 2nd order equalizers with adjustable f, Q, gain
- Processors with peak or rms detection for monochannel
  and multichannel dynamics
- Mixers and splitters
- Tone and noise generators
- Fixed and variable gain
- Loudness
- Delay 
- Stereo enhancement
- Dynamic bass boost
- Noise and tone sources
- FIR filters
- Level detectors
- GPIO control and conditioning
- Special function to update DSP parameters in real-time without clicks and pops
- More...

### Audio enthusiasts and engineers, this board is for you!

### More info

- [Examples](https://github.com/AidaDSP/AidaDSP/tree/master/Software/Examples)
- [Arduino/Energia Libraries](https://github.com/AidaDSP/AidaDSP/tree/master/Software/Libraries)
- [Java Apps](https://github.com/AidaDSP/AidaDSP/tree/master/Software/Java)
- [Hardware specs & additional informations](https://github.com/AidaDSP/AidaDSP/tree/master/Hardware)
- [Wiki](https://github.com/AidaDSP/AidaDSP/wiki)
- **Where to BUY:** [Tindie](https://www.tindie.com/products/MaxAidaDSP/aida-dsp/) and [Aida DSP Store](http://www.aidadsp.com/#!/STORE)

Aida DSP Team 2016 www.aidadsp.com
