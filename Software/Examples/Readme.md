List of sofware examples to be used with Aida DSP board 

### TEMPLATES 
Start immediately coding with Aida DSP. A minimal UI (user interface) composed
only by a switch, some leds and an encoder knob (all them soldered on Aida DSP board) and a PuTTY based 
GUI is simple but powerful enough for testing many of your audio projects. Then when
your project is working you can always migrate to a more complex UI.

- Template_1 	dual function with single switch (fast/long pressure) and encoder knob position
				allow the user to move between master volume control and preset selection
- Template_2    dual function with single swith (fast/long pressure) and encoder knob position 
				allow the user to engage a FX disable/enable and a circular property selector to modify one parameter at a time

### BASICS 
These examples show the basic usage of the Aida DSP API when used with Arduino and Energia

- **Tutorial_0**  dsp gpio/led control
- **Tutorial_1**	master volume control 
- **Tutorial_2**	1st order equalizer 
- **Tutorial_3**	2nd order equalizer (4-band graphic equalizer with selectable Q, f, gain)
- **Tutorial_4** 	state variable filter and audio mux 
- **Tutorial_5**	stereo rms compressor with post gain
- **Tutorial_6**	stereo peak compressor with post gain
- **Tutorial_7**	multiple signal generator (synthesizer) with average/rms/peak/raw volume readback
- **Tutorial_8**	distortion hard & soft (tube) clipping

### ADVANCED / USER PROJECTS 
These examples show advanced functionality or complete projects submitted by us and various users/enthusiasts

Project's name | Compatibility | Function | Status | Link 
-------------- | ------------- | -------- | ------ | ---- 
**Motown EQ**  | Aida DSP "La Prima" | a 7 band parametric equalizer with fixed musical frequencies | Finished | ... 
**Tubescreamer**  | Aida DSP "Box" | the replica of the evergreen stomp box for guitar | Finished | ... 
**Tremolo**  | Aida DSP "Box" | an algorithm inspired by Strymon | Finished | http://www.strymon.net/2012/04/12/amplifier-tremolo-technology-white-paper/ 
**3D Enhancer** | Aida DSP "La Prima" | various audio 3D virtualization algorithms for a stereo setup (speakers or headphones) | Work In Progress | ... 
**Reverb Mono** | Aida DSP "La Prima" | a basic reverb algorithm with dry/wet control | Work In Progress | ... 
**Vocoder** | Aida DSP "La Prima" | well, this ROCKS!!! | Work In Progress | ... 



**NOTES**: 

- The usage of [PuTTY](http://www.chiark.greenend.org.uk/~sgtatham/putty/download.html) is very simple. You need
to install the USB drivers of your controller board (ex. Arduino) which will adds a **COM Port** to your Pc. Then 
you must open a **PuTTY** serial com session with
 * **115200** (baud rate) 
 * 8 bits 
 * 1 stop bits 
 * Parity:None 
 * Flow Control:None  
 
- Advanced / User Projects may use external 
hardware (pots, switches, displays...ecc) to address specific purposes. See comments inside sketches for details.




AidaDSP Team 2016