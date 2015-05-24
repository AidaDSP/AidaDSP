/*
 AIDA Tutorial_8 Sketch
 	
 This sketch controls selection of 2 type of distortion. They're not
 good for musical usage because they're not filtered. 
 Try to add equalization by yourself...mixing together all previous examples!!! 
 Soft clip is basically a compressed distortion usually found in tube amps. 
 Hard clip is diode-like distortion basically the one you find in stomp-boxes.
 Both can be symmetrical or asymmetrical, soft clip is biased by a DC source to do so. 
 User can modify the presets in order to adjust for his own requirements. 
 Preset 1 = ;
 Preset 2 = ;
 Preset 3 = ;
 Preset 4 = ;
 This sketch was written for the Stellaris, and will not work on other boards.
 	
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
uint32_t time=0, prevtime=0;

float volume = 0.00;

float alpha = 0.00, th_high = 0.00, th_low = 0.00;

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
  MasterVolume(DEVICE_ADDR_7bit, Single1, 0.00);    // With DAC in mute, set volume to 0
  delay(1);   
  AIDA_WRITE_REGISTER_BLOCK(DEVICE_ADDR_7bit, CoreRegisterR4Addr, CoreRegisterR4Size, CoreRegisterR4Data);    // Mute DAC Off
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
    Serial.println("*    AIDA Tutorial_8 Sketch    *");
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
        volume = processencoder(VOLMIN, VOLMAX, getPulses());

        Serial.print("Master Vol. ");
        Serial.print(volume, 1);
        Serial.println("dB");

        volume = pow(10, volume/20);    // From dB to linear conversion --> DSP takes only linear values in 5.28 fixed point format!!!
        MasterVolume(DEVICE_ADDR_7bit, Single1, volume);
      }
      else
      {
        MasterVolume(DEVICE_ADDR_7bit, Single1, volume);
      }			
    }
    else if(mute == ON)
    {
      MasterVolume(DEVICE_ADDR_7bit, Single1, 0.00);
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
      preset = (uint8_t)selectorwithencoder(getPulses(), 2);  // Use the encoder as a selector

      switch(preset)
      {
      case 1:
        Serial.println(" Bypass");          
        mux(DEVICE_ADDR_7bit, Nx2_1, 1, 3);   // Select 1 of 3
        break;
      case 2:
        Serial.println(" Hard Clip");
        Serial.print("    th: ");
        th_high = 0.5;
        th_low = -th_high;
        Serial.print(th_high, 3);
        Serial.write(' ');
        Serial.println(th_low, 3);
        hard_clip(DEVICE_ADDR_7bit, HardClip1, th_high, th_low);
        mux(DEVICE_ADDR_7bit, Nx2_1, 2, 3);   // Select 1 of 3
        break;
      case 3:
        Serial.println(" Soft Clip");
        Serial.print("    alpha: ");
        alpha = 2.0;
        Serial.println(alpha, 1);
        dc_source(DEVICE_ADDR_7bit, DC1, 0); // Set DC bias to 0%
        delay(1);
        soft_clip(DEVICE_ADDR_7bit, SoftClip1, alpha); 
        delay(1);
        mux(DEVICE_ADDR_7bit, Nx2_1, 3, 3);   // Select 1 of 3
        break;
      case 4:
        Serial.println(" Bypass");          
        mux(DEVICE_ADDR_7bit, Nx2_1, 1, 3);   // Select 1 of 3
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

