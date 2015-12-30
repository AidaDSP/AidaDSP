/*
 AIDA Tutorial_6 Sketch
 	
 This sketch controls a peak rms compressor with post gain using the structure of Template2.
 This sketch was written for Arduino, and will not work on other boards.
 	
 The circuit:
 
 Audio:
 * Input range 2.0Vrms (5.66Vpp, 8.23dBu)
 * Output range 0.9Vrms (2.5Vpp, 1.30dBu) 
 
 PC:
 * Please connect with PuTTY on Arduino USB Serial with a PC for a minimal user interface
 
 NOTE:
 Attenuation Out/In = 2.264, to have out = in you must provide 7.097dB of gain through DSP algorithm
 or externally with active LPF filter.
 
 created November 2014
 by Massimo Pennazio
 
 This example code is in the public domain.
 
 */

#include <Arduino.h>
#include <pins_arduino.h>
#include <Wire.h>
#include "AidaDSP.h"
#include "AidaFW.h"

#define EVER (;;)

// DEFINES I/O
#define PIN_LED  13

// DEFINES USER INTERFACE
#define VOLMAX 0.00 // dB
#define VOLMIN -80.00 // dB
#define THRMAX 6.00 // dB
#define THRMIN -90.00 // dB
#define RATIOMAX 100.0
#define RATIOMIN 1.00
#define ATTACKMAX 500.0 // ms 
#define ATTACKMIN 1.00 // ms
#define HOLDMAX 500.0 // ms
#define HOLDMIN 1.00 // ms
#define DECAYMAX 2000.0 // ms
#define DECAYMIN 868.0 // ms
#define POSTGAINMAX 24.00 // dB
#define POSTGAINMIN -30.00 //dB 

#define ON 1
#define OFF 0

// FUNCTION PROTOTYPES
void spettacolino(void);
void clearAndHome(void);

// GLOBAL VARIABLES

// UI
uint8_t bypass = OFF;
uint8_t oldbypass = OFF;
uint8_t func_counter = 0;
uint8_t old_func_counter = 0;
uint8_t push_e_count = 0;
uint8_t push_e_function = 0;
uint8_t restore = 1; // If restore == 1 when program starts, you apply default settings

uint16_t param2_index = 1;
uint16_t oldparam2_index = 0;

// Default settings or values for encoder pulses with various parameters 
// you should code a function which converts from a desired default value ex: -10.00 to the relative pulse count for
// a processenc function call for this parameter (with MIN and MAX) because default settings works with encoder pulses for a defined parameter
int32_t param1_pulses = -10; // -9.4 dB threshold  
int32_t param2_pulses = 53;  // 6 ratio
int32_t param3_pulses = 0; // 1.0 ms attack
int32_t param4_pulses = 16; // 84.2 ms hold 
int32_t param5_pulses = 8; // 962.3 ms decay
int32_t param6_pulses = 0; // 0 dB postgain
int32_t param7_pulses = 0; // 0 dB Master Volume 

uint32_t timec=0, prevtimec=0;

float param1_value = 0.00;
float param3_value = 0.00;
float param4_value = 0.00;
float param5_value = 0.00;
float param6_value = 0.00;
float param7_value = 0.00;

float volume = 0.00;
compressor_t comp1; // Instantiate resource to be used by compressor function

void setup()
{
  // put your setup code here, to run once:
  // I/O
  pinMode(PIN_LED, OUTPUT);
   
  // open the USBSerial port
  Serial.begin(115200);
  clearAndHome();
  Serial.println(F("Aida DSP control with ARDUINO")); // Welcome message
  Serial.print(F("0x"));
  Serial.println((DEVICE_ADDR_7bit<<1)&~0x01, HEX); // Print I2C Dsp address

  InitAida(); // Initialize DSP board 
  digitalWrite(RESET, HIGH); // Wake up DSP
  delay(100);  // Start-up delay for DSP
  program_download();    // Here we load program, parameters and hardware configuration to DSP
  delay(20);
  spettacolino();
  // Initialize comp1 values
  param1_value = processencoder(THRMIN, THRMAX, param1_pulses);
  param2_index = selectorwithencoder(param2_pulses, 2); 
  switch(param2_index) 
  {
    case 1:
      comp1.ratio = 1.0;
      break;
    case 2:
      comp1.ratio = 2.0;
      break;
    case 3:
      comp1.ratio = 6.0;
      break;
    case 4:
      comp1.ratio = 8.0;
      break;
  }
  param3_value = processencoder(ATTACKMIN, ATTACKMAX, param3_pulses);
  param4_value = processencoder(HOLDMIN, HOLDMAX, param4_pulses);
  param5_value = processencoder(DECAYMIN, DECAYMAX, param5_pulses);
  param6_value = processencoder(POSTGAINMIN, POSTGAINMAX, param6_pulses);
  param7_value = processencoder(VOLMIN, VOLMAX, param7_pulses);
  
  comp1.threshold = param1_value; // dB
  comp1.attack = param3_value; // ms
  comp1.hold = param4_value; // ms
  comp1.decay = param5_value; // ms
  comp1.postgain = param6_value; // dB 
  volume = param7_value; // dB
}

void loop()
{
  // put your main code here, to run repeatedly:

  if(digitalRead(ENC_PUSH)==LOW)  
  {
    digitalWrite(PIN_LED, HIGH);
    delay(50);  // debounce
    if(digitalRead(ENC_PUSH)==LOW)
    {
      push_e_count++;
    }   
  }
  else
  {
    if(push_e_count>0 && push_e_count<10)
      push_e_function = 1;
    else if(push_e_count>10 && push_e_count<30)
      push_e_function = 2;
    else
      push_e_function = 0;  // No function triggered on switch
    push_e_count = 0;
    digitalWrite(PIN_LED, LOW);
  }
  
  if(push_e_function==1)
  {
    func_counter++;
    if(func_counter==7)
      func_counter=0;
  }
  else if(push_e_function==2)
  {
    bypass ^=1; 
  }

  timec = millis();
  if(timec-prevtimec >= 250)  // Here we manage control interface every 250ms
  { 
    clearAndHome();    // !!!Warning use with real terminal emulation program
    Serial.println(F("********************************"));
    Serial.println(F("*    User control interface    *"));
    Serial.println(F("*    AIDA Tutorial_6 Sketch    *"));
    Serial.println(F("********************************"));
    Serial.println(F(""));
    Serial.print(F(" Encoder pulses: "));
    Serial.println(getPulses(), DEC);
    Serial.println(F(""));
    
    if(oldbypass != bypass)
    {
      if(bypass == ON)
      {
        mux(DEVICE_ADDR_7bit, BypassAddr, 2, 2); // Switch bypass mux
      }
      else
      {
        mux(DEVICE_ADDR_7bit, BypassAddr, 1, 2); // Switch bypass mux
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
    case 0: // PARAM1
      if(restore)
      {
        restore = 0;
        setPulses(param1_pulses);
      }
      param1_pulses = getPulses();
      param1_value = processencoder(THRMIN, THRMAX, param1_pulses);
      if(param1_value != comp1.threshold)
      {
        comp1.threshold = param1_value;
        CompressorPeak(DEVICE_ADDR_7bit, Compressor1Addr, &comp1);
      }
      break;
    case 1: // PARAM2
      if(restore)
      {
        restore = 0;
        setPulses(param2_pulses);
      }
      param2_pulses = getPulses();
      param2_index = selectorwithencoder(param2_pulses, 2);
      if(param2_index != oldparam2_index)
      { 
        oldparam2_index = param2_index;
        switch(param2_index) 
        {
          case 1:
            comp1.ratio = 1.0;
            CompressorPeak(DEVICE_ADDR_7bit, Compressor1Addr, &comp1);
            break;
          case 2:
            comp1.ratio = 2.0;
            CompressorPeak(DEVICE_ADDR_7bit, Compressor1Addr, &comp1);
            break;
          case 3:
            comp1.ratio = 6.0;
            CompressorPeak(DEVICE_ADDR_7bit, Compressor1Addr, &comp1);
            break;
          case 4:
            comp1.ratio = 8.0;
            CompressorPeak(DEVICE_ADDR_7bit, Compressor1Addr, &comp1);
            break;
        }
      }
      break;
    case 2: // PARAM3
      if(restore)
      {
        restore = 0;
        setPulses(param3_pulses);
      }
      param3_pulses = getPulses();
      param3_value = processencoder(ATTACKMIN, ATTACKMAX, param3_pulses);
      if(param3_value != comp1.attack)
      {
        comp1.attack = param3_value;
        //CompressorPeak(DEVICE_ADDR_7bit, Compressor1Addr, &comp1); // Peak Compressor doesn't have attack parameter!!!
      }
      break;
    case 3: // PARAM4
      if(restore)
      {
        restore = 0;
        setPulses(param4_pulses);
      }
      param4_pulses = getPulses();
      param4_value = processencoder(HOLDMIN, HOLDMAX, param4_pulses);
      if(param4_value != comp1.hold)
      {
        comp1.hold = param4_value;
        CompressorPeak(DEVICE_ADDR_7bit, Compressor1Addr, &comp1);
      }
      break;
    case 4: // PARAM5
      if(restore)
      {
        restore = 0;
        setPulses(param5_pulses);
      }
      param5_pulses = getPulses();
      param5_value = processencoder(DECAYMIN, DECAYMAX, param5_pulses);
      if(param5_value != comp1.decay)
      {
        comp1.decay = param5_value;
        CompressorPeak(DEVICE_ADDR_7bit, Compressor1Addr, &comp1);
      }
      break;
    case 5: // PARAM 6
      if(restore)
      {
        restore = 0;
        setPulses(param6_pulses);
      }
      param6_pulses = getPulses();
      param6_value = processencoder(POSTGAINMIN, POSTGAINMAX, param6_pulses);
      if(param6_value != comp1.postgain)
      {
        comp1.postgain = param6_value;
        CompressorPeak(DEVICE_ADDR_7bit, Compressor1Addr, &comp1);
      }
      break;
    case 6: // PARAM 7
      if(restore)
      {
        restore = 0;
        setPulses(param7_pulses);
      }
      param7_pulses = getPulses();
      param7_value = processencoder(POSTGAINMIN, POSTGAINMAX, param7_pulses);
      if(param7_value != volume)
      {
        volume = param7_value;
        MasterVolumeStereo(DEVICE_ADDR_7bit, MasterVolumeAddr, pow(10, volume/20.0));
      }
      break;  
    } // End switch func_counter

    // Display information for user
    print_menu_putty();

    prevtimec = timec;
  } // End if 1000ms tick
} // End void loop

void spettacolino()
{
  byte i;
  byte status = 0x00;

  for(i=0;i<6;i++)
  {
    status ^= 1;
    digitalWrite(PIN_LED, status);
    delay(250);
  }
}

void clearAndHome(void)
{
  Serial.write(0x1b); // ESC
  Serial.print(F("[2J")); // clear screen
  Serial.write(0x1b); // ESC
  Serial.print(F("[H")); // cursor to home
}

void print_menu_putty(void)
{
  // Print menu
  Serial.print(F(" FX: "));
  if(bypass)
    Serial.println(F("BYP"));
  else
    Serial.println(F("ON"));
  Serial.println(F(""));  
  if(func_counter==0)
    Serial.print(F("    "));
  Serial.print(F(" Thr: "));
  Serial.print(param1_value, 1);
  Serial.println(F("dB"));
  if(func_counter==1)
    Serial.print(F("    "));
  Serial.print(F(" Ratio: "));
  if(param2_index==1)
    Serial.println(F("1"));
  if(param2_index==2)
    Serial.println(F("2"));
  if(param2_index==3)
    Serial.println(F("6"));
  if(param2_index==4)
    Serial.println(F("8"));
  if(func_counter==2)
    Serial.print(F("    "));
  Serial.print(F(" Attack: "));
  Serial.print(param3_value, 1);
  Serial.println(F("ms"));
  if(func_counter==3)
    Serial.print(F("    "));
  Serial.print(F(" Hold: "));
  Serial.print(param4_value, 1);
  Serial.println(F("ms"));
  if(func_counter==4)
    Serial.print(F("    "));
  Serial.print(F(" Decay: "));
  Serial.print(param5_value, 1);
  Serial.println(F("ms"));
  if(func_counter==5)
    Serial.print(F("    "));
  Serial.print(F(" Postgain: "));
  Serial.print(param6_value, 1);
  Serial.println(F("dB"));
  if(func_counter==6)
    Serial.print(F("    "));
  Serial.print(F(" M. Vol: "));
  Serial.print(param7_value, 1);
  Serial.println(F("dB"));
  
  Serial.println(F(""));
  Serial.println(F(""));
  Serial.print(F(" Active item: "));
  Serial.println(func_counter, DEC);
}
