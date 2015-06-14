/*
 AIDA Tremolo Sketch
 	
 This sketch is an "Harmonic Tremolo" as described in Strimon White Paper
 http://www.strymon.net/2012/04/12/amplifier-tremolo-technology-white-paper/
 This sketch was written for the Stellaris, and will not work on other boards.
 	
 The circuit:
 
 Audio:
 * Input range 2.0Vrms (5.66Vpp, 8.23dBu)
 * Output range 0.9Vrms (2.5Vpp, 1.30dBu) 
 
 PC:
 * Please connect with PuTTY on Stellaris USB Serial with a PC for a minimal user interface
 
 NOTE:
 Attenuation Out/In = 2.264, to have out = in you must provide 7.097dB of gain through DSP algorithm
 or externally with active LPF filter.
 
 created November 2014
 by Massimo Pennazio
 
 This example code is in the public domain.
 
 FRA TRINGALI
 - scurisce il suono;
 - tipo di tremolo normale non da abbastanza colore...
 - capire 
 
 */

#include <Energia.h>
#include <pins_energia.h>
#include <Wire.h>
#include "AidaFW.h"
#include "AidaDSP.h"

#define EVER (;;)

// DEFINES USER INTERFACE
#define VOLMAX 6.00
#define VOLMIN -80.00
#define FREQMIN 0.1f // Hz
#define FREQMAX 10.0f // Hz

#define ON 1
#define OFF 0

#define POT1 A0
#define POT2 A1
#define POT3 A2
#define POT4 A3

// FUNCTION PROTOTYPES
void spettacolino();
void clearAndHome(void);
void mix(uint8_t percent);

// GLOBAL VARIABLES
// ENCODER
int32_t OldPulses = 0;

// UI
uint8_t count = 0;
uint8_t function = 0;
uint8_t mute = OFF;
uint8_t submenu = OFF;
uint8_t preset = 0;
uint8_t oldpreset = 0;
uint8_t restore = OFF;
uint8_t restoreflag = false;
uint16_t PotValue = 0;
uint32_t time=0, prevtime=0;

float volume = 0.00;
float frequency = 0.00;
equalizer_t color;

void setup()
{
  // put your setup code here, to run once:
  // I/O
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);

  // open the USBSerial port
  Serial.begin(115200);
  clearAndHome();
  Serial.println("Aida DSP control with STELLARIS"); // Welcome message
  Serial.print("0x");
  Serial.println((DEVICE_ADDR_7bit<<1)&~0x01, HEX);

  // DSP board
  InitAida();	// Initialize DSP board
  digitalWrite(RESET, HIGH); // Wake up DSP
  delay(100);  // Start-up delay for DSP
  program_download();    // Here we load program, parameters and hardware configuration to DSP
  spettacolino();
  MasterVolumeMono(DEVICE_ADDR_7bit, MasterVol, 0.00);    // With DAC in mute, set volume to 0
  delay(1);   
  AIDA_WRITE_REGISTER_BLOCK(DEVICE_ADDR_7bit, CoreRegisterR4Addr, CoreRegisterR4Size, CoreRegisterR4Data);    // Mute DAC Off
  delay(1);
  // Depth
  AIDA_SAFELOAD_WRITE_VALUE(DEVICE_ADDR_7bit, Depth, true, pow(10, -9.0/20.00));	 // Depth is a combination of offset and volume
  delay(1);
  // DepthBias
  dc_source(DEVICE_ADDR_7bit, DepthBias, 0.5);
  delay(1);
  // RefBias
  dc_source(DEVICE_ADDR_7bit, RefBias, 0.5);
  // Mix
  mix(25);
  delay(1);
  // State Variable Filter
  StateVariable(DEVICE_ADDR_7bit, StateVarFilter1, 700.0, 0.71);
  delay(1);
  // Color
  color.S = 0.70;
  color.f0 = 1200.00;
  color.boost = 6.00;
  color.type = HighShelf;
  EQ2ndOrd(DEVICE_ADDR_7bit, Color, &color);
  delay(1);
  // Master Vol
  MasterVolumeMono(DEVICE_ADDR_7bit, MasterVol, pow(10, 0.00/20.00));
  delay(1);
  // Mode
  mux(DEVICE_ADDR_7bit, ModeSelector, 1, 2); // Harmonic
  delay(1);
  // Bypass
  mux(DEVICE_ADDR_7bit, BypassSelector, 2, 2); // Bypass 
  delay(1);
  // LFO Source
  mux(DEVICE_ADDR_7bit, LfoSelector, 1, 4); // Triangular
  delay(1);
  // Gain
  MasterVolumeMono(DEVICE_ADDR_7bit, Gain, 2.5);
  delay(1);
  // Pre Gain
  MasterVolumeMono(DEVICE_ADDR_7bit, PreGain, 2.5);
  delay(1);
}

void loop()
{
  // put your main code here, to run repeatedly:

  if(digitalRead(ENC_PUSH)==LOW)  
  {
    digitalWrite(GREEN_LED, HIGH);
    delay(50);  // debounce
    if(digitalRead(ENC_PUSH)==LOW)
    {
      count++;
    }   
  }
  else
  {
    if(count>0 && count<10)
      function = 1;
    else if(count>10 && count<30)
      function = 2;
    else
      function = 0;  // No function triggered on switch
    count = 0;
    digitalWrite(GREEN_LED, LOW);
  }

  if(function==1)
  {
    mute ^= 1;
  }
  else if(function==2)
  {
    submenu ^=1; 
  }

  time = millis();
  if(time-prevtime >= 1000)  // Here we manage control interface every second
  {
    clearAndHome();    // !!!Warning use with real terminal emulation program
    Serial.println("********************************");
    Serial.println("*    User control interface    *");
    Serial.println("*    AIDA Tremolo Sketch       *");
    Serial.println("********************************");
    Serial.println("Press button rapidly to switch mute on/off,");
    Serial.println("press button for 1 sec to enter submenu.");
    Serial.write('\n');

    Serial.print("Encoder pulses: ");
    Serial.println(getPulses(), DEC);
    if(mute == OFF)
    {
      if(submenu == OFF)
      {
        if(restoreflag == true)
        {
          restoreflag = false;
          setPulses(OldPulses);
        }
        //volume = processencoder(VOLMIN, VOLMAX, getPulses());
        frequency = processencoder(FREQMIN, FREQMAX, getPulses());

        //Serial.print("Master Vol. ");
        //Serial.print(volume, 1);
        //Serial.println("dB");
        
        Serial.print("Freq. ");
        Serial.print(frequency, 1);
        Serial.println("Hz");

        switch(preset)
        {
          case 1:  
            break;
          case 2:
            triangle_source(DEVICE_ADDR_7bit, Triangle1, frequency);
            break;
          case 3:
            triangle_source(DEVICE_ADDR_7bit, Triangle1, frequency);
            break;
          case 4:
            sine_source(DEVICE_ADDR_7bit, Triangle1, frequency);
            break;
          case 5:
            sawtooth_source(DEVICE_ADDR_7bit, Triangle1, frequency);
            break;
          case 6:
            square_source(DEVICE_ADDR_7bit, Triangle1, frequency);
            break;
          case 7:
            
            break;
          case 8:
            
            break;
        }

        //volume = pow(10, volume/20);    // From dB to linear conversion --> DSP takes only linear values in 5.28 fixed point format!!!
        //MasterVolumeMono(DEVICE_ADDR_7bit, MasterVol, volume);
      }
      else
      {
        //MasterVolumeMono(DEVICE_ADDR_7bit, MasterVol, volume);
      }			
    }
    else if(mute == ON)
    {
      //MasterVolumeMono(DEVICE_ADDR_7bit, MasterVol, 0.00);
      Serial.println("mute on");
    }
    if(submenu==ON)
    {
      if(restoreflag == false)
      {
        restoreflag = true;
        OldPulses = getPulses();  // Save actual Pulses for restoring when exit menu
        setPulses(16);  // Restart from a known position
      }
      preset = (uint8_t)selectorwithencoder(getPulses(), 3);  // Use the encoder as a selector, 3 bit

      // Muxes produces noises so switch them only one time (when changed)
      if(oldpreset != preset)
      {
        switch(preset)
        {
        case 1:
          // Bypass
          mux(DEVICE_ADDR_7bit, BypassSelector, 2, 2);
          delay(1);
          break;
        case 2:
          // Triangle
          // Color
          color.S = 0.70;
          color.f0 = 1200.00;
          color.boost = 0.00;
          color.type = HighShelf;
          EQ2ndOrd(DEVICE_ADDR_7bit, Color, &color);
          mux(DEVICE_ADDR_7bit, BypassSelector, 1, 2); 
          delay(1);
          mux(DEVICE_ADDR_7bit, LfoSelector, 1, 4);
          delay(1);
          //triangle_source(DEVICE_ADDR_7bit, Triangle1, 1.7);
          delay(1);
          break;
        case 3:
          // Triangle
          // Color
          color.S = 0.70;
          color.f0 = 1200.00;
          color.boost = 12.00;
          color.type = HighShelf;
          EQ2ndOrd(DEVICE_ADDR_7bit, Color, &color);
          mux(DEVICE_ADDR_7bit, BypassSelector, 1, 2); 
          delay(1);
          mux(DEVICE_ADDR_7bit, LfoSelector, 1, 4);
          delay(1);
          //triangle_source(DEVICE_ADDR_7bit, Triangle1, 1.7);
          delay(1);
          break;
        case 4:
          // Sine
          // Color
          color.S = 0.70;
          color.f0 = 1200.00;
          color.boost = 20.00;
          color.type = HighShelf;
          EQ2ndOrd(DEVICE_ADDR_7bit, Color, &color);
          delay(1);
          mux(DEVICE_ADDR_7bit, BypassSelector, 1, 2); 
          delay(1);
          mux(DEVICE_ADDR_7bit, LfoSelector, 2, 4);
          delay(1);
          //sine_source(DEVICE_ADDR_7bit, Tone1, 1.7);
          delay(1);
          break;
        case 5:
          // Sawtooth
          mux(DEVICE_ADDR_7bit, BypassSelector, 1, 2); 
          delay(1);
          mux(DEVICE_ADDR_7bit, LfoSelector, 3, 4);
          delay(1);
          //sawtooth_source(DEVICE_ADDR_7bit, Sawtooth1, 1.7);
          delay(1);
          break;
        case 6:
          // Square
          mux(DEVICE_ADDR_7bit, BypassSelector, 1, 2); 
          delay(1);
          mux(DEVICE_ADDR_7bit, LfoSelector, 4, 4);
          delay(1);
          //square_source(DEVICE_ADDR_7bit, Square1, 1.7);
          delay(1);
          break;
        case 7:
          
          break;
        case 8:
          
          break;
        }
        oldpreset = preset;
      }
      
      // Print only submenu messages
      switch(preset)
      {
        case 1:
          Serial.println(" Bypass");
          break;
        case 2:
          Serial.println(" LFO Triangle Harm Scuro...");
          break;
        case 3:
          Serial.println(" LFO Triangle Harm Chiaro...");
          break;
        case 4:
          Serial.println(" LFO Sine Harm...");
          break;
        case 5:
          Serial.println(" LFO Sawtooth Harm...");
          break;
        case 6:
          Serial.println(" LFO Square Harm...");
          break;
        case 7:
          Serial.println(" Nothing...");
          break;
        case 8:
          Serial.println(" Nothing...");
          break;
      }
      

      Serial.write('\n');
      Serial.print("    Selected preset: ");
      Serial.println(preset, DEC);

    }
    else if(submenu==OFF)
    {
      Serial.write('\n');
      Serial.println("    Submenu OFF");
    }

    prevtime = time;
  } 

} // End void loop

void spettacolino()
{
  byte i;
  byte count = 0x00;

  for(i=0;i<6;i++)
  {
    count += 1;
    if(count==1)
      digitalWrite(RED_LED, HIGH);   
    else if(count==2)
      digitalWrite(GREEN_LED, HIGH);
    else{
      digitalWrite(BLUE_LED, HIGH);
      count=0;
    }
    delay(250);
    digitalWrite(RED_LED, LOW);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(BLUE_LED, LOW);
    delay(250);
  }
}

void clearAndHome(void)
{
  Serial.write(0x1b); // ESC
  Serial.print("[2J"); // clear screen
  Serial.write(0x1b); // ESC
  Serial.print("[H"); // cursor to home
}

void mix(uint8_t percent)
{
  float value = percent/100.00;
  
  // MIX
  AIDA_SAFELOAD_WRITE_VALUE(DEVICE_ADDR_7bit, Mix, false, value);  // Dry	 
  AIDA_SAFELOAD_WRITE_VALUE(DEVICE_ADDR_7bit, Mix+1, true, 1.00-value);   // Wet
}



