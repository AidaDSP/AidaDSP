/*
 AIDA Tutorial_8 Sketch
 	
 This sketch controls a distortion hard & soft (tube) clipping with pre and post equalization & gain using the 
 structure of Template1.
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
#include "AidaFW.h"
#include "AidaDSP.h"

#define EVER (;;)

// DEFINES I/O
#define PIN_LED  13

// DEFINES USER INTERFACE
#define VOLMAX 0.00
#define VOLMIN -80.00

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
  Serial.print(F("0x"));
  Serial.println((DEVICE_ADDR_7bit<<1)&~0x01, HEX); // Write DSP I2C address

  // DSP board
  InitAida();	// Initialize DSP board
  digitalWrite(RESET, HIGH); // Wake up DSP
  delay(100);  // Start-up delay for DSP
  program_download();    // Here we load program, parameters and hardware configuration to DSP
  delay(20);
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
      function = 0;  // No function triggered on switch
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
  if(timec-prevtimec >= 750)  // Here we manage control interface every 750ms
  {
    clearAndHome();    // !!!Warning use with real terminal emulation program
    Serial.println(F("********************************"));
    Serial.println(F("*    User control interface    *"));
    Serial.println(F("*    AIDA Tutorial_8 Sketch    *"));
    Serial.println(F("********************************"));
    Serial.println(F("Press button rapidly to switch mute on/off,"));
    Serial.println(F("press button for 1 sec to enter submenu."));
    Serial.println("");

    Serial.print(F(" Encoder pulses: "));
    Serial.println(getPulses(), DEC);
    Serial.println("");
    Serial.println("");
    if(mute == OFF)
    {
      if(submenu == OFF)
      {
        if(restoreflag == true)
        {
          restoreflag = false;
          setPulses(OldPulses);
        }
        volume = processencoder(VOLMIN, VOLMAX, getPulses()); // dB
        Serial.print(F(" Master Vol. : "));
        Serial.print(volume, 1);
        Serial.println(F("dB"));
        MasterVolumeStereo(DEVICE_ADDR_7bit, MasterVolumeAddr, pow(10, volume/20)); // Call Aida DSP API with linear value from dB
      }
      else
      {
        MasterVolumeStereo(DEVICE_ADDR_7bit, MasterVolumeAddr, pow(10, volume/20)); // To re-enable volume after mute switch off
      }			
    }
    else if(mute == ON)
    {
      MasterVolumeStereo(DEVICE_ADDR_7bit, MasterVolumeAddr, 0.00);
      Serial.println(" mute on");
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
        Serial.println(F(" Distortion Off..."));
        mux(DEVICE_ADDR_7bit, MuxAddr, 1, 3);
        break;
      case 2:
        Serial.println(F(" Hard Clip..."));
        hard_clip(DEVICE_ADDR_7bit, HardClip1Addr, 0.5, -0.5);
        mux(DEVICE_ADDR_7bit, MuxAddr, 2, 3);
        break;
      case 3:
        Serial.println(F(" Soft Clip..."));
        soft_clip(DEVICE_ADDR_7bit, SoftClip1Addr, 0.3);
        mux(DEVICE_ADDR_7bit, MuxAddr, 3, 3);
        break;
      case 4:
        Serial.println(F(" Name this effect..."));
        // Do something call Aida DSP API
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


