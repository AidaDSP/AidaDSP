# AidaDSP

AidaDSP is a shield for Arduino and TivaC boards.
Basically, you have a small but powerful DSP from Analog Devices
capable of 24bit-48/96/192kHz audio processing on the fly.

What you can do with this shield?
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
- More...since you can program your own processing block at low level
 
Thanks to our library (AidaDSP's official library on Github https://github.com/AidaDSP) you can
integrate the control of this DSP in the Arduino enviroment.
If the DSP itself it's interesting, the interface with Arduino gives you outstanding possibilities since
you can manage communications and I/O with your preferred microcontroller while the DSP is processing audio
on its own, and you can change DSP algorithms parameters in real-time when you just want to do it.

Audio enthusiasts, this board is for you!!!

NOTE: this solution is SUPERIOR to ANY present and future DSP system based on a microcontroller with external Codec.
Microcontrollers, once very similar to DSPs, are a whole DIFFERENT story. The hardware of this DSP is DEDICATED
for audio processing, for example you have 56-bit accumulator wich gives you room for internal algorithm gains without the 
risk for clipping. Ready to use algorithms cell has been coded and optimized in ASSEMBLER by Analog Devices Engineers, wich work at the top of audio industry from 40 years.

2015 Aida Team
www.aidadsp.com
