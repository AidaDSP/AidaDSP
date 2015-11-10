/*
 AIDA Tutorial_7 Sketch
 	
 This sketch controls a multiple signal generator (synthesizer) with average/rms/peak/raw volume readback using the 
 structure of Template1.
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
float readback1 = 0.00;
float readback2 = 0.00;
float readback3 = 0.00;
float readback4 = 0.00;

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
  Serial.println((DEVICE_ADDR_7bit<<1)&~0x01, HEX); // Write DSP I2C address

  // DSP board
  InitAida();	// Initialize DSP board
  digitalWrite(RESET, HIGH); // Wake up DSP
  delay(100);  // Start-up delay for DSP
  program_download();    // Here we load program, parameters and hardware configuration to DSP
  delay(20);
  check_program(); // !!!Debug!!!
  delay(5);
  check_param(); // !!!Debug!!!
  delay(5);
  check_config(); // !!!Debug!!!
  delay(2);
  spettacolino();
  AIDA_WRITE_REGISTER_BLOCK( DEVICE_ADDR_7bit, CoreRegisterR4Addr, CoreRegisterR4Size, CoreRegisterR4Data ); // Mute DACs Off
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
  if(timec-prevtimec >= 750)  // Here we manage control interface every 750ms
  {
    clearAndHome();    // !!!Warning use with real terminal emulation program
    Serial.println(F("********************************"));
    Serial.println(F("*    User control interface    *"));
    Serial.println(F("*    AIDA Tutorial_7 Sketch    *"));
    Serial.println(F("********************************"));
    Serial.println(F("Press button rapidly to switch mute on/off,"));
    Serial.println(F("press button for 1 sec to enter submenu."));
    Serial.println("");

    readBack(DEVICE_ADDR_7bit, ReadBackAlg1, 0x0312, &readback1); // running average
    readBack(DEVICE_ADDR_7bit, ReadBackAlg2, 0x031E, &readback2); // rms
    readBack(DEVICE_ADDR_7bit, ReadBackAlg3, 0x032A, &readback3); // peak
    readBack(DEVICE_ADDR_7bit, ReadBackAlg4, 0x0336, &readback4); // raw abs
    
    Serial.print(F(" Running Avg. : ")); // Print linear values (max +/-1.00) for readback values
    Serial.println(readback1, 1);
    Serial.print(F(" RMS : "));
    Serial.println(readback2, 1);
    Serial.print(F(" Peak : "));
    Serial.println(readback3, 1);
    Serial.print(F(" Raw abs : "));
    Serial.println(readback4, 1);

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
        MasterVolumeStereo(DEVICE_ADDR_7bit, MasterVolume, pow(10, volume/20)); // Call Aida DSP API with linear value from dB
      }
      else
      {
        MasterVolumeStereo(DEVICE_ADDR_7bit, MasterVolume, pow(10, volume/20)); // To re-enable volume after mute switch off
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
      preset = (uint8_t)selectorwithencoder(getPulses(), 3);  // Use the encoder as a selector

      switch(preset)
      {
      case 1:
        Serial.println(F(" Sawtooth..."));
        mux(DEVICE_ADDR_7bit, Selector, 1, 4);
        mux(DEVICE_ADDR_7bit, Mux, 1, 2);
        break;
      case 2:
        Serial.println(F(" Sine..."));
        mux(DEVICE_ADDR_7bit, Selector, 2, 4);
        mux(DEVICE_ADDR_7bit, Mux, 1, 2);
        break;
      case 3:
        Serial.println(F(" Square..."));
        mux(DEVICE_ADDR_7bit, Selector, 3, 4);
        mux(DEVICE_ADDR_7bit, Mux, 1, 2);
        break;
      case 4:
        Serial.println(F(" Triangle..."));
        mux(DEVICE_ADDR_7bit, Selector, 4, 4);
        mux(DEVICE_ADDR_7bit, Mux, 1, 2);
        break;
      case 5:
        Serial.println(F(" Audio In..."));
        mux(DEVICE_ADDR_7bit, Mux, 2, 2);
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

void check_program(void) 
{
  uint8_t value_wr = 0;
  uint8_t buff_r[5];
  uint8_t value_r;
  uint16_t addr = progDataAddr;
  uint16_t i, j, errors;
  
  Serial.println(F("Program checking..."));
  
  errors = 0;
  for(i=0;i<progDataSize;i+=5) // Program address 1024 to 2047
  {
    memset(buff_r, 0, 5);
    AIDA_READ_REGISTER(DEVICE_ADDR_7bit, addr, 5, buff_r); 
    for(j=0;j<5;j++)
    {
      #ifdef __AVR__
      //value_wr = pgm_read_byte_far(&progDataData[i+j]);
      value_wr = pgm_read_byte_near(&progDataData[i+j]);
      #else
      value_wr = progDataData[i+j];
      #endif
      value_r = buff_r[j];
      if(value_wr != value_r)
      {
        errors++;
        break;
      }
    }
    if(errors)
      break;
    addr++;
    delayMicroseconds(100);
  }

  if(errors)
  {
    //Serial.print(F("i: "));
    //Serial.println(i, DEC);
    //Serial.print(F("j: "));
    //Serial.println(j, DEC);
    Serial.print(errors, DEC);
    Serial.println(F(" errors during Program download")); 
    Serial.print(F("Address: "));
    Serial.println(addr, DEC);
    Serial.print(F("Written = "));
    Serial.print(F("0x"));
    Serial.println(value_wr, HEX);
    Serial.print(F("Readed = "));
    Serial.print(F("0x"));
    Serial.println(value_r, HEX);
    while(1);
  }
  else
  {
    Serial.println(F("Program OK"));
  }
}

void check_param(void)
{
  uint8_t value_wr = 0;
  uint8_t buff_r[4];
  uint8_t value_r;
  uint16_t addr = regParamAddr;
  uint16_t i, j, errors;
  
  Serial.println(F("Parameter checking..."));
  
  errors = 0;
  for(i=0;i<regParamSize;i+=4) // 0 to 1023
  {
    memset(buff_r, 0, 4);
    AIDA_READ_REGISTER(DEVICE_ADDR_7bit, addr, 4, buff_r); 
    for(j=0;j<4;j++)
    {
      #ifdef __AVR__
      //value_wr = pgm_read_byte_far(&regParamData[i+j]);
      value_wr = pgm_read_byte_near(&regParamData[i+j]);
      #else
      value_wr = regParamData[i+j];
      #endif
      value_r = buff_r[j];
      if(value_wr != value_r)
      {
        errors++;
        break;
      }
    }
    if(errors)
      break;
    addr++;
    delayMicroseconds(100);
  }
  
  if(errors)
  {
    Serial.print(errors, DEC);
    Serial.println(F(" errors during Reg Param download")); 
    Serial.print(F("Address: "));
    Serial.println(addr, DEC);
    Serial.print(F("Written = "));
    Serial.print(F("0x"));
    Serial.println(value_wr, HEX);
    Serial.print(F("Readed = "));
    Serial.print(F("0x"));
    Serial.println(value_r, HEX);
    while(1);
  }
  else
  {
    Serial.println(F("Reg Param OK"));
  }
}

void check_config(void)
{
  uint8_t value_wr = 0;
  uint8_t buff_r[HWConFigurationSize];
  uint8_t value_r;
  uint16_t addr = HWConFigurationAddr;
  uint16_t i, errors;
  
  Serial.println(F("HW Config checking..."));
  
  errors = 0;
  memset(buff_r, 0, HWConFigurationSize);
  AIDA_READ_REGISTER(DEVICE_ADDR_7bit, addr, HWConFigurationSize, buff_r); // Read all parameters in one block   
  
  for(i=0;i<HWConFigurationSize;i++) //  2076 to 2087 
  {
    #ifdef __AVR__
    //value_wr = pgm_read_byte_far(&HWConFigurationData[i]);
    value_wr = pgm_read_byte_near(&HWConFigurationData[i]);
    #else
    value_wr = HWConFigurationData[i];
    #endif
    value_r = buff_r[i];
    if(value_wr != value_r)
    {
      errors++;
      break;
    }
  }
  
  if(errors)
  {
    Serial.print(errors, DEC);
    Serial.println(F(" errors during HW config download")); 
    Serial.print(F("Address: "));
    Serial.println(addr, DEC);
    Serial.print(F("Written = "));
    Serial.print(F("0x"));
    Serial.println(value_wr, HEX);
    Serial.print(F("Readed = "));
    Serial.print(F("0x"));
    Serial.println(value_r, HEX);
    while(1);
  }
  else
  {
    Serial.println(F("HW Config OK"));
  }
}

