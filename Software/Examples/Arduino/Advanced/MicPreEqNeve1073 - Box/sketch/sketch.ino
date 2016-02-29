/*
 AIDA Neve 1073 Sketch
 	
 This sketch is an "Neve 1073" as described in...

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
 
 created February 2016
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
//#define PRE_GAIN_MAX 80.00
#define PRE_GAIN_MAX 24.00
#define PRE_GAIN_MIN 0.0
#define PRE_LPF_MAX 300.00
#define PRE_LPF_MIN 50.00
// Main Eq Frequencies
#define MAIN_EQ_LOW_MAX 220.00
#define MAIN_EQ_LOW_MIN 35.00
#define MAIN_EQ_MID_MAX 7200.00
#define MAIN_EQ_MID_MIN 360.00
#define MAIN_EQ_HI_MAX 12000.00
#define MAIN_EQ_HI_MIN 12000.00
// Main Eq Q
#define MAIN_EQ_Q_MAX 2.00
#define MAIN_EQ_Q_MIN 0.35

// Master Volume
#define MASTER_VOLUME_MAX 6.00
#define MASTER_VOLUME_MIN -80.00

#define POT_THR  4 // Threshold for filtering noise on pots (adcs)

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
void setPhase(uint8_t);
void setEqInOut(uint8_t);

void print_menu_putty(void);
void print_menu_lcd(void);

// GLOBAL VARIABLES
// ENCODER
int32_t OldPulses = 0;

// UI
uint8_t func_counter = 0;
uint8_t old_func_counter = 0;
uint8_t bypass = OFF;
uint8_t oldbypass = OFF;

uint32_t timec=0, prevtimec=0;

// Values in param pulses are startup values for
// DSP Blocks
int32_t param1_pulses = 0; // Gain Pre
int32_t param2_pulses = 0;
int32_t param3_pulses = 106;  // Low Eq Freq 220
int32_t param4_pulses = 0;
int32_t param5_pulses = 16;   // Mid Eq Freq 1500
int32_t param6_pulses = 0;
int32_t param7_pulses = 0;    // Hi Eq Freq 12000
int32_t param8_pulses = 0;
int32_t param9_pulses = 119; // alpha = 10
int32_t param10_pulses = 119; // alpha = 10
int32_t param11_pulses = 0;
int32_t param12_pulses = 27; // Pre HPF Freq 120 
int32_t param13_pulses = 0;
int32_t param14_pulses = 10; // Mid Eq Q 0.5
int32_t param15_pulses = 52; // Eq In Out : In 

uint8_t restore = 1;  // If 1 startup values are written to DSP

float param1_value = 0.00; 
float param2_value = 0.00; 
float param3_value = 0.00; 
float param4_value = 0.00; 
float param5_value = 0.00; 
float param6_value = 0.00; 
float param7_value = 0.00; 
uint8_t param8_value = 0; 
float param9_value = 0.00; 
float param10_value = 0.00; 
uint8_t param11_value = 0;
float param12_value = 0.00;
float param13_value = 0.00;
float param14_value = 0.00; 
uint8_t param15_value = 0;

equalizer_t pre_eq;
equalizer_t low_eq;
equalizer_t mid_eq;
equalizer_t hi_eq;

uint16_t pot1 = 0;
uint16_t oldpot1 = 0;
uint16_t pot2 = 0;
uint16_t oldpot2 = 0;
uint16_t pot3 = 0;
uint16_t oldpot3 = 0;
uint16_t pot4 = 0;
uint16_t oldpot4 = 0;

// Pot Filter 1
uint16_t adcvalue1 = 0;
uint32_t sum1 = 0;
uint32_t out1 = 0;
// Pot Filter 2
uint16_t adcvalue2 = 0;
uint32_t sum2 = 0;
uint32_t out2 = 0;
// Pop Filter 3
uint16_t adcvalue3 = 0;
uint32_t sum3 = 0;
uint32_t out3 = 0;
// Pop Filter 4
uint16_t adcvalue4 = 0;
uint32_t sum4 = 0;
uint32_t out4 = 0;

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
  lcd.print(F("Neve 1073 V0.1"));

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
   
  // -- Initialization 
  
  // Param Values
  param1_value = processencoder(PRE_GAIN_MIN, PRE_GAIN_MAX, param1_pulses); // Pre Gain
  param2_value = processencoder(-16.0, 16.0, param2_pulses); // Low Boost
  param3_value = processencoder(MAIN_EQ_LOW_MIN, MAIN_EQ_LOW_MAX, param3_pulses); // Low Freq
  param4_value = processencoder(-18.0, 18.0, param4_pulses); // Mid Boost
  param5_value = processencoder(MAIN_EQ_MID_MIN, MAIN_EQ_MID_MAX, param5_pulses); // Mid Freq
  param6_value = processencoder(-16.0, 16.0, param6_pulses); // Hi Boost
  param7_value = processencoder(MAIN_EQ_HI_MIN, MAIN_EQ_HI_MAX, param7_pulses); // Hi Freq
  param8_value = selectorwithencoder(param8_pulses, 1); // Phase On Off
  param9_value = processencoder(0.1, 10, param9_pulses); // Soft Clip 1
  param10_value = processencoder(0.1, 10, param10_pulses); // Soft Clip 2
  param11_value = selectorwithencoder(param11_pulses, 1); // Pre HPF On Off
  param12_value = processencoder(PRE_LPF_MIN, PRE_LPF_MAX, param12_pulses); // Pre HPF Freq
  param13_value = processencoder(MASTER_VOLUME_MIN, MASTER_VOLUME_MAX, param13_pulses); // Gain Pre
  param14_value = processencoder(MAIN_EQ_Q_MIN, MAIN_EQ_Q_MAX, param14_pulses); // Mid Eq Q
  param15_value = selectorwithencoder(param15_pulses, 1); // Eq In Out (On Off)
  
  // Pre Gain
  gainCell(DEVICE_ADDR_7bit, PreGainAddr, pow(10, param1_value/20.0)); // Set Mic Pre Gain 
  delayMicroseconds(100);
  
  // Main EQ
  // Low
  low_eq.type = LowShelf;
  low_eq.S = 1.0;
  low_eq.f0 = param3_value;
  low_eq.boost = param2_value;
  low_eq.onoff = true;
  low_eq.phase = false;
  EQ2ndOrd(DEVICE_ADDR_7bit, LowEqAddr, &low_eq); 
  delayMicroseconds(100); 
  
  // Mid
  mid_eq.type = Peaking;
  mid_eq.Q = param14_value;
  mid_eq.f0 = param5_value;
  mid_eq.boost = param4_value;
  mid_eq.onoff = true;
  mid_eq.phase = false;
  EQ2ndOrd(DEVICE_ADDR_7bit, MidEqAddr, &mid_eq);
  delayMicroseconds(100);
  
  // High
  hi_eq.type = HighShelf;
  hi_eq.S = 1.0;
  hi_eq.f0 = param7_value;
  hi_eq.boost = param6_value;
  hi_eq.onoff = true;
  hi_eq.phase = false;
  EQ2ndOrd(DEVICE_ADDR_7bit, HiEqAddr, &hi_eq);
  delayMicroseconds(100);
  
  setEqInOut(param15_value);
  delayMicroseconds(100);
  
  setPhase(param8_value);
  delayMicroseconds(100);
  
  soft_clip(DEVICE_ADDR_7bit, SoftClipper1Addr, param9_value); // Set Soft Clip 1
  delayMicroseconds(100);
  
  soft_clip(DEVICE_ADDR_7bit, SoftClipper2Addr, param10_value); // Set Soft Clip 2
  delayMicroseconds(100);
  
  // Pre Eq
  pre_eq.gain = 0.0; 
  pre_eq.f0 = param12_value;
  pre_eq.type = Highpass;
  pre_eq.phase = false;
  if(param11_value==1)
    pre_eq.onoff = OFF;
  else if(param11_value==2)
    pre_eq.onoff = ON;
  EQ1stOrd(DEVICE_ADDR_7bit, PreHPFAddr, &pre_eq);
  delayMicroseconds(100);
  
  MasterVolumeMono(DEVICE_ADDR_7bit, MasterVolAddr, pow(10, param13_value/20.0)); // Set Master Volume 
  delayMicroseconds(100); 
   
  MuteOff();  // Mute DAC Off
  
  // Bypass status init = disable
  bypass = 0;
  muxnoiseless(DEVICE_ADDR_7bit, BypassAddr, 1); // FX
  digitalWrite(LED_2, LOW); // Led 2 On
}

void loop()
{
  // put your main code here, to run repeatedly:

  adcvalue1 = analogRead(POT1);
  sum1 = ((((64)-1) * sum1)+((uint32_t)adcvalue1*(64)))/(64);
  out1 = sum1/64;
  pot1 = out1;
  if(!isinrange(pot1, oldpot1, POT_THR))
  {
    func_counter=0;
    param1_value = processpot(PRE_GAIN_MIN, PRE_GAIN_MAX, pot1); // Pre Gain
    gainCell(DEVICE_ADDR_7bit, PreGainAddr, pow(10, param1_value/20.0)); // Set Mic Pre Gain 
    oldpot1 = pot1;
  }
  
  adcvalue2 = analogRead(POT2);
  sum2 = ((((64)-1) * sum2)+((uint32_t)adcvalue2*(64)))/(64);
  out2 = sum2/64;
  pot2 = out2;
  if(!isinrange(pot2, oldpot2, POT_THR))
  {
    func_counter=1;
    param2_value = processpot(-16.0, 16.0, pot2); // Low Boost
    low_eq.boost = param2_value;
    EQ2ndOrd(DEVICE_ADDR_7bit, LowEqAddr, &low_eq); 
    oldpot2 = pot2;
  }
  
  adcvalue3 = analogRead(POT3);
  sum3 = ((((64)-1) * sum3)+((uint32_t)adcvalue3*(64)))/(64);
  out3 = sum3/64;
  pot3 = out3;
  if(!isinrange(pot3, oldpot3, POT_THR))
  {
    func_counter=3;
    //func_counter=4;
    param4_value = processpot(-16.0, 16.0, pot3); // Low Boost
    //param5_value = processpot(MAIN_EQ_MID_MIN, MAIN_EQ_MID_MAX, pot3); // Mid Freq
    mid_eq.boost = param4_value;
    //mid_eq.f0 = param5_value; 
    EQ2ndOrd(DEVICE_ADDR_7bit, MidEqAddr, &mid_eq);
    oldpot3 = pot3;
  }
  
  adcvalue4 = analogRead(POT4);
  sum4 = ((((64)-1) * sum4)+((uint32_t)adcvalue4*(64)))/(64);
  out4 = sum4/64;
  pot4 = out4;
  if(!isinrange(pot4, oldpot4, POT_THR))
  {
    func_counter=5;
    param6_value = processpot(-16.0, 16.0, pot4); // Low Boost
    hi_eq.boost = param6_value;
    EQ2ndOrd(DEVICE_ADDR_7bit, HiEqAddr, &hi_eq); 
    oldpot4 = pot4;
  }

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
    if(func_counter==15)
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
        // PUSH_1 PRESSED
        if(param15_value==2) // Toggle Eq In Out status variable
          param15_value = 1;
        else if(param15_value==1)
          param15_value = 2;
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
    Serial.println(F("*    AIDA Neve 1073 Sketch     *"));
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
    
    setEqInOut(param15_value); // Using PUSH_1 and LED_1
    setBypass(); // Using PUSH_2 and LED_2
    
    if(old_func_counter != func_counter)
    {
      restore = 1;
      old_func_counter = func_counter;
    }
    switch(func_counter)
    {
    case 0: // Pre Gain
      /*if(restore)
      {
        restore = 0;
        setPulses(param1_pulses);
      }
      param1_pulses = getPulses();
      param1_value = processencoder(PRE_GAIN_MIN, PRE_GAIN_MAX, param1_pulses); // Pre Gain
      gainCell(DEVICE_ADDR_7bit, PreGainAddr, pow(10, param1_value/20.0)); // Set Mic Pre Gain 
      delayMicroseconds(100);*/
      break;
    case 1: // Bass Boost
      /*if(restore)
      {
        restore = 0;
        setPulses(param2_pulses);
      }
      param2_pulses = getPulses();
      param2_value = processencoder(-16.0, 16.0, param2_pulses); // Low Boost
      low_eq.boost = param2_value;
      EQ2ndOrd(DEVICE_ADDR_7bit, LowEqAddr, &low_eq); 
      delayMicroseconds(100);*/
      break;
    case 2: // Bass Frequency
      if(restore)
      {
        restore = 0;
        setPulses(param3_pulses);
      }
      param3_pulses = getPulses();
      param3_value = processencoder(MAIN_EQ_LOW_MIN, MAIN_EQ_LOW_MAX, param3_pulses); // Low Freq
      low_eq.f0 = param3_value;
      EQ2ndOrd(DEVICE_ADDR_7bit, LowEqAddr, &low_eq); 
      delayMicroseconds(100);
      break;
    case 3: // Middle Boost 
      /*if(restore)
      {
        restore = 0;
        setPulses(param4_pulses);
      }
      param4_pulses = getPulses();
      param4_value = processencoder(-18.0, 18.0, param4_pulses); // Mid Boost
      mid_eq.boost = param4_value;
      EQ2ndOrd(DEVICE_ADDR_7bit, MidEqAddr, &mid_eq);
      delayMicroseconds(100);*/
      break;
    case 4: // Middle Frequency
      if(restore)
      {
        restore = 0;
        setPulses(param5_pulses);
      }
      param5_pulses = getPulses();
      param5_value = processencoder(MAIN_EQ_MID_MIN, MAIN_EQ_MID_MAX, param5_pulses); // Mid Freq
      mid_eq.f0 = param5_value;
      EQ2ndOrd(DEVICE_ADDR_7bit, MidEqAddr, &mid_eq);
      delayMicroseconds(100);
      break;  
    case 5: // High Boost
      /*if(restore)
      {
        restore = 0;
        setPulses(param6_pulses);
      }
      param6_pulses = getPulses();
      param6_value = processencoder(-16.0, 16.0, param6_pulses); // Hi Boost
      hi_eq.boost = param6_value;
      EQ2ndOrd(DEVICE_ADDR_7bit, HiEqAddr, &hi_eq);
      delayMicroseconds(100);*/
      break;
    case 6: // High Frequency
      if(restore)
      {
        restore = 0;
        setPulses(param7_pulses);
      }
      param7_pulses = getPulses();
      param7_value = processencoder(MAIN_EQ_HI_MIN, MAIN_EQ_HI_MAX, param7_pulses); // Hi Freq
      hi_eq.f0 = param7_value;
      EQ2ndOrd(DEVICE_ADDR_7bit, HiEqAddr, &hi_eq);
      delayMicroseconds(100);
      break;  
    case 7: // Phase On Off
      if(restore)
      {
        restore = 0;
        setPulses(param8_pulses);
      }
      param8_pulses = getPulses();
      param8_value = selectorwithencoder(param8_pulses, 1); // Phase On Off
      setPhase(param8_value);
      delayMicroseconds(100);
      break;  
    case 8: // Soft Clip 1
      if(restore)
      {
        restore = 0;
        setPulses(param9_pulses);
      }
      param9_pulses = getPulses();
      param9_value = processencoder(0.1, 10, param9_pulses); // Soft Clip 1
      soft_clip(DEVICE_ADDR_7bit, SoftClipper1Addr, param9_value); // Set Soft Clip 1
      delayMicroseconds(100);
      break;
    case 9: // Soft Clip 2
      if(restore)
      {
        restore = 0;
        setPulses(param10_pulses);
      }
      param10_pulses = getPulses();
      param10_value = processencoder(0.1, 10, param10_pulses); // Soft Clip 2
      soft_clip(DEVICE_ADDR_7bit, SoftClipper2Addr, param10_value); // Set Soft Clip 2
      delayMicroseconds(100);
      break;  
    case 10: // Pre HPF On Off
      if(restore)
      {
        restore = 0;
        setPulses(param11_pulses);
      }
      param11_pulses = getPulses();
      param11_value = selectorwithencoder(param11_pulses, 1); // Pre HPF On Off
      if(param11_value==1)
        pre_eq.onoff = OFF;
      else if(param11_value==2)
        pre_eq.onoff = ON;
      EQ1stOrd(DEVICE_ADDR_7bit, PreHPFAddr, &pre_eq);
      delayMicroseconds(100); 
      break;
    case 11: // Pre HPF Frequency
      if(restore)
      {
        restore = 0;
        setPulses(param12_pulses);
      }
      param12_pulses = getPulses();
      param12_value = processencoder(PRE_LPF_MIN, PRE_LPF_MAX, param12_pulses);
      pre_eq.f0 = param12_value;
      EQ1stOrd(DEVICE_ADDR_7bit, PreHPFAddr, &pre_eq);
      delayMicroseconds(100);
      break;
    case 12: // Master Volume
      if(restore)
      {
        restore = 0;
        setPulses(param13_pulses);
      }
      param13_pulses = getPulses();
      param13_value = processencoder(MASTER_VOLUME_MIN, MASTER_VOLUME_MAX, param13_pulses);
      MasterVolumeMono(DEVICE_ADDR_7bit, MasterVolAddr, pow(10, param13_value/20.0)); // Set Master Volume 
      delayMicroseconds(100);
      break;
    case 13: // Mid Eq Q
      if(restore)
      {
        restore = 0;
        setPulses(param14_pulses);
      }
      param14_pulses = getPulses();
      param14_value = processencoder(MAIN_EQ_Q_MIN, MAIN_EQ_Q_MAX, param14_pulses); // Mid Eq Q
      mid_eq.Q = param14_value;
      EQ2ndOrd(DEVICE_ADDR_7bit, MidEqAddr, &mid_eq);
      delayMicroseconds(100);
      break;
    case 14:  
      if(restore)
      {
        restore = 0;
        setPulses(param15_pulses);
      }
      param15_pulses = getPulses();
      param15_value = selectorwithencoder(param15_pulses, 1); // Eq In Out
      setEqInOut(param15_value);
      delayMicroseconds(100);
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
  Serial.print(F("Gain: "));
  Serial.print(param1_value, 1);
  Serial.println(F(" dB"));
  if(func_counter==1)
    Serial.print(F("    "));
  Serial.print(F("Low Eq Boost: "));
  Serial.print(param2_value, 1);
  Serial.println(F(" dB"));
  if(func_counter==2)
    Serial.print(F("    "));
  Serial.print(F("Low Eq Freq: "));
  Serial.print(param3_value, 1);
  Serial.println(F(" Hz"));
  if(func_counter==3)
    Serial.print(F("    "));
  Serial.print(F("Mid Eq Boost: "));
  Serial.print(param4_value, 1);
  Serial.println(F(" dB"));
  if(func_counter==4)
    Serial.print(F("    "));
  Serial.print(F("Mid Eq Freq: "));
  Serial.print(param5_value, 1);
  Serial.println(F(" Hz"));
  if(func_counter==5)
    Serial.print(F("    "));
  Serial.print(F("Hi Eq Boost: "));
  Serial.print(param6_value, 1);
  Serial.println(F(" dB"));
  if(func_counter==6)
    Serial.print(F("    "));
  Serial.print(F("Hi Eq Freq: "));
  Serial.print(param7_value, 1);
  Serial.println(F(" Hz"));
  if(func_counter==7)
    Serial.print(F("    "));
  Serial.print(F("Phase: "));
  if(param8_value==1)
    Serial.println(F("Off"));
  else if(param8_value==2)
    Serial.println(F("On"));
  if(func_counter==8)
    Serial.print(F("    "));
  Serial.print(F("Tube Warmth 1: "));
  Serial.print(param9_value, 1);
  Serial.println(F("a"));
  if(func_counter==9)
    Serial.print(F("    "));
  Serial.print(F("Tube Warmth 2: "));
  Serial.print(param10_value, 1);
  Serial.println(F("a"));
  if(func_counter==10)
    Serial.print(F("    "));
  Serial.print(F("Pre HPF: "));
  if(param11_value==1)
    Serial.println(F("Off"));
  else if(param11_value==2)
    Serial.println(F("On"));
  if(func_counter==11)
    Serial.print(F("    "));
  Serial.print(F("Pre HPF Freq: "));
  Serial.print(param12_value, 1);
  Serial.println(F(" Hz"));
  if(func_counter==12)
    Serial.print(F("    "));
  Serial.print(F("Mst Vol: "));
  Serial.print(param13_value, 1);
  Serial.println(F(" dB"));
  if(func_counter==13)
    Serial.print(F("    "));
  Serial.print(F("Mid Eq Q: "));
  Serial.println(param14_value, 1);
  if(func_counter==14)
    Serial.print(F("    "));
  Serial.print(F("EqInOut: "));
  if(param15_value==1)
    Serial.println(F("Out"));
  else if(param15_value==2)
    Serial.println(F("In"));
  
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
    lcd.print(F("Neve 1073")); 
    lcd.setCursor(0, 1);
    switch(func_counter)
    {
      case 0:
        lcd.print(F("Gain: "));
        lcd.print(param1_value, 2);
        lcd.print(F(" dB"));
        break;
      case 1:
        lcd.print(F("Low: "));
        lcd.print(param2_value, 2);
        lcd.print(F(" dB"));
        break;
      case 2:
        lcd.print(F("LowF: "));
        lcd.print(param3_value, 2);
        lcd.print(F(" Hz"));
        break;
      case 3:
        lcd.print(F("Mid: "));
        lcd.print(param4_value, 2);
        lcd.print(F(" dB"));
        break;
      case 4:
        lcd.print(F("MidF: "));
        lcd.print(param5_value, 2);
        lcd.print(F(" Hz"));
        break;
      case 5:
        lcd.print(F("Hi: "));
        lcd.print(param6_value, 2);
        lcd.print(F(" dB"));
        break;
      case 6:
        lcd.print(F("HiF: "));
        lcd.print(param7_value, 2);
        lcd.print(F(" Hz"));
        break;
      case 7:
        lcd.print(F("Phase: "));
        if(param8_value==1)
          lcd.print(F("Off"));
        else if(param8_value==2)
          lcd.print(F("On"));
        break;
      case 8:
        lcd.print(F("Tube1: "));
        lcd.print(param9_value, 2);
        lcd.print(F(" a"));
        break;
      case 9:
        lcd.print(F("Tube2: "));
        lcd.print(param10_value, 2);
        lcd.print(F(" a"));
        break;
      case 10:
        lcd.print(F("Pre HPF: "));
        if(param11_value==1)
          lcd.print(F("Off"));
        else if(param11_value==2)
          lcd.print(F("On"));
        break;
      case 11:
        lcd.print(F("HPF F: "));
        lcd.print(param12_value, 2);
        lcd.print(F(" Hz"));
        break;
      case 12:
        lcd.print(F("Mst Vol: "));
        lcd.print(param13_value, 2);
        lcd.print(F(" dB"));
        break;
      case 13:
        lcd.print(F("Mid Q: "));
        lcd.print(param14_value, 2);
        break;
      case 14:
        lcd.print(F("EqInOut: "));
        if(param15_value==1)
          lcd.print(F("Out"));
        else if(param15_value==2)
          lcd.print(F("In"));
        break;
    }
  }
}

void setBypass(void)
{
  static uint8_t oldbypass = OFF;
  
  if(oldbypass != bypass)
  {
    if(bypass == ON)
    {
      muxnoiseless(DEVICE_ADDR_7bit, BypassAddr, 2); // Bypass
      digitalWrite(LED_2, HIGH);
    }
    else
    {
      muxnoiseless(DEVICE_ADDR_7bit, BypassAddr, 1); // FX
      digitalWrite(LED_2, LOW);
    }
    oldbypass = bypass;
  }
}

void setPhase(uint8_t state)
{
  static uint8_t oldstate = 1;
  
  if(oldstate != state)
  {
    if(state==1)
    {
      AIDA_WRITE_VALUE(DEVICE_ADDR_7bit, PhaseAddr, 1.0); // 0
    }
    else if(state==2)
    {
      AIDA_WRITE_VALUE(DEVICE_ADDR_7bit, PhaseAddr, -1.0); // 180
    }
    oldstate = state;
  }
}

void setEqInOut(uint8_t state)
{
  static uint8_t oldstate = OFF;
  
  if(oldstate != state)
  {
    if(state == 2)
    {
      muxnoiseless(DEVICE_ADDR_7bit, EqInOutAddr, 2); // Eq In
      digitalWrite(LED_1, LOW);
    }
    else
    {
      muxnoiseless(DEVICE_ADDR_7bit, EqInOutAddr, 1); // Eq Out
      digitalWrite(LED_1, HIGH);
    }
    oldstate = state;
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

