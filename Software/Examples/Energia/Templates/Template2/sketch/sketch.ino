/*
 AIDA Template2 Sketch
 	
 This sketch is a Template to start coding with
 Aida DSP.
 This sketch was written for Stellaris/TivaC Launchpad, and will not work on other boards.
 	
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
 
 */

#include <Energia.h>
#include <pins_energia.h>
#include <Wire.h>
#include "AidaDSP.h"

#define EVER (;;)

// DEFINES USER INTERFACE
#define MAX 20.00
#define MIN -40.00

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
uint8_t restore = 0;

uint16_t param2_index = 1;

int32_t param1_pulses = 0;
int32_t param2_pulses = 0;
int32_t param3_pulses = 0;
int32_t param4_pulses = 0;

uint32_t timec=0, prevtimec=0;

float param1_value = 0.00;
float param3_value = 0.00;
float param4_value = 0.00;

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
  Serial.println(F("Aida DSP control with LAUNCHPAD")); // Welcome message
  Serial.println(F("0x00"));    // Print DSP I2C address

  InitAida(); // Initialize DSP board 
  // ... download of program ... (see other examples to know what to do here)
  spettacolino();
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
    digitalWrite(GREEN_LED, LOW);
  }
  
  if(push_e_function==1)
  {
    func_counter++;
    if(func_counter==4)
      func_counter=0;
  }
  else if(push_e_function==2)
  {
    bypass ^=1; 
  }

  timec = millis();
  if(timec-prevtimec >= 1000)  // Here we manage control interface every second
  { 
    clearAndHome();    // !!!Warning use with real terminal emulation program
    Serial.println(F("********************************"));
    Serial.println(F("*    User control interface    *"));
    Serial.println(F("*    AIDA Template2 Sketch     *"));
    Serial.println(F("********************************"));
    Serial.println(F(""));
    Serial.print(F(" Encoder pulses: "));
    Serial.println(getPulses(), DEC);
    Serial.println(F(""));
    
    if(oldbypass != bypass)
    {
      if(bypass == ON)
      {
        // Do something, call Aida DSP API
      }
      else
      {
        // Do something, call Aida DSP API
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
      param1_value = processencoder(MIN, MAX, param1_pulses);
      // Do something, call Aida DSP API
      break;
    case 1: // PARAM2
      if(restore)
      {
        restore = 0;
        setPulses(param2_pulses);
      }
      param2_pulses = getPulses();
      param2_index = selectorwithencoder(param2_pulses, 2); 
      switch(param2_index) 
      {
        case 1:
          // Do something, call Aida DSP API
          break;
        case 2:
          // Do something, call Aida DSP API
          break;
        case 3:
          // Do something, call Aida DSP API
          break;
        case 4:
          // Do something, call Aida DSP API
          break;
      }
      break;
    case 2: // PARAM3
      if(restore)
      {
        restore = 0;
        setPulses(param3_pulses);
      }
      param3_pulses = getPulses();
      param3_value = processencoder(MIN, MAX, param3_pulses);
      // Do something, call Aida DSP API
      break;
    case 3: // Color
      if(restore)
      {
        restore = 0;
        setPulses(param4_pulses);
      }
      param4_pulses = getPulses();
      param4_value = processencoder(MIN, MAX, param4_pulses);
      // Do something, call Aida DSP API
      break;
    } // End switch func_counter

    // Display information for user
    print_menu_putty();

    prevtimec = timec;
  } // End if 1000ms tick
} // End void loop

void spettacolino(void)
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
  Serial.print(F("[2J")); // clear screen
  Serial.write(0x1b); // ESC
  Serial.print(F("[H")); // cursor to home
}

void print_menu_putty(void)
{
  // Print menu
  Serial.print(F(" Effect status: "));
  if(bypass)
    Serial.println(F("bypass"));
  else
    Serial.println(F("on"));
  Serial.println(F(""));  
  if(func_counter==0)
    Serial.print(F("    "));
  Serial.print(F(" Param 1: "));
  Serial.print(param1_value, 1);
  Serial.println(F(""));
  if(func_counter==1)
    Serial.print(F("    "));
  Serial.print(F(" Param 2: "));
  if(param2_index==1)
    Serial.println(F("one"));
  if(param2_index==2)
    Serial.println(F("two"));
  if(param2_index==3)
    Serial.println(F("three"));
  if(param2_index==4)
    Serial.println(F("four"));
  if(func_counter==2)
    Serial.print(F("    "));
  Serial.print(F(" Param 3: "));
  Serial.print(param3_value, 1);
  Serial.println(F(""));
  if(func_counter==3)
    Serial.print(F("    "));
  Serial.print(F(" Param 4: "));
  Serial.print(param4_value, 1);
  Serial.println(F(""));
  Serial.println(F(""));
  Serial.print(F(" Active item: "));
  Serial.println(func_counter, DEC);
}
