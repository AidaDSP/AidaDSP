/*
 AIDA Tutorial_2 Sketch
 	
 This sketch controls a stereo pair of HP filters and LP filters of the 1st order 
 using the structure of Template2.  
 This sketch was written for Stellaris/TivaC Launchpad, and will not work on other boards.
 	
 The circuit:
 
 Audio:
 * Input range 2.0Vrms (5.66Vpp, 8.23dBu)
 * Output range 0.9Vrms (0-2.5Vpp, 1.30dBu) 
 
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
#include "AidaFW.h"
#include "AidaDSP.h"

#define EVER (;;)

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

equalizer_t equalizer1, equalizer2, equalizer3, equalizer4;    // Instantiate resources for left HP left LP right HP right LP
int tmpaddress = 0;

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
  Serial.println(F("Aida DSP control with STELLARIS")); // Welcome message
  Serial.print(F("0x"));
  Serial.println((DEVICE_ADDR_7bit<<1)&~0x01, HEX);

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

  timec = millis();
  if(timec-prevtimec >= 1000)  // Here we manage control interface every second
  {
    clearAndHome();    // !!!Warning use with real terminal emulation program
    Serial.println(F("********************************"));
    Serial.println(F("*    User control interface    *"));
    Serial.println(F("*    AIDA Tutorial_2 Sketch    *"));
    Serial.println(F("********************************"));
    Serial.println(F("Press button rapidly to switch mute on/off,"));
    Serial.println(F("press button for 1 sec to enter submenu."));
    Serial.println("");

    Serial.print(F(" Encoder pulses: "));
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
        volume = processencoder(VOLMIN, VOLMAX, getPulses()); // dB
        Serial.print(F(" Master Vol. : "));
        Serial.print(volume, 1);
        Serial.println(F("dB"));
        MasterVolumeStereo(DEVICE_ADDR_7bit, MasterVolume, pow(10, volume/20)); // Call Aida DSP API with linear value from dB
      }
      else
      {
        MasterVolumeStereo(DEVICE_ADDR_7bit, MasterVolume, volume); // To re-enable volume after mute switch off
      }			
    }
    else if(mute == ON)
    {
      MasterVolumeStereo(DEVICE_ADDR_7bit, MasterVolume, 0.00);
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
        Serial.println(F(" Voice Box")); 
        // Setup HP LP blocks parameters to eliminate bass response,
        // producing small radio or walkie-talkie effect to a voice or instrument
        equalizer1.type = Highpass;
        equalizer2.type = Lowpass;
        equalizer3.type = Highpass;
        equalizer4.type = Lowpass;

        equalizer1.f0 = 2500.00; // hp
        equalizer2.f0 = 8000.00; // lp
        equalizer3.f0 = 2500.00; // hp
        equalizer4.f0 = 8000.00; // lp

        equalizer1.gain = 6.00;
        equalizer2.gain = 6.00;
        equalizer3.gain = 6.00;
        equalizer4.gain = 6.00;

        equalizer1.onoff = true;
        equalizer2.onoff = true;
        equalizer3.onoff = true;
        equalizer4.onoff = true;

        // Write them to DSP
        tmpaddress = Gen1stOrder1;
        EQ1stOrd(DEVICE_ADDR_7bit, tmpaddress, &equalizer1);
        tmpaddress+=3;
        EQ1stOrd(DEVICE_ADDR_7bit, tmpaddress, &equalizer2);
        tmpaddress+=3;
        EQ1stOrd(DEVICE_ADDR_7bit, tmpaddress, &equalizer3);
        tmpaddress+=3;
        EQ1stOrd(DEVICE_ADDR_7bit, tmpaddress, &equalizer4);
        break;
      case 2:
        Serial.println(F(" Bypass")); // Eliminate the FX shutting down equalizer cells
        equalizer1.onoff = false;
        equalizer2.onoff = false;
        equalizer3.onoff = false;
        equalizer4.onoff = false;

        // Write them to DSP
        tmpaddress = Gen1stOrder1;
        EQ1stOrd(DEVICE_ADDR_7bit, tmpaddress, &equalizer1);
        tmpaddress+=3;
        EQ1stOrd(DEVICE_ADDR_7bit, tmpaddress, &equalizer2);
        tmpaddress+=3;
        EQ1stOrd(DEVICE_ADDR_7bit, tmpaddress, &equalizer3);
        tmpaddress+=3;
        EQ1stOrd(DEVICE_ADDR_7bit, tmpaddress, &equalizer4);
        break;
      case 3:
        Serial.println(F(" Bypass")); // Eliminate the FX shutting down equalizer cells
        equalizer1.onoff = false;
        equalizer2.onoff = false;
        equalizer3.onoff = false;
        equalizer4.onoff = false;

        // Write them to DSP
        tmpaddress = Gen1stOrder1;
        EQ1stOrd(DEVICE_ADDR_7bit, tmpaddress, &equalizer1);
        tmpaddress+=3;
        EQ1stOrd(DEVICE_ADDR_7bit, tmpaddress, &equalizer2);
        tmpaddress+=3;
        EQ1stOrd(DEVICE_ADDR_7bit, tmpaddress, &equalizer3);
        tmpaddress+=3;
        EQ1stOrd(DEVICE_ADDR_7bit, tmpaddress, &equalizer4);
        break;
      case 4:
        Serial.println(F(" Bypass")); // Eliminate the FX shutting down equalizer cells
        equalizer1.onoff = false;
        equalizer2.onoff = false;
        equalizer3.onoff = false;
        equalizer4.onoff = false;

        // Write them to DSP
        tmpaddress = Gen1stOrder1;
        EQ1stOrd(DEVICE_ADDR_7bit, tmpaddress, &equalizer1);
        tmpaddress+=3;
        EQ1stOrd(DEVICE_ADDR_7bit, tmpaddress, &equalizer2);
        tmpaddress+=3;
        EQ1stOrd(DEVICE_ADDR_7bit, tmpaddress, &equalizer3);
        tmpaddress+=3;
        EQ1stOrd(DEVICE_ADDR_7bit, tmpaddress, &equalizer4);
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

