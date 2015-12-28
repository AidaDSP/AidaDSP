/*
 AIDA Phaser+Flanger Sketch
 	
 This sketch is an "Phaser+Flanger" as described in...

 
 This sketch was written for the Arduino, and will not work on other boards.
 	
 The circuit:
 
 Audio:
 * Input range 2.0Vrms (5.66Vpp, 8.23dBu)
 * Output range 0.9Vrms (2.5Vpp, 1.30dBu) 
 
 PC:
 * Please connect with PuTTY on Stellaris USB Serial with a PC for a minimal user interface
 
 NOTE:
 Attenuation Out/In = 2.264, to have out = in you must provide 7.097dB of gain through DSP algorithm
 or externally with active LPF filter.
 Sigma Studio seems to send 0xFF on 0x09 address of params. This is the -1 on a triangular lookup table. 
 But then it's readed 0x0F, because 4 msb(its) are don't care in 5.23 fixed point format. 
 
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
#define VOLMAX 15.00
#define VOLMIN -80.00
#define FREQMIN 0.1f // Hz
#define FREQMAX 35.0f // Hz
#define BPMMIN 5.0f
#define BPMMAX 260.0f
//#define FREQMIN 1000.0f // Hz
//#define FREQMAX 10000.0f // Hz
#define COLORMIN -15.00f // dB
#define COLORMAX 15.00f // dB
#define POT_THR 25.0 // Threshold for filtering noise on pots (adcs)

#define ON 1
#define OFF 0

#define POT1 A0
#define POT2 A1
#define POT3 A2
#define POT4 A3

#define PIN_LED  13
#define LED_1    23
#define LED_2    25
#define PUSH_1   18
#define PUSH_2   19

// FUNCTION PROTOTYPES
void spettacolino();
void clearAndHome(void);
void setBypass(void);
void setMix(uint8_t);
void setMode(void);
void setFrequency(void);
void setLfo(void);
void setColor(float);
void setVolume(float);
void setFXType(void);
void depth(void);
void print_menu_putty(void);
void print_menu_lcd(void);

// GLOBAL VARIABLES
// ENCODER
int32_t OldPulses = 0;

// UI
uint8_t bypass = OFF;
uint8_t func_counter = 0;
uint8_t old_func_counter = 0;
uint8_t mode = 1;
uint8_t lfotype = 1;
uint8_t restore = 1;
uint8_t mix = 0;
uint8_t fx_counter = 0;
uint16_t readbackcount = 0;
int32_t freqpulses = 198;
int32_t bpmpulses = 213;
int32_t lfotypepulses = 0;
int32_t modepulses = 0;
int32_t colorpulses = 0;
int32_t volumepulses = 0;
int32_t mixpulses = 0;
int32_t depthpulses = 0;
uint32_t timec=0, prevtimec=0;

float volumedB = 0.00;
float frequency = 0.00;
float bpm = 0.00;
float colorvalue = 0.00;
float depthvalue = 0.00;
equalizer_t color;

uint16_t pot1 = 0;
uint16_t oldpot1 = 0;
uint16_t pot2 = 0;
uint16_t oldpot2 = 0;
uint16_t pot3 = 0;
uint16_t oldpot3 = 0;
uint16_t pot4 = 0;
uint16_t oldpot4 = 0;

// Push Encoder
uint8_t push_e_count = 0;
uint8_t push_e_function = 0;

// Push 1
uint8_t push_1_lock = 0;

// Push 2
uint8_t push_2_lock = 0;

uint8_t reinitdisplaycounter = 0;

// Configure pins for LCD display
LiquidCrystal lcd(17, 16, 15, 14, 6, 7); // RS, EN, D4, D5, D6, D7

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
  //attachInterrupt(5, push1_isr, FALLING); 
  pinMode(PUSH_2, INPUT_PULLUP);
  //attachInterrupt(4, push2_isr, FALLING); 

  // open the USBSerial port
  Serial.begin(115200);
  clearAndHome();
  Serial.println(F("Aida DSP control with ARDUINO")); // Welcome message
  Serial.print(F("0x"));
  Serial.println((DEVICE_ADDR_7bit<<1)&~0x01, HEX);
  
  // LCD Display
  lcd.begin(16, 2); // set up the LCD's number of columns and rows
  lcd.setCursor(0, 0);
  lcd.print(F("Aida DSP Box")); // Print a message to the LCD.
  lcd.setCursor(0, 1);
  lcd.print(F("PhaserFlang V0.1"));

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
   
  MuteOff();  // Mute DAC Off
  
  // Bypass status init = disable
  bypass = 0;
  mux(DEVICE_ADDR_7bit, Bypass, 2, 2); // FX
  digitalWrite(LED_2, LOW); // Led 2 On
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
    // Not managed yet
  }
  
  if(digitalRead(PUSH_1)==LOW)
  {
    delay(50);  // debounce
    if(digitalRead(PUSH_1)==LOW)
    {
      if(push_1_lock != 1)
      {
        push_1_lock = 1;
        fx_counter++;
        if(fx_counter == 3)
          fx_counter = 0;
      }
    }
  }
  else
  {
    push_1_lock = 0;
  }
  
  if(digitalRead(PUSH_2)==LOW)
  {
    delay(50);  // debounce
    if(digitalRead(PUSH_2)==LOW)
    {
      if(push_2_lock != 1)
      {
        push_2_lock = 1;
        bypass ^= 1;
      }
    }
  }
  else
  {
    push_2_lock = 0;
  }
  
  timec = millis();
  if(timec-prevtimec >= 250)  // Here we manage control interface every 250ms
  { 
    clearAndHome();    // !!!Warning use with real terminal emulation program
    Serial.println(F("********************************"));
    Serial.println(F("*    User control interface    *"));
    Serial.println(F("*  AIDA Phaser+Flanger Sketch  *"));
    Serial.println(F("********************************"));
    Serial.write('\n');
    Serial.print(F("Encoder pulses: "));
    Serial.println(getPulses(), DEC);
    Serial.write('\n');
    
    reinitdisplaycounter++;
    if(reinitdisplaycounter==4) // Sometimes display takes noise and corrupts its RAM...
    {
      lcd.begin(16, 2); // set up the LCD's number of columns and rows
      reinitdisplaycounter = 0;
    }
    
    setFXType(); // Using PUSH_1 and LED_1
    setBypass(); // Using PUSH_2 and LED_2
    
    pot1 = analogRead(POT1);
    if(!isinrange(pot1, oldpot1, POT_THR))
    {
      func_counter=0;
      oldpot1 = pot1;
      bpm = processpot(BPMMIN, BPMMAX, pot1);
      frequency = bpm / 60.0; // Bpm to frequency conversion
      setFrequency();
    }
    
    pot2 = analogRead(POT2);
    if(!isinrange(pot2, oldpot2, POT_THR))
    {
      func_counter=5;
      oldpot2 = pot2;
      mix = (uint8_t)processpot(0.0, 100.0, pot2);
      setMix(mix);
    }
    
    pot3 = analogRead(POT3);
    if(!isinrange(pot3, oldpot3, POT_THR))
    {
      func_counter=3;
      oldpot3 = pot3;
      colorvalue = processpot(COLORMIN, COLORMAX, pot3);
      setColor(colorvalue);
    }
    
    pot4 = analogRead(POT4);
    if(!isinrange(pot4, oldpot4, POT_THR))
    {
      func_counter=4;
      oldpot4 = pot4;
      volumedB = processpot(VOLMIN, VOLMAX, pot4);
      setVolume(volumedB);
    }
    
    if(old_func_counter != func_counter)
    {
      restore = 1;
      old_func_counter = func_counter;
    }
    switch(func_counter)
    {
    case 0: // Frequency
      /*if(restore)
      {
        restore = 0;
        //setPulses(freqpulses);
        setPulses(bpmpulses);
      }
      set_regulation_precision(ON); // Fine regulation
      //freqpulses = getPulses();
      bpmpulses = getPulses();
      //frequency = processencoder(FREQMIN, FREQMAX, freqpulses);
      bpm = processencoder(BPMMIN, BPMMAX, bpmpulses);
      frequency = bpm / 60.0;
      setFrequency();*/
      break;
    case 1: // LFO type
      if(restore)
      {
        restore = 0;
        setPulses(lfotypepulses);
      }
      lfotypepulses = getPulses();
      lfotype = selectorwithencoder(lfotypepulses, 1);
      setLfo();
      break;
    case 2: // Mode
      if(restore)
      {
        restore = 0;
        setPulses(modepulses);
      }
      modepulses = getPulses();
      mode = selectorwithencoder(modepulses, 2); 
      setMode();
      break;
    case 3: // Color
      /*if(restore)
      {
        restore = 0;
        setPulses(colorpulses);
      }
      set_regulation_precision(OFF); // Rough regulation
      colorpulses = getPulses();
      colorvalue = processencoder(COLORMIN, COLORMAX, colorpulses);
      setColor(colorvalue);*/
      break;
    case 4: // Volume
      /*if(restore)
      {
        restore = 0;
        setPulses(volumepulses);
      }
      set_regulation_precision(OFF); // Rough regulation
      volumepulses = getPulses();
      volumedB = processencoder(VOLMIN, VOLMAX, volumepulses);
      setVolume(volumedB);*/
      break;  
    case 5: // Mix
      /*if(restore)
      {
        restore = 0;
        setPulses(mixpulses);
      }
      set_regulation_precision(OFF); // Rough regulation
      mixpulses = getPulses();
      mix = (uint8_t)processencoder(0, 100, mixpulses);
      setMix(mix);*/
      break;
    case 6: // Depth
      /*if(restore)
      {
        restore = 0;
        setPulses(depthpulses);
      } 
      //set_regulation_precision(OFF); // Rough regulation
      depthpulses = getPulses();
      depthvalue = processencoder(-1.0, 1.0, depthpulses);
      //depth(); */
      break;    
    } // End switch func_counter

    // Display information for user
    //print_menu_putty();
    print_menu_lcd();

    prevtimec = timec;
  } // End if 1000ms tick
} // End void loop

void spettacolino()
{
  byte i;
  byte statusc = 0x00;

  for(i=0;i<6;i++)
  {
    statusc ^= 1;
    digitalWrite(PIN_LED, statusc);
    digitalWrite(LED_1, statusc);
    digitalWrite(LED_2, statusc);
    delay(250);
  }
  digitalWrite(PIN_LED, HIGH);
  digitalWrite(LED_1, HIGH);
  digitalWrite(LED_2, HIGH);
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
  uint16_t addr = ProgramDataAddr;
  uint16_t i, j, errors;
  
  Serial.println(F("Program checking..."));
  
  errors = 0;
  for(i=0;i<ProgramDataSize;i+=5) // Program address 1024 to 2047
  {
    memset(buff_r, 0, 5);
    AIDA_READ_REGISTER(DEVICE_ADDR_7bit, addr, 5, buff_r); 
    for(j=0;j<5;j++)
    {
      #ifdef __AVR__
      //value_wr = pgm_read_byte_far(&ProgramDataData[i+j]);
      value_wr = pgm_read_byte_near(&ProgramDataData[i+j]);
      #else
      value_wr = ProgramDataData[i+j];
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
      if(j==0)
        value_wr&=0x0F;
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

void setMix(uint8_t percent)
{
  static uint8_t oldpercent = 0;
  float value = 0.00;
  
  if(oldpercent != percent)
  {
    if(percent>100)
      percent = 100;
    value = percent/100.00;
  
    // MIX
    AIDA_SAFELOAD_WRITE_VALUE(DEVICE_ADDR_7bit, Mix, false, 1.00-value);  // Dry	 
    AIDA_SAFELOAD_WRITE_VALUE(DEVICE_ADDR_7bit, Mix+1, true, value);   // Wet
  }
}

void setFrequency(void)
{
  static float oldfrequency = 0.00;
  
  if(frequency != oldfrequency) // Freq change, update all oscillators
  { 
    triangle_source(DEVICE_ADDR_7bit, Triangle1, frequency*2.00);
    delayMicroseconds(10);
    sine_source(DEVICE_ADDR_7bit, Sine, frequency);
    delayMicroseconds(10);
    oldfrequency = frequency;
  }
}

void setLfo(void)
{
  static uint8_t lfotypeold = 0.00;
  
  if(lfotypeold != lfotype)
  {
    mux(DEVICE_ADDR_7bit, LFOType, lfotype, 2);
    lfotypeold = lfotype;
  }
}

void setMode(void)
{
  static uint8_t oldmode = 0;
  
  if(oldmode != mode)
  {
    if(fx_counter == 1) // If Phaser 2 selected
    {
      switch(mode)
      {
      case 1:  
        mux(DEVICE_ADDR_7bit, Phaser2StageSelector, 1, 4); // 4 Stage Phaser 2
        break;
      case 2:
        mux(DEVICE_ADDR_7bit, Phaser2StageSelector, 2, 4); // 8 Stage Phaser 2
        break;
      case 3:
        mux(DEVICE_ADDR_7bit, Phaser2StageSelector, 3, 4); // 12 Stage Phaser 2
        break; 
      case 4:
        mux(DEVICE_ADDR_7bit, Phaser2StageSelector, 4, 4); // 16 Stage Phaser 2
        break;
      }
    }
    oldmode = mode;
  }
}

void setFXType(void)
{
  static uint8_t oldfx_counter = 0;
  
  if(fx_counter>1) // If Flanger Fx selected
    digitalWrite(LED_1, LOW); // Harmonic LED_1 On
  else
    digitalWrite(LED_1, HIGH); // Normal mode Harmonic LED_1 Off
   
  if(oldfx_counter != fx_counter)
  { 
    switch(fx_counter)
    {
      case 0:
        mux(DEVICE_ADDR_7bit, FXSelect, 1, 3); // Phaser 1 Tracking Filter
        break;
      case 1:
        mux(DEVICE_ADDR_7bit, FXSelect, 2, 3); // Phaser 2 Cascade All Pass
        break;
      case 2:
        mux(DEVICE_ADDR_7bit, FXSelect, 3, 3); // Flanger
        break;
    }
    oldfx_counter = fx_counter;
  } 
}

void depth(void)
{ 
  static float olddepthvalue = 0.00;
  
  if(olddepthvalue != depthvalue)
  {
    
    olddepthvalue = depthvalue;
  }
}

void print_menu_putty(void)
{
  // Print menu
  Serial.print(F("Effect status: "));
  if(bypass)
    Serial.println(F("bypass"));
  else
    Serial.println(F("on"));
  Serial.write('\n');  
  if(func_counter==0)
    Serial.print(F("    "));
  Serial.print(F("Freq. "));
  Serial.print(frequency, 1);
  Serial.println(F(" Hz"));
  if(func_counter==1)
    Serial.print(F("    "));
  Serial.print(F("Lfo type: "));
  if(lfotype==1)
    Serial.println(F("sine"));
  if(lfotype==2)
    Serial.println(F("triangular"));
  //Serial.println(lfotype, DEC);
  if(func_counter==2)
    Serial.print(F("    "));
  Serial.print(F("Mode: "));
  if(mode==1)
    Serial.println(F("Phaser1"));
  if(mode==2)
    Serial.println(F("Phaser2"));
  if(mode==3)
    Serial.println(F("Flanger"));
  //Serial.println(mode, DEC);
  /*if(func_counter==3)
    Serial.print(F("    "));
  Serial.print(F("Color: "));
  Serial.print(colorvalue, 1);
  Serial.println(F(" dB"));*/
  if(func_counter==4)
    Serial.print(F("    "));
  Serial.print(F("Volume: "));
  Serial.print(volumedB, 1);
  Serial.println(F(" dB"));
  if(func_counter==5)
    Serial.print(F("    "));
  Serial.print(F("Mix: "));
  Serial.print(mix, DEC);
  Serial.println(F(" %"));
  /*if(func_counter==6)
    Serial.print(F("    "));
  Serial.print(F("Depth: "));
  Serial.println(depthvalue, 2);*/
  
  Serial.write('\n');
  Serial.print(F("Active item: "));
  Serial.println(func_counter, DEC);
}

void print_menu_lcd(void)
{
  lcd.clear();
  lcd.setCursor(0, 0);
  if(bypass)
    lcd.print(F("BYPASS"));
  else
  {
    switch(fx_counter)
    {
      case 0:
        lcd.print(F("Phaser 1 TrkFlt"));
        break;
      case 1:
        lcd.print(F("Phaser 2 AllPFlt"));
        break;
      case 2:
        lcd.print(F("Flanger"));
        break;
        
    }
    lcd.setCursor(0, 1);
    switch(func_counter)
    {
      case 0:
        //lcd.print(F("Freq. "));
        //lcd.print(frequency, 1);
        //lcd.print(F(" Hz"));
        lcd.print(F("BPM: "));
        lcd.print(bpm, 2);
        break;
      case 1:
        lcd.print(F("Lfo type: "));
        if(lfotype==1)
          lcd.print(F("sine"));
        if(lfotype==2)
          lcd.print(F("triang"));
        break;
      case 2:
        if(fx_counter==1) // If Phaser 2
        {
          lcd.print(F("NStages: "));
          if(mode==1)
            lcd.print(F("4"));
          if(mode==2)
            lcd.print(F("8"));
          if(mode==3)
            lcd.print(F("12"));
          if(mode==4)
            lcd.print(F("16"));
        }
        else
        {
          lcd.print(F("No mode avail."));
        }
        break;
      case 3:
        lcd.print(F("Color: "));
        lcd.print(colorvalue, 1);
        lcd.print(F(" dB"));
        break;
      case 4:
        lcd.print(F("Volume: "));
        lcd.print(volumedB, 1);
        lcd.print(F(" dB"));
        break;
      case 5:
        lcd.print(F("Mix: "));
        lcd.print(mix, DEC);
        lcd.print(F(" %"));
        break;
      case 6:
        lcd.print(F("Depth: "));
        lcd.print(depthvalue, 2);
        break;
    }
  }
}

void setBypass(void)
{
  static uint8_t oldbypass = OFF;
  
  if(oldbypass != bypass)
  {
    MuteOn();
    if(bypass == ON)
    {
      mux(DEVICE_ADDR_7bit, Bypass, 1, 2); // Bypass
      digitalWrite(LED_2, HIGH);
    }
    else
    {
      mux(DEVICE_ADDR_7bit, Bypass, 2, 2); // FX
      digitalWrite(LED_2, LOW);
    }
    MuteOff();
    oldbypass = bypass;
  }
}

void setColor(float boost)
{
  static float oldboost = 0.00;
  
  if(boost != oldboost)
  {
    color.S = 0.70;
    color.f0 = 1200.00;
    color.boost = boost;
    color.type = HighShelf;
    EQ2ndOrd(DEVICE_ADDR_7bit, Color, &color);
    
    oldboost = boost;
  }
}

void setVolume(float boostdb)
{
  static float oldboostdb = 0.00;
  float boostlinear = 0.00;
  
  if(boostdb != oldboostdb)
  {
    
    boostlinear = pow(10, boostdb/20.0);
    MasterVolumeMono(DEVICE_ADDR_7bit, MasterVolume, boostlinear);
    
    oldboostdb = boostdb;
  }
}

void MuteOff(void)
{
  AIDA_WRITE_REGISTER_BLOCK(DEVICE_ADDR_7bit, CoreRegisterR4Addr, CoreRegisterR4Size, CoreRegisterR4Data);    // Mute DAC Off
}

void MuteOn(void)
{
  AIDA_WRITE_REGISTER_BLOCK( DEVICE_ADDR_7bit, CoreRegisterR0Addr, CoreRegisterR0Size, CoreRegisterR0Data ); // Mute DAC On
}

/*void push1_isr(void)
{
  
}*/

/*void push2_isr(void)
{

}*/

