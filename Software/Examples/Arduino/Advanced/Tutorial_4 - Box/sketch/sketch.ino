/*
 AIDA Tutorial_4 Sketch
 	
 This sketch controls selection of variuos 4 band parametric equalizer presets. 
 User can modify the presets in order to adjust for his own requirements. 
 Preset 1 = flat;
 Preset 2 = acoustic guitar;
 Preset 3 = flat;
 Preset 4 = flat;
 This sketch was written for the Arduino 2 / Arduino Mega R3, and will not work on other boards.
 	
 The circuit:
 
 Audio:
 * Input range 2.0Vrms (5.66Vpp, 8.23dBu)
 * Output range 0.9Vrms (0-2.5Vpp, 1.30dBu) 
 
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
#include "LiquidCrystal.h"

#define EVER (;;)

// DEFINES USER INTERFACE
#define VOLMAX 6.00
#define VOLMIN -80.00

#define ON 1
#define OFF 0

#define Q_HIGH   3.00f
#define Q_LOW    1.41f
#define FREQ1    61.00f;
#define FREQ2    500.00f;
#define FREQ3    2228.00f;
#define FREQ4    10100.00f;

#define PIN_LED  13
#define LED_1    23
#define LED_2    25
#define PUSH_1   18
#define PUSH_2   19

// FUNCTION PROTOTYPES
void spettacolino();
void clearAndHome(void);
void check_program(void);
void check_param(void);
void check_config(void);

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
float pot1 = 0.00;

equalizer_t equalizer1, equalizer2, equalizer3, equalizer4;    // Instantiate resources for a 4 band equalizer
uint16_t tmpaddress = 0;

// Configure pins for LCD display
LiquidCrystal lcd(17, 16, 15, 14, 6, 7); // RS, EN, D4, D3, D2, D1

void setup()
{
  // put your setup code here, to run once:
  // I/O
  pinMode(PIN_LED, OUTPUT);
  pinMode(LED_1, OUTPUT);
  digitalWrite(LED_1, HIGH);
  pinMode(LED_2, OUTPUT);
  digitalWrite(LED_2, HIGH);
  pinMode(PUSH_1, INPUT_PULLUP);
  pinMode(PUSH_2, INPUT_PULLUP);

  // open the USBSerial port
  Serial.begin(115200);
  clearAndHome();
  Serial.println(F("Aida DSP control with ARDUINO")); // Welcome message
  Serial.print(F("0x"));
  Serial.println((DEVICE_ADDR_7bit<<1)&~0x01, HEX);

  // LCD Display
  lcd.begin(16, 2); // set up the LCD's number of columns and rows
  lcd.setCursor(0, 0);
  lcd.print("Aida DSP Box"); // Print a message to the LCD.
  lcd.setCursor(0, 1);
  lcd.print("V0.1");
  
  // DSP board
  InitAida();	// Initialize DSP board
  digitalWrite(RESET, HIGH); // Wake up DSP
  delay(250);  // Start-up delay for DSP
  program_download();    // Here we load program, parameters and hardware configuration to DSP
  delay(20);
  check_program(); // !!!Debug!!!
  delay(5);
  check_param(); // !!!Debug!!!
  delay(5);
  check_config(); // !!!Debug!!!
  delay(2);
  spettacolino();
  MasterVolumeStereo(DEVICE_ADDR_7bit, Single1, 1.00);    // With DAC in mute, set volume to 0dB
  delay(1);   
  AIDA_WRITE_REGISTER_BLOCK(DEVICE_ADDR_7bit, CoreRegisterR4Addr, CoreRegisterR4Size, CoreRegisterR4Data);    // Mute DAC Off
}

void loop()
{
  // put your main code here, to run repeatedly:
  
  if(digitalRead(ENC_PUSH)==LOW)  
  {
    digitalWrite(PIN_LED, HIGH);
    digitalWrite(LED_1, LOW);
    digitalWrite(LED_2, LOW);
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
    digitalWrite(LED_1, HIGH);
    digitalWrite(LED_2, HIGH);
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
    pot1 = analogRead(POT1);
    pot1 = processpot(VOLMIN, VOLMAX, pot1);
    //lcd.setCursor(5, 1);
    //lcd.print(pot1);
    //lcd.print(F("dB"));
    
    clearAndHome();    // !!!Warning use with real terminal emulation program
    Serial.println(F("********************************"));
    Serial.println(F("*    User control interface    *"));
    Serial.println(F("*    AIDA Tutorial_4 Sketch    *"));
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
        
        lcd.setCursor(5, 1);
        lcd.print(volume,1);
        lcd.print(F("dB"));

        volume = pow(10, volume/20);    // From dB to linear conversion --> DSP takes only linear values in 5.28 fixed point format!!!
        MasterVolumeStereo(DEVICE_ADDR_7bit, Single1, volume);
      }
      else
      {
        MasterVolumeStereo(DEVICE_ADDR_7bit, Single1, volume);
      }			
    }
    else if(mute == ON)
    {
      MasterVolumeStereo(DEVICE_ADDR_7bit, Single1, 0.00);
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
      
      Serial.write('\n');
      Serial.print(F("    Selected preset: "));
      Serial.println(preset, DEC);

    }
    else if(submenu==OFF)
    {
      Serial.write('\n');
      Serial.println(F("    Submenu OFF"));
    }
    prevtimec = timec;
  } 

} // End void loop

void spettacolino()
{
  byte i;
  byte statusc = 0x00;

  for(i=0;i<6;i++)
  {
    statusc ^= 1;
    pinMode(PIN_LED, statusc);
    delay(250);
  }
  pinMode(PIN_LED, HIGH);
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

