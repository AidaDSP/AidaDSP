/*
 AIDA Template Sketch
 	
 This sketch is a Template to start coding with
 Aida DSP.
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

#define EVER (;;)

// DEFINES I/O
#define PIN_LED  13

// DEFINES USER INTERFACE
#define VOLMAX 6.00 // dB
#define VOLMIN -80.00 // dB

#define ON 1
#define OFF 0

// FUNCTION PROTOTYPES
void spettacolino();
void clearAndHome(void);

// GLOBAL VARIABLES
// ENCODER
int32_t OldPulses = 0;

// UI
uint8_t count = 0;
uint8_t function = 0;
uint8_t mute = OFF;
uint8_t submenu = OFF;
uint8_t preset = 0;
uint8_t restore = OFF;
uint8_t restoreflag = false;
uint16_t PotValue = 0;
uint32_t timec=0, prevtimec=0;

float volume = 0.00;

void setup()
{
  // put your setup code here, to run once:
  // I/O
  pinMode(PIN_LED, OUTPUT);

  // open the USBSerial port
  Serial.begin(115200);
  clearAndHome();
  Serial.println(F("Aida DSP control with ARDUINO")); // Welcome message
  Serial.println(F("0x00")); // Print DSP I2C address
 
  InitAida(); // Initialize DSP board 
  // ... download of program ... (see other examples to know what to do here)
  spettacolino();
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
      function = 0;  // No function triggered switch in idle state
    count = 0;
    digitalWrite(PIN_LED, LOW);
  }

  if(function==1)
  {
    mute ^= 1;
  }
  else if(function==2)
  {
    submenu ^=1; 
  }

  timec = millis();
  if(timec-prevtimec >= 1000)  // Here we manage control interface every second
  {
    clearAndHome();    // !!!Warning use with real terminal emulation program
    Serial.println(F("********************************"));
    Serial.println(F("*    User control interface    *"));
    Serial.println(F("*    AIDA Template1 Sketch     *"));
    Serial.println(F("********************************"));
    Serial.println(F("Press button rapidly to switch mute on/off,"));
    Serial.println(F("press button for 1 sec to enter submenu."));
    Serial.println(F(""));
    Serial.print(F(" Encoder pulses: "));
    Serial.println(getPulses(), DEC);
    Serial.println(F(""));
    if(mute == OFF)
    {
      if(submenu == OFF)
      {
        if(restoreflag == true)
        {
          restoreflag = false;
          setPulses(OldPulses);
        }
        volume = processencoder(VOLMIN, VOLMAX, getPulses()); // Obtain value in dB from digital encoder
        // Here you should call Aida DSP API to set volume, see other examples
        Serial.print(F(" Master Vol. : "));
        Serial.print(volume, 1);
        Serial.println(F("dB"));
      }	
    }
    else if(mute == ON)
    {
      Serial.println(F(" mute on"));
      // Here you should call Aida DSP API to set volume = 0.00 (or -inf in dB), see other examples
    }
    if(submenu==ON)
    {
      if(restoreflag == false)
      {
        restoreflag = true;
        OldPulses = getPulses();  // Save actual Pulses for restoring when exit menu
        setPulses(16);  // Restart from a known position
      }
      preset = (uint8_t)selectorwithencoder(getPulses(), 2);  // Use the encoder as a selector
  
      switch(preset)
      {
      case 1:
        Serial.println(F(" Name this preset..."));
        break;
      case 2:
	Serial.println(F(" Name this preset..."));
        break;
      case 3:
	Serial.println(F(" Name this preset..."));
        break;
      case 4:
	Serial.println(F(" Name this preset..."));
        break;
      }

      Serial.println("");
      Serial.print(F("    Selected preset: "));
      Serial.println(preset, DEC);

    }
    else if(submenu==OFF)
    {
      Serial.println("");
      Serial.println(F("    Submenu OFF"));
    }

    prevtimec = timec;
  } 

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


