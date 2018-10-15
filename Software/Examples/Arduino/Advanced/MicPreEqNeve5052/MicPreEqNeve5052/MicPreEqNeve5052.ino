/*
 AIDA MicPreEqNeve5052 Sketch
 	
 This sketch is a mic pre studio EQ inspired by Neve 5052. I've maintained original equalizer bands and range 
 and I added one clipping stage immediately after gain stage (where it should be). I've added to 
 the original design a THD injection section based on Chebyshev polynomials of 2nd and 3rd order. 
 I'm looking to find a list of presets to be used with various instruments. 
 The sketch is based on Template 2, then modified to fit my needs. 
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
 
 created December 2015
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
#define PRE_LPF_MAX 250.00
#define PRE_LPF_MIN 20.00
#define PRE_GAIN_MAX 66.00
#define PRE_GAIN_MIN 0.0
// Main Eq Frequencies
#define MAIN_EQ_LOW_MAX 220.00
#define MAIN_EQ_LOW_MIN 35.00
#define MAIN_EQ_MID_MAX 6000.00
#define MAIN_EQ_MID_MIN 200.00
#define MAIN_EQ_HI_MAX 16000.00
#define MAIN_EQ_HI_MIN 8000.00
// Main Eq Q
#define MAIN_EQ_Q_MAX 2.00
#define MAIN_EQ_Q_MIN 0.35

// Master Volume
#define MASTER_VOLUME_MAX 6.00
#define MASTER_VOLUME_MIN -80.00

#define MAX 20.0
#define MIN -20.0

#define ON 1
#define OFF 0

// FUNCTION PROTOTYPES
void spettacolino(void);
void clearAndHome(void);
void check_program(void);
void check_param(void);
void check_config(void);
void setMix(uint8_t percent);

// GLOBAL VARIABLES

// UI
uint8_t bypass = OFF;
uint8_t oldbypass = OFF;
uint8_t func_counter = 0;
uint8_t old_func_counter = 0;
uint8_t push_e_count = 0;
uint8_t push_e_function = 0;
uint8_t restore = 1;

int32_t param1_pulses = -30; // -25 dB
int32_t param2_pulses = 0;
int32_t param3_pulses = 26;
int32_t param4_pulses = 40;
int32_t param5_pulses = -28;
int32_t param6_pulses = 32;
int32_t param7_pulses = 44;
int32_t param8_pulses = -8;
int32_t param9_pulses = 109;
int32_t param10_pulses = 0;

uint32_t timec=0, prevtimec=0;

float param1_value = 0.00; // Mst Vol
float param2_value = 0.00; // THD
float param3_value = 0.00; // Low Boost
float param4_value = 0.00; // Low F
float param5_value = 0.00; // Mid Boost
float param6_value = 0.00; // Mid F
float param7_value = 0.00; // Hi Boost
float param8_value = 0.00; // Hi F
float param9_value = 0.00; // Soft Clip
float param10_value = 0.00; // Gain Pre

equalizer_t pre_eq;
equalizer_t low_eq;
equalizer_t mid_eq;
equalizer_t hi_eq;

char inByte = 0x00;

void setup()
{
  // put your setup code here, to run once:
  // I/O
  pinMode(PIN_LED, OUTPUT);
   
  // open the USBSerial port
  Serial.begin(115200);
  //clearAndHome();
  Serial.println(F("Aida DSP control with ARDUINO")); // Welcome message
  Serial.print(F("0x"));
  Serial.println((DEVICE_ADDR_7bit<<1)&~0x01, HEX);

  InitAida(); // Initialize DSP board
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
  param1_value = processencoder(MASTER_VOLUME_MIN, MASTER_VOLUME_MAX, param1_pulses); // Volume
  param2_value = processencoder(0, 100.0, param2_pulses); // THD
  param3_value = processencoder(-15.0, 15.0, param3_pulses); // Low Boost
  param4_value = processencoder(MAIN_EQ_LOW_MIN, MAIN_EQ_LOW_MAX, param4_pulses); // Low Freq
  param5_value = processencoder(-15.0, 15.0, param5_pulses); // Mid Boost
  param6_value = processencoder(MAIN_EQ_MID_MIN, MAIN_EQ_MID_MAX, param6_pulses); // Mid Freq
  param7_value = processencoder(-15.0, 15.0, param7_pulses); // Hi Boost
  param8_value = processencoder(MAIN_EQ_HI_MIN, MAIN_EQ_HI_MAX, param8_pulses); // Hi Freq
  param9_value = processencoder(0.1, 10.0, param9_pulses); // Soft Clip
  param10_value = processencoder(0.0, 24.0, param10_pulses); // Gain Pre
  
  // Bypass
  muxnoiseless(DEVICE_ADDR_7bit, BypassAddr, 1);
  
  // Pre Eq
  pre_eq.gain = 0.0; 
  pre_eq.f0 = 80.0;
  pre_eq.type = Highpass;
  pre_eq.onoff = ON;
  EQ1stOrd(DEVICE_ADDR_7bit, PreHPFAddr, &pre_eq);
  delayMicroseconds(100);
  
  // Main EQ
  // Low
  low_eq.type = LowShelf;
  low_eq.S = 1.0;
  low_eq.f0 = param4_value;
  low_eq.boost = param3_value;
  low_eq.onoff = true;
  low_eq.phase = true;
  EQ2ndOrd(DEVICE_ADDR_7bit, LowEqAddr, &low_eq);
  delayMicroseconds(100);
   
  // Mid
  mid_eq.type = Peaking;
  mid_eq.Q = 1.41;
  mid_eq.f0 = param6_value;
  mid_eq.boost = param5_value;
  mid_eq.onoff = true;
  mid_eq.phase = true;
  EQ2ndOrd(DEVICE_ADDR_7bit, MidEqAddr, &mid_eq);
  delayMicroseconds(100);
  
  // High
  hi_eq.type = HighShelf;
  hi_eq.S = 1.0;
  hi_eq.f0 = param8_value;
  hi_eq.boost = param7_value;
  hi_eq.onoff = true;
  hi_eq.phase = true;
  EQ2ndOrd(DEVICE_ADDR_7bit, HiEqAddr, &hi_eq);
  delayMicroseconds(100);
  
  setMix(param2_value); // Set Dry Signal Mix
  delayMicroseconds(100);
  
  gainCell(DEVICE_ADDR_7bit, MicGainAddr, pow(10, param10_value/20.0)); // Set Mic Pre Gain 
  delayMicroseconds(100);
  
  soft_clip(DEVICE_ADDR_7bit, SoftClipperAddr, param9_value); // Set Soft Clip 
  delayMicroseconds(100);
  
  MasterVolumeMono(DEVICE_ADDR_7bit, MasterVolAddr, pow(10, param1_value/20.0)); // Set Master Volume 
  delayMicroseconds(100);
  
  AIDA_WRITE_REGISTER_BLOCK(DEVICE_ADDR_7bit, CoreRegisterR4Addr, CoreRegisterR4Size, CoreRegisterR4Data);    // Mute DAC Off 
  delayMicroseconds(100);
}

void loop()
{
  // put your main code here, to run repeatedly:

  if(Serial.available()>0) // Serial parser to speed up Eq operations
  {
    inByte = Serial.read();
    switch(inByte)
    {
      case 'v':
      case 'V':
        func_counter = 0; // Mst Vol
        break;
      
      case 't':
      case 'T':
        func_counter = 1; // THD Mix
        break;
        
      case 'l':
      case 'L':
        func_counter = 2; // Low Eq
        break;
      
      case 'm':
      case 'M':
        func_counter = 4; // Mid Eq
        break;
        
      case 'h':
      case 'H':
        func_counter = 6; // Mid Eq
        break;
      
      case 's':
      case 'S':
        func_counter = 8; // Soft Clip (Warmth)
        break;
      
      case 'g':
      case 'G':
        func_counter = 9; // PRE GAIN
        break;
      
      default:
        // Do nothing
        break;
    }
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
    if(func_counter==10)
      func_counter=0;
  }
  else if(push_e_function==2)
  {
    bypass ^=1; 
  }

  timec = millis();
  if(timec-prevtimec >= 250)  // Here we manage control interface every 250ms
  { 
    clearAndHome();    // !!!Warning use with real terminal emulation program
    Serial.println(F("********************************"));
    Serial.println(F("*    User control interface    *"));
    Serial.println(F("*    AIDA Eq Neve 5052 Sketch  *"));
    Serial.println(F("********************************"));
    Serial.println(F(""));
    Serial.print(F(" Encoder pulses: "));
    Serial.println(getPulses(), DEC);
    Serial.println(F(""));
    
    if(oldbypass != bypass)
    {
      if(bypass == ON)
      {
        // Do something, call Aida DSP API
        muxnoiseless(DEVICE_ADDR_7bit, BypassAddr, 2);
      }
      else
      {
        // Do something, call Aida DSP API
        muxnoiseless(DEVICE_ADDR_7bit, BypassAddr, 1);
      }
      oldbypass = bypass;
    }

    if(old_func_counter != func_counter)
    {
      restore = 1;
      old_func_counter = func_counter;
    }
    switch(func_counter)
    {
    case 0: // PARAM1 Mst Vol
    
      if(restore)
      {
        restore = 0;
        setPulses(param1_pulses);
      }
      param1_pulses = getPulses();
      param1_value = processencoder(MASTER_VOLUME_MIN, MASTER_VOLUME_MAX, param1_pulses);
      MasterVolumeMono(DEVICE_ADDR_7bit, MasterVolAddr, pow(10, param1_value/20)); // Set Master Volume 
      delayMicroseconds(100);
      // Do something, call Aida DSP API
      break;
    case 1: // PARAM2 THD Mix
      if(restore)
      {
        restore = 0;
        setPulses(param2_pulses);
      }
      param2_pulses = getPulses();
      param2_value = processencoder(0, 100, param2_pulses);
      setMix(param2_value);
      delayMicroseconds(100);
      
      break;
    case 2: // PARAM3 Low Eq Boost
      if(restore)
      {
        restore = 0;
        setPulses(param3_pulses);
      }
      param3_pulses = getPulses();
      param3_value = processencoder(-15.0, 15.0, param3_pulses);
      low_eq.boost = param3_value;
      EQ2ndOrd(DEVICE_ADDR_7bit, LowEqAddr, &low_eq);
      delayMicroseconds(100);
      // Do something, call Aida DSP API
      break;
    case 3: // PARAM4 Low Eq Freq
      if(restore)
      {
        restore = 0;
        setPulses(param4_pulses);
      }
      param4_pulses = getPulses();
      param4_value = processencoder(MAIN_EQ_LOW_MIN, MAIN_EQ_LOW_MAX, param4_pulses);
      low_eq.f0 = param4_value;
      EQ2ndOrd(DEVICE_ADDR_7bit, LowEqAddr, &low_eq);
      delayMicroseconds(100);
      // Do something, call Aida DSP API
      break;
    case 4: // PARAM5
      if(restore)
      {
        restore = 0;
        setPulses(param5_pulses);
      }
      param5_pulses = getPulses();
      param5_value = processencoder(-15.0, 15.0, param5_pulses);
      mid_eq.boost = param5_value;
      EQ2ndOrd(DEVICE_ADDR_7bit, MidEqAddr, &mid_eq);
      delayMicroseconds(100);
      // Do something, call Aida DSP API
      break;
    case 5: // PARAM6
      if(restore)
      {
        restore = 0;
        setPulses(param6_pulses);
      }
      param6_pulses = getPulses();
      param6_value = processencoder(MAIN_EQ_MID_MIN, MAIN_EQ_MID_MAX, param6_pulses);
      mid_eq.f0 = param6_value;
      EQ2ndOrd(DEVICE_ADDR_7bit, MidEqAddr, &mid_eq);
      delayMicroseconds(100);
      // Do something, call Aida DSP API
      break;
    case 6: // PARAM7
      if(restore)
      {
        restore = 0;
        setPulses(param7_pulses);
      }
      param7_pulses = getPulses();
      param7_value = processencoder(-15.0, 15.0, param7_pulses);
      hi_eq.boost = param7_value;
      EQ2ndOrd(DEVICE_ADDR_7bit, HiEqAddr, &hi_eq);
      delayMicroseconds(100);
      // Do something, call Aida DSP API
      break; 
    case 7: // PARAM8
      if(restore)
      {
        restore = 0;
        setPulses(param8_pulses);
      }
      param8_pulses = getPulses();
      param8_value = processencoder(MAIN_EQ_HI_MIN, MAIN_EQ_HI_MAX, param8_pulses);
      hi_eq.f0 = param8_value;
      EQ2ndOrd(DEVICE_ADDR_7bit, HiEqAddr, &hi_eq);
      delayMicroseconds(100);
      // Do something, call Aida DSP API
      break;
    case 8: // PARAM9
      if(restore)
      {
        restore = 0;
        setPulses(param9_pulses);
      }
      param9_pulses = getPulses();
      param9_value = processencoder(0.1, 10.0, param9_pulses);
      soft_clip(DEVICE_ADDR_7bit, SoftClipperAddr, param9_value);
      delayMicroseconds(100);
      // Do something, call Aida DSP API
      break;
    case 9: // PARAM9 GAIN
      if(restore)
      {
        restore = 0;
        setPulses(param10_pulses);
      }
      param10_pulses = getPulses();
      param10_value = processencoder(0.0, 24.0, param10_pulses);
      gainCell(DEVICE_ADDR_7bit, MicGainAddr, pow(10, param10_value/20.0));
      delayMicroseconds(100);
      // Do something, call Aida DSP API
      break;
    } // End switch func_counter

    // Display information for user
    print_menu_putty();

    prevtimec = timec;
  } // End if 1000ms tick
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

void setMix(float percent)
{
  static float oldpercent = 0.1; // Different from 0% for initialization
  float value = 0.00;
  float restofthemix = 0.00;
  
  if(oldpercent != percent)
  {
    if(percent>100)
      percent = 100;
    value = percent/100.00;
  
    // MIX
    AIDA_SAFELOAD_WRITE_VALUE(DEVICE_ADDR_7bit, THDMixerAddr, false, 1.00-value);  // Dry	
    restofthemix = value;
    
    AIDA_SAFELOAD_WRITE_VALUE(DEVICE_ADDR_7bit, THDMixerAddr+1, true, restofthemix*1.0/5.0);   // Wet 2nd
    AIDA_SAFELOAD_WRITE_VALUE(DEVICE_ADDR_7bit, THDMixerAddr+2, true, restofthemix*4.0/5.0);   // Wet 3rd
  }
}

void print_menu_putty(void)
{
  // Print menu
  Serial.print(F(" Effect status: "));
  if(bypass)
    Serial.println(F("bypass"));
  else
    Serial.println(F("on"));
  Serial.println(F(""));  
  if(func_counter==0)
    Serial.print(F("    "));
  Serial.print(F(" MstVol: "));
  Serial.print(param1_value, 1);
  Serial.println(F("dB"));
  if(func_counter==1)
    Serial.print(F("    "));
  Serial.print(F(" THDMix: "));
  Serial.print(param2_value, 1);
  Serial.println(F("%"));
  if(func_counter==2)
    Serial.print(F("    "));
  Serial.print(F(" Low Eq Boost: "));
  Serial.print(param3_value, 1);
  Serial.println(F("dB"));
  if(func_counter==3)
    Serial.print(F("    "));
  Serial.print(F(" Low Eq Freq.: "));
  Serial.print(param4_value, 1);
  Serial.println(F("Hz"));
  if(func_counter==4)
    Serial.print(F("    "));
  Serial.print(F(" Mid Eq Boost: "));
  Serial.print(param5_value, 1);
  Serial.println(F("dB"));
  if(func_counter==5)
    Serial.print(F("    "));
  Serial.print(F(" Mid Eq Freq.: "));
  Serial.print(param6_value, 1);
  Serial.println(F("Hz"));
  if(func_counter==6)
    Serial.print(F("    "));
  Serial.print(F(" Hi Eq Boost: "));
  Serial.print(param7_value, 1);
  Serial.println(F("dB"));
  if(func_counter==7)
    Serial.print(F("    "));
  Serial.print(F(" Hi Eq Freq.: "));
  Serial.print(param8_value, 1);
  Serial.println(F("Hz"));
  if(func_counter==8)
    Serial.print(F("    "));
  Serial.print(F(" Tube Warmth: "));
  Serial.print(param9_value, 1);
  Serial.println(F("a"));
  if(func_counter==9)
    Serial.print(F("    "));
  Serial.print(F(" GAIN PRE: "));
  Serial.print(param10_value, 1);
  Serial.println(F("dB"));
  
  Serial.println(F(""));
  Serial.println(F(""));
  Serial.print(F(" Active item: "));
  Serial.println(func_counter, DEC);
}
