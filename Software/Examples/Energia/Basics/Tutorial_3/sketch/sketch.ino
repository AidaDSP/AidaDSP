/*
 AIDA Tutorial_3 Sketch
 	
 This sketch controls selection of variuos 4 band parametric equalizer presets
 using the structure of Template1.
 User can modify the presets in order to adjust for his own requirements. 
 Preset 1 = flat;
 Preset 2 = acoustic guitar;
 Preset 3 = flat;
 Preset 4 = flat;
 Please note that most of presets are empty (flat). Feel free to experiment with these
 parametric equalizers, and take confidence with Aida DSP API creating your own presets! 
 Don't forget to leave a flat equalizer preset to have an ear comparison of how it affects the sound!
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

#define Q_HIGH   3.00f
#define Q_LOW    1.41f
#define FREQ1    61.00f;
#define FREQ2    500.00f;
#define FREQ3    2228.00f;
#define FREQ4    10100.00f;

// FUNCTION PROTOTYPES
void spettacolino(void);
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

equalizer_t equalizer1, equalizer2, equalizer3, equalizer4;    // Instantiate resources for a 4 band equalizer
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
  Serial.println(F("Aida DSP control with LANCHPAD")); // Welcome message
  Serial.print(F("0x"));
  Serial.println((DEVICE_ADDR_7bit<<1)&~0x01, HEX);

  // DSP board
  InitAida();	// Initialize DSP board
  digitalWrite(RESET, HIGH); // Wake up DSP
  delay(100);  // Start-up delay for DSP
  program_download(); // Here we load program, parameters and hardware configuration to DSP
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
    Serial.println(F("*    AIDA Tutorial_3 Sketch    *"));
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
        volume = processencoder(VOLMIN, VOLMAX, getPulses());
        Serial.print(F(" Master Vol. "));
        Serial.print(volume, 1);
        Serial.println(F("dB"));
        MasterVolumeStereo(DEVICE_ADDR_7bit, MasterVolume, pow(10, volume/20));
      }
      else
      {
        MasterVolumeStereo(DEVICE_ADDR_7bit, MasterVolume, pow(10, volume/20)); // To re-enable volume after mute switch off
      }			
    }
    else if(mute == ON)
    {
      MasterVolumeStereo(DEVICE_ADDR_7bit, MasterVolume, 0.00);
      Serial.println(F(" mute on"));
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

      equalizer1.Q = 1.41;
      equalizer1.boost = 0.00;
      equalizer1.f0 = FREQ1;
      equalizer1.type = Peaking;
      equalizer1.phase = true;
      equalizer1.onoff = true;

      equalizer2.Q = 1.41;
      equalizer2.boost = 0.00;
      equalizer2.f0 = FREQ2;
      equalizer2.type = Peaking;
      equalizer2.phase = true;
      equalizer2.onoff = true;

      equalizer3.Q = 1.41;
      equalizer3.boost = 0.00;
      equalizer3.f0 = FREQ3;
      equalizer3.type = Peaking;
      equalizer3.phase = true;
      equalizer3.onoff = true;

      equalizer4.Q = 1.41;
      equalizer4.boost = 0.00;
      equalizer4.f0 = FREQ4;
      equalizer4.type = Peaking;
      equalizer4.phase = true;
      equalizer4.onoff = true;

      switch(preset)
      {
      case 1:
        Serial.println(F(" Bypass"));
        equalizer1.boost = 0.00;
        equalizer2.boost = 0.00;
        equalizer3.boost = 0.00;
        equalizer4.boost = 0.00;
        tmpaddress = MidEQ1;
        EQ2ndOrd(DEVICE_ADDR_7bit, tmpaddress, &equalizer1);
        tmpaddress+=5; 
        EQ2ndOrd(DEVICE_ADDR_7bit, tmpaddress, &equalizer2);
        tmpaddress+=5; 
        EQ2ndOrd(DEVICE_ADDR_7bit, tmpaddress, &equalizer3);
        tmpaddress+=5;
        EQ2ndOrd(DEVICE_ADDR_7bit, tmpaddress, &equalizer4);
        break;
      case 2:
        Serial.println(F(" Acoustic Guitar"));
        equalizer1.boost = -3.00;
        equalizer2.boost = 9.20;
        equalizer3.boost = -7.60;
        equalizer4.boost = 1.20;
        tmpaddress = MidEQ1;
        EQ2ndOrd(DEVICE_ADDR_7bit, tmpaddress, &equalizer1);
        tmpaddress+=5; 
        EQ2ndOrd(DEVICE_ADDR_7bit, tmpaddress, &equalizer2);
        tmpaddress+=5; 
        EQ2ndOrd(DEVICE_ADDR_7bit, tmpaddress, &equalizer3);
        tmpaddress+=5;
        EQ2ndOrd(DEVICE_ADDR_7bit, tmpaddress, &equalizer4);
        break;
      case 3:
        Serial.println(F(" Bypass"));
        equalizer1.boost = 0.00;
        equalizer2.boost = 0.00;
        equalizer3.boost = 0.00;
        equalizer4.boost = 0.00;
        tmpaddress = MidEQ1;
        EQ2ndOrd(DEVICE_ADDR_7bit, tmpaddress, &equalizer1);
        tmpaddress+=5; 
        EQ2ndOrd(DEVICE_ADDR_7bit, tmpaddress, &equalizer2);
        tmpaddress+=5; 
        EQ2ndOrd(DEVICE_ADDR_7bit, tmpaddress, &equalizer3);
        tmpaddress+=5;
        EQ2ndOrd(DEVICE_ADDR_7bit, tmpaddress, &equalizer4);
        break;
      case 4:
        Serial.println(F(" Bypass"));
        equalizer1.boost = 0.00;
        equalizer2.boost = 0.00;
        equalizer3.boost = 0.00;
        equalizer4.boost = 0.00;
        tmpaddress = MidEQ1;
        EQ2ndOrd(DEVICE_ADDR_7bit, tmpaddress, &equalizer1);
        tmpaddress+=5; 
        EQ2ndOrd(DEVICE_ADDR_7bit, tmpaddress, &equalizer2);
        tmpaddress+=5; 
        EQ2ndOrd(DEVICE_ADDR_7bit, tmpaddress, &equalizer3);
        tmpaddress+=5;
        EQ2ndOrd(DEVICE_ADDR_7bit, tmpaddress, &equalizer4);
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
  Serial.print("[2J"); // clear screen
  Serial.write(0x1b); // ESC
  Serial.print("[H"); // cursor to home
}

