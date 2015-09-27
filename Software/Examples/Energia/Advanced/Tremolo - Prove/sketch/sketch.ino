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
#define VOLMAX 15.00
#define VOLMIN -80.00
#define FREQMIN 0.1f // Hz
#define FREQMAX 35.0f // Hz
//#define FREQMIN 1000.0f // Hz
//#define FREQMAX 10000.0f // Hz
#define COLORMIN -15.00f // dB
#define COLORMAX 15.00f // dB

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
void setMode(void);
void setFrequency(void);
void setLfo(void);
void depth(void);

// GLOBAL VARIABLES
// ENCODER
int32_t OldPulses = 0;

// UI
uint8_t count = 0;
uint8_t function = 0;
uint8_t bypass = OFF;
uint8_t oldbypass = OFF;
uint8_t func_counter = 0;
uint8_t old_func_counter = 0;
uint8_t mode = 1;
uint8_t lfotype = 1;
uint8_t restore = 1;
uint8_t mixvalue = 0;
uint16_t readbackcount = 0;
int32_t freqpulses = 198;
int32_t lfotypepulses = 0;
int32_t modepulses = 0;
int32_t colorpulses = 0;
int32_t volumepulses = 0;
int32_t mixpulses = 0;
int32_t depthpulses = 0;
uint32_t time=0, prevtime=0;

float volumedB = 0.00;
float volume = 0.00;
float frequency = 0.00;
float colorvalue = 0.00;
float depthvalue = 0.00;
float readback = 0.00;
float readbackmin = 0.00;
float readbackmax = 0.00;
equalizer_t color;

uint16_t a0;
uint16_t a1;
uint16_t a2;
uint16_t a3;

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

  a0 = Analog.read(A0);
  a1 = Analog.read(A1);
  a2 = Analog.read(A2);
  a3 = Analog.read(A3);


  // DSP board
  InitAida();	// Initialize DSP board
  digitalWrite(RESET, HIGH); // Wake up DSP
  delay(100);  // Start-up delay for DSP
  program_download();    // Here we load program, parameters and hardware configuration to DSP
  spettacolino();
  MasterVolumeMono(DEVICE_ADDR_7bit, MasterVol, 1.00);    // With DAC in mute, set volume to 0dB
  delay(1);   
  AIDA_WRITE_REGISTER_BLOCK(DEVICE_ADDR_7bit, CoreRegisterR4Addr, CoreRegisterR4Size, CoreRegisterR4Data);    // Mute DAC Off
  
  gainCell(DEVICE_ADDR_7bit, Depth, 0.5);
  delay(1);
  dc_source(DEVICE_ADDR_7bit, Bias, 0.5);
  delay(1);
  dc_source(DEVICE_ADDR_7bit, FixedBias, 0.0); 
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
    func_counter++;
    if(func_counter==7)
      func_counter=0;
  }
  else if(function==2)
  {
    bypass ^=1; 
  }

  time = millis();
  if(time-prevtime >= 1000)  // Here we manage control interface every second
  {
    clearAndHome();    // !!!Warning use with real terminal emulation program
    Serial.println("********************************");
    Serial.println("*    User control interface    *");
    Serial.println("*    AIDA Tremolo Sketch       *");
    Serial.println("********************************");
    Serial.write('\n');
    Serial.print("Encoder pulses: ");
    Serial.println(getPulses(), DEC);
    Serial.write('\n');
    
    readbackcount = (ReadBackAlg1Data[0]<<8) | (ReadBackAlg1Data[1]&0xFF);
    readBack(DEVICE_ADDR_7bit, ReadBackAlg1, readbackcount, &readback);
    if(readback > readbackmax)
      readbackmax = readback;
    if(readback < readbackmin)
      readbackmin = readback;
    Serial.print("Raw: ");
    Serial.println(readback, 2);
    Serial.print("Max: ");
    Serial.println(readbackmax, 2);
    Serial.print("Min: ");
    Serial.println(readbackmin, 2);
    Serial.println();
    
    if(oldbypass != bypass)
    {
      if(bypass == ON)
      {
        mux(DEVICE_ADDR_7bit, BypassSelector, 2, 2); // Bypass 
      }
      else
      {
        mux(DEVICE_ADDR_7bit, BypassSelector, 1, 2); // Fx
      }
      oldbypass = bypass;
    }

    if(old_func_counter != func_counter)
    {
      restore = 1;
      old_func_counter = func_counter;
    }
    switch(func_counter)
    {
    case 0: // Frequency
    
      if(restore)
      {
        restore = 0;
        setPulses(freqpulses);
      }
      set_regulation_precision(ON); // Fine regulation
      freqpulses = getPulses();
      frequency = processencoder(FREQMIN, FREQMAX, freqpulses);
      setFrequency();
      break;
    case 1: // LFO type
      if(restore)
      {
        restore = 0;
        setPulses(lfotypepulses);
      }
      lfotypepulses = getPulses();
      lfotype = selectorwithencoder(lfotypepulses, 2); 
      setLfo();
      break;
    case 2: // Mode
      if(restore)
      {
        restore = 0;
        setPulses(modepulses);
      }
      modepulses = getPulses();
      mode = selectorwithencoder(modepulses, 2); 
      setMode();
      break;
    case 3: // Color
      if(restore)
      {
        restore = 0;
        setPulses(colorpulses);
      }
      set_regulation_precision(OFF); // Rough regulation
      colorpulses = getPulses();
      colorvalue = processencoder(COLORMIN, COLORMAX, colorpulses);
      color.S = 0.70;
      color.f0 = 1200.00;
      color.boost = colorvalue;
      color.type = HighShelf;
      EQ2ndOrd(DEVICE_ADDR_7bit, Color, &color);
      break;
    case 4: // Volume
      if(restore)
      {
        restore = 0;
        setPulses(volumepulses);
      }
      set_regulation_precision(OFF); // Rough regulation
      volumepulses = getPulses();
      volumedB = processencoder(VOLMIN, VOLMAX, volumepulses);
      volume = pow(10, volumedB/20);    // From dB to linear conversion --> DSP takes only linear values in 5.28 fixed point format!!!
      MasterVolumeMono(DEVICE_ADDR_7bit, MasterVol, volume);
      break;  
    case 5: // Mix
      if(restore)
      {
        restore = 0;
        setPulses(mixpulses);
      }
      set_regulation_precision(OFF); // Rough regulation
      mixpulses = getPulses();
      mixvalue = (uint8_t)processencoder(0, 100, mixpulses);
      mix(mixvalue);
      break;
    case 6: // Depth
      if(restore)
      {
        restore = 0;
        setPulses(depthpulses);
      } 
      set_regulation_precision(OFF); // Rough regulation
      depthpulses = getPulses();
      depthvalue = processencoder(-1.0, 1.0, depthpulses);
      depth(); 
      break;    
    } // End switch func_counter


    // Print menu
    Serial.print("Effect status: ");
    if(bypass)
      Serial.println("bypass");
    else
      Serial.println("on");
    Serial.write('\n');  
    if(func_counter==0)
      Serial.print("    ");
    Serial.print("Freq. ");
    Serial.print(frequency, 1);
    Serial.println(" Hz");
    if(func_counter==1)
      Serial.print("    ");
    Serial.print("Lfo type: ");
    if(lfotype==1)
      Serial.println("triangular");
    if(lfotype==2)
      Serial.println("sine");
    if(lfotype==3)
      Serial.println("sawtooth");
    if(lfotype==4)
      Serial.println("square");
    //Serial.println(lfotype, DEC);
    if(func_counter==2)
      Serial.print("    ");
    Serial.print("Mode: ");
    if(mode==1)
      Serial.println("normal");
    if(mode==2)
      Serial.println("harmonic");
    if(mode==3)
      Serial.println("opto");
    if(mode==4)
      Serial.println("opto + harmonic");
    //Serial.println(mode, DEC);
    if(func_counter==3)
      Serial.print("    ");
    Serial.print("Color: ");
    Serial.print(colorvalue, 1);
    Serial.println(" dB");
    if(func_counter==4)
      Serial.print("    ");
    Serial.print("Volume: ");
    Serial.print(volumedB, 1);
    Serial.println(" dB");
    if(func_counter==5)
      Serial.print("    ");
    Serial.print("Mix: ");
    Serial.print(mixvalue, DEC);
    Serial.println(" %");
    if(func_counter==6)
      Serial.print("    ");
    Serial.print("Depth: ");
    Serial.println(depthvalue, 2);
    
    Serial.write('\n');
    Serial.print("Active item: ");
    Serial.println(func_counter, DEC);


    prevtime = time;
  } // End if 1000ms tick
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

void setFrequency(void)
{
  triangle_source(DEVICE_ADDR_7bit, Triangle1, frequency*2.00);
  delay(1);
  sine_source(DEVICE_ADDR_7bit, Tone1, frequency);
  delay(1);
  sawtooth_source(DEVICE_ADDR_7bit, Sawtooth1, frequency);
  delay(1);
  square_source(DEVICE_ADDR_7bit, Square1, frequency);
  delay(1);
  /* 
  switch(lfotype)
  {
  case 1:  
    triangle_source(DEVICE_ADDR_7bit, Triangle1, frequency);
    break;
  case 2:
    sine_source(DEVICE_ADDR_7bit, Triangle1, frequency);
    break;
  case 3:
    sawtooth_source(DEVICE_ADDR_7bit, Triangle1, frequency);
    break;
  case 4:
    square_source(DEVICE_ADDR_7bit, Triangle1, frequency);
    break;
  }
  */
}

void setLfo(void)
{
  static uint8_t lfotypeold = 0.00;
  
  if(lfotypeold != lfotype)
  {
    mux(DEVICE_ADDR_7bit, LfoSelector, lfotype, 4);
    lfotypeold = lfotype;
    
    readbackmax = 0.00; // Debug !!!
    readbackmin = 0.00; // debug !!!
  }
}

void setMode(void)
{
  static uint8_t oldmode = 0.00;
  
  if(oldmode != mode)
  {
    switch(mode)
    {
    case 1:  
      mux(DEVICE_ADDR_7bit, Harmonic, 2, 2); // NO Harmonic
      mux(DEVICE_ADDR_7bit, Opto, 2, 2); // NO Opto
      break;
    case 2:
      mux(DEVICE_ADDR_7bit, Harmonic, 1, 2); // SI Harmonic
      mux(DEVICE_ADDR_7bit, Opto, 2, 2); // NO Opto
      break;
    case 3:
      mux(DEVICE_ADDR_7bit, Harmonic, 2, 2); // NO Harmonic
      mux(DEVICE_ADDR_7bit, Opto, 1, 2); // SI Opto
      break;
    case 4:
      mux(DEVICE_ADDR_7bit, Harmonic, 1, 2); // SI Harmonic
      mux(DEVICE_ADDR_7bit, Opto, 1, 2); // SI Opto
      break;
    }
    oldmode = mode;
  }
}

void depth(void)
{ 
  static float olddepthvalue = 0.00;
  
  if(olddepthvalue != depthvalue)
  {
    dc_source(DEVICE_ADDR_7bit, Bias, depthvalue);
    delay(1);
    //gainCell(DEVICE_ADDR_7bit, Depth, 1.0-depthvalue);
    olddepthvalue = depthvalue;
  }
}




