/*
 AIDA Template Sketch
 	
 This sketch is a the first attempt to write program and control ADAU144x DSP.
 It uses the newest version of Aida DSP API to test Sw Safeload feature.
 This sketch was written for Arduino, and will not work on other boards.
 	
 The circuit:
 I/O:
 SBOOT   12
 RESET   11
 ENCB     3 (Interrupt)
 ENCA     5
 PUSH1    2
 LED     13
 
 Audio:
 * Input range ???
 * Output range ???
 
 PC:
 * Please connect with PuTTY on Arduino USB Serial with a PC for a minimal user interface
 
 NOTE:
 Further modifications on main Aida DSP API will follow. It takes time because I want
 to support both ADAU170x and ADAU144x series within the same API library.
 
 created August 2015
 by Massimo Pennazio
 
 This example code is in the public domain.
 
 */

#include <Arduino.h>
#include <pins_arduino.h>
#include <Wire.h>
#include "AidaFW.h"
#include "AidaDSP.h"

#define EVER (;;)

// DEFINES USER INTERFACE
#define VOLMAX 0.00
#define VOLMIN -80.00

#define ON 1
#define OFF 0

// I/O
#define PIN_LED  13

// FUNCTION PROTOTYPES
void spettacolino(void);
void clearAndHome(void);
void VolumeControl(float value);

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
uint32_t time=0, prevtime=0;

float volume = 0.00;

void setup()
{
  // put your setup code here, to run once:
  // I/O
  pinMode(PIN_LED, OUTPUT);

  // open the USBSerial port
  Serial.begin(115200);
  clearAndHome();
  Serial.println(F("Aida ADAU144x control with ARDUINO")); // Welcome message
  Serial.print(F("0x"));
  Serial.println((DEVICE_ADDR_7bit<<1)&~0x01, HEX);

  // DSP board
  InitAida();	// Initialize DSP board
  digitalWrite(RESET, HIGH); // Wake up DSP
  delay(100);  // Start-up delay for DSP
  program_download();    // Here we load program, parameters and hardware configuration to DSP
  spettacolino();
  //MasterVolume(DEVICE_ADDR_7bit, Single1, 1.00);    // With DAC in mute, set volume to 1
  VolumeControl(1.00);
  delay(1);   
  //AIDA_WRITE_REGISTER_BLOCK(DEVICE_ADDR_7bit, CoreRegisterR4Addr, CoreRegisterR4Size, CoreRegisterR4Data);    // Mute DAC Off
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

  time = millis();
  if(time-prevtime >= 1000)  // Here we manage control interface every second
  {
    clearAndHome();    // !!!Warning use with real terminal emulation program
    Serial.println(F("********************************"));
    Serial.println(F("*    User control interface    *"));
    Serial.println(F("*    AIDA ADAU144x Sketch    *"));
    Serial.println(F("********************************"));
    Serial.println(F("Press button rapidly to switch mute on/off,"));
    Serial.println(F("press button for 1 sec to enter submenu."));
    Serial.write('\n');

    Serial.print(F("Encoder pulses: "));
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
        volume = processencoder(VOLMIN, VOLMAX, getPulses());

        Serial.print(F("Master Vol. "));
        Serial.print(volume, 1);
        Serial.println(F("dB"));

        volume = pow(10, volume/20);    // From dB to linear conversion --> DSP takes only linear values in 5.28 fixed point format!!!
        //MasterVolume(DEVICE_ADDR_7bit, Single1, volume);
        VolumeControl(volume);
      }
      else
      {
        //MasterVolume(DEVICE_ADDR_7bit, Single1, volume);
        VolumeControl(volume);
      }			
    }
    else if(mute == ON)
    {
      //MasterVolume(DEVICE_ADDR_7bit, Single1, 0.00);
      VolumeControl(0.00);
      Serial.println(F("mute on"));
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

      Serial.write('\n');
      Serial.print(F("    Selected preset: "));
      Serial.println(preset, DEC);

    }
    else if(submenu==OFF)
    {
      Serial.write('\n');
      Serial.println(F("    Submenu OFF"));
    }

    prevtime = time;
  } 

} // End void loop

void spettacolino(void)
{
  byte i;
  byte status = 0x00;

  for(i=0;i<6;i++)
  {
    status ^= 1;
	pinMode(PIN_LED, status);
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

// Write Volume linear value onto DSP with Sw Safeload
void VolumeControl(float value)  // Use this function until I adjust code for MasterVolume in Aida DSP API
{
  float data[5];
  
  memset(data, 0.00, 5);
  data[0] = value;
  
  AIDA_SW_SAFELOAD_WRITE_VALUES(DEVICE_ADDR_7bit, Single1, 1, data);
}


