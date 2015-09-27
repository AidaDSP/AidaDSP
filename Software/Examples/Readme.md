List of sofware to use with Aida DSP board 

TEMPLATES - start immediately coding with Aida DSP. A minimal UI composed
only by a switch and an encoder knob (soldered on Aida DSP board) and a PuTTY based 
GUI is simple but powerful enough for many of your audio projects.
- Template_1 	dual function with single swith (fast/long pressure) and encoder knob position
				switch between master volume control and preset selection
- Template_2    dual function with single swith (fast/long pressure) and encoder knob position 
				FX disable/enable and circular property selector to modify one parameter at a time

BASICS - these examples show the basic usage of the Aida DSP API when used with Arduino and Energia
- Tutorial_1	master volume control 
- Tutorial_2	1st order equalizer 
- Tutorial_3	2nd order equalizer (4-band graphic equalizer with selectable Q, f, gain)
- Tutorial_4 	state variable filter and audio mux 
- Tutorial_5	stereo rms compressor with post gain
- Tutorial_6	stereo peak compressor with post gain
- Tutorial_7	multiple signal generator (synthesizer) with rms/peak volume readback
- Tutorial_8	distortion hard & soft (tube) clipping

ADVANCED / USER PROJECTS - these examples show advanced functionality or complete projects submitted by users
- Motown 		a 7 band parametric equalizer with fixed musical frequencies inspired by [link]
- TS9 			the replica of the evergreen stomp box for guitar [link]
- Tremolo 		an algorithm inspired by Strymon http://www.strymon.net/2012/04/12/amplifier-tremolo-technology-white-paper/
- 3D Enhancer   various audio 3D virtualization algorithms for a stereo setup (speakers or headphones)
- Reverb Mono   a basic reverb algorithm with dry/wet control (mix) inspired by [link]
- Vocoder       a beautiful project inspired by [link] wich finally shows the power of DSP
- ...

NOTE: templates and basics uses encoder switch and knob and PuTTY to do everything. Mount Aida DSP and
you have all that you need to start audio processing! Advanced / User Projects may use external 
hardware (pots, switches, displays...ecc) to address specific purposes. See comments inside sketches for details. 

AidaDSP Team 2015
www.aidadsp.com