/*
 AIDA Tubescreamer Sketch
 	
 This sketch is a "Tubescreamer" inspired the work of David Yeh in his PhD Thesis. All the rights
 about the mathematical analysis and theory go to David, which I sincerely thank for publishing
 such an amazing work. Well, physical simulation is the job of taking a well sounding circuit
 and approximate it in digital domain. It's amazing how much I've learned from this project,
 both in skills and musical taste. Since the original tubescreamer had silicon diodes (Si), I've included a 
 germanium diode (Ge) mathematical model also, to increase tone versatility. 
 See Doc inside this folder, along with Matlab script which calculates LUT for non-linear stage. 
 The official discussion about this effect on Engineerzone https://ez.analog.com/thread/82012

 This sketch was written for the Arduino, and will not work on other boards.
 	
 The circuit:
 
 Audio:
 * Input range 2.0Vrms (5.66Vpp, 8.23dBu)
 * Output range 0.9Vrms (2.5Vpp, 1.30dBu) 
 
 PC:
 * Please connect with PuTTY on Stellaris USB Serial with a PC for a minimal user interface
 
 NOTE:
 - Despite I prefer to visualize Hz, dB etc. many commercial pedals show only 0-100% settings. This
 is the reason why you'll find paramN_fake
 - Original Tubescreamer had these pot controls
   . Potentiometer 500K/470K Lin (Drive)
   . Potentiometer 20K/22K Lin (Tone)
   . Potentiometer 100K Log (Volume)
 - V0.3 introduced "blend" (mix) control between Silicon and Germanium LUTs, highpass filter is fixed frequency 720Hz and Germanium LUT is rms 
 compensated to sound loud as Silicon LUT  
 - V0.2 improved dynamics (LUTs are now calculated for +/-5V input signal) and highpass filter moves its frequency depending on drive setting
 - V0.1 first version
 
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
#define DRIVE_MAX 100.0f
#define DRIVE_MIN 0.0f
#define TONE_MAX 4180.0f // Hz
#define TONE_MIN 792.0f  // Hz
#define MIX_MAX 100.0
#define MIX_MIN 0.0

// Master Volume
#define MASTER_VOLUME_MAX 0.00
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

#define STOMPBOX // Comment to use on Aida DSP "La Prima"
//#define READBACK // Comment to do not perform readback of input guitar signal level inside DSP

// FUNCTION PROTOTYPES
void spettacolino();
void clearAndHome(void);
void setBypass(uint8_t);
void setDrive(float);
void setMix(float);
void toggleTech(uint8_t);

void print_menu_putty(void);
void print_menu_lcd(void);

// GLOBAL VARIABLES
// ENCODER
int32_t OldPulses = 0;

// UI
uint8_t func_counter = 0;
uint8_t old_func_counter = 0;
uint8_t bypass = OFF;
uint8_t toggletech = OFF;

uint32_t timec=0, prevtimec=0;

// Values in param pulses are startup values for
// DSP Blocks
int32_t param1_pulses = 0; // Drive
int32_t param2_pulses = 0; // Tone
int32_t param3_pulses = -8; // Master Volume -> -6dB
int32_t param4_pulses = 0; // Technology -> Si

uint8_t restore = 1;  // If 1 startup values are written to DSP

float param1_value = 0.00;
float param2_value = 0.00; 
float param2_fake = 0.00;
float param3_value = 0.00; 
float param3_fake = 0.00;
float param4_value = 0;

equalizer_t tone_eq;
equalizer_t opamp_eq;
equalizer_t antialias_eq; 
equalizer_t postdist_eq;

float readback1 = 0.00;
float maxreadback1 = 0.00;

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
  #ifdef STOMPBOX
  pinMode(LED_1, OUTPUT);
  digitalWrite(LED_1, HIGH);
  pinMode(LED_2, OUTPUT);
  digitalWrite(LED_2, HIGH);
  pinMode(PUSH_1, INPUT_PULLUP);
  //attachInterrupt(5, push1_isr, FALLING); 
  pinMode(PUSH_2, INPUT_PULLUP);
  //attachInterrupt(4, push2_isr, FALLING); 
  #endif

  // open the USBSerial port
  Serial.begin(115200);
  clearAndHome();
  Serial.println(F("Aida DSP control with ARDUINO")); // Welcome message
  Serial.print(F("0x"));
  Serial.println((DEVICE_ADDR_7bit<<1)&~0x01, HEX);
  
  // LCD Display
  #ifdef STOMPBOX
  lcd.begin(16, 2); // set up the LCD's number of columns and rows
  lcd.setCursor(0, 0);
  lcd.print(F("Aida DSP Box")); // Print a message to the LCD.
  lcd.setCursor(0, 1);
  lcd.print(F("Tubescr. V0.3"));
  #endif

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
  param1_value = processencoder(DRIVE_MIN, DRIVE_MAX, param1_pulses); // Drive
  param2_value = processencoder(TONE_MIN, TONE_MAX, param2_pulses); // Tone
  param3_value = processencoder(MASTER_VOLUME_MIN, MASTER_VOLUME_MAX, param3_pulses); // Master Volume
  param4_value = processencoder(MIX_MIN, MIX_MAX, param4_pulses); // Technology Mix
  
  // Pre Gain
  gainCell(DEVICE_ADDR_7bit, PreGainAddr, 2.83);
  delayMicroseconds(100);
  
  // Opamp Highpass Filter
  opamp_eq.gain = 1.0; 
  opamp_eq.f0 = 720.0; // Hz
  opamp_eq.type = Highpass;
  opamp_eq.phase = false;
  opamp_eq.onoff = ON;
  EQ1stOrd(DEVICE_ADDR_7bit, OpampAddr, &opamp_eq);
  delayMicroseconds(100);
  
  hard_clip(DEVICE_ADDR_7bit, PreGainLimitAddr, 5.0, -5.0);
  delayMicroseconds(100);
  
  // Drive
  setDrive(param1_value);
  delayMicroseconds(100);
  
  // Anti-aliasing filter (lowpass before distortion)
  antialias_eq.gain = 0.0; 
  antialias_eq.f0 = 6000; // 8x oversampling @ 96k
  //antialias_eq.f0 = 12000.0; // 4x oversampling @ 96k
  antialias_eq.type = Lowpass;
  antialias_eq.phase = false;
  antialias_eq.onoff = ON;
  EQ1stOrd(DEVICE_ADDR_7bit, AntiAliasingFAddr, &antialias_eq);
  delayMicroseconds(100);
  
  hard_clip(DEVICE_ADDR_7bit, PostGainLimitAddr, 5.0, -5.0);
  delayMicroseconds(100);
  
  setMix(param4_value);
  delayMicroseconds(100);
  
  // Post-distortion lowpass filter
  postdist_eq.gain = 0.0; 
  postdist_eq.f0 = 24000.0;
  postdist_eq.type = Lowpass;
  postdist_eq.phase = false;
  postdist_eq.onoff = OFF;
  EQ1stOrd(DEVICE_ADDR_7bit, PostFAddr, &postdist_eq);
  delayMicroseconds(100);
  
  // Tone
  tone_eq.gain = 0.0; 
  tone_eq.f0 = param2_value;
  tone_eq.type = Lowpass;
  tone_eq.phase = false;
  tone_eq.onoff = ON;
  EQ1stOrd(DEVICE_ADDR_7bit, ToneAddr, &tone_eq);
  delayMicroseconds(100);
  
  // Master Volume
  MasterVolumeMono(DEVICE_ADDR_7bit, MasterVolumeAddr, pow(10, param3_value/20.0)); // Set Master Volume 
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

  #ifdef STOMPBOX
  adcvalue1 = analogRead(POT1);
  sum1 = ((((64)-1) * sum1)+((uint32_t)adcvalue1*(64)))/(64);
  out1 = sum1/64;
  pot1 = out1;
  if(!isinrange(pot1, oldpot1, POT_THR))
  {
    func_counter=0;
    param1_value = processpot(DRIVE_MIN, DRIVE_MAX, pot1); // Drive
    setDrive(param1_value);
    delayMicroseconds(25);
    oldpot1 = pot1;
  }
  
  adcvalue2 = analogRead(POT2);
  sum2 = ((((64)-1) * sum2)+((uint32_t)adcvalue2*(64)))/(64);
  out2 = sum2/64;
  pot2 = out2;
  if(!isinrange(pot2, oldpot2, POT_THR))
  {
    func_counter=1;
    param2_value = processpot(TONE_MIN, TONE_MAX, pot2); // Tone
    param2_fake = processpot(0.0, 100.0, pot2); // Only for vizualization
    tone_eq.f0 = param2_value;
    EQ1stOrd(DEVICE_ADDR_7bit, ToneAddr, &tone_eq);
    oldpot2 = pot2;
  }
  
  adcvalue3 = analogRead(POT3);
  sum3 = ((((64)-1) * sum3)+((uint32_t)adcvalue3*(64)))/(64);
  out3 = sum3/64;
  pot3 = out3;
  if(!isinrange(pot3, oldpot3, POT_THR))
  {
    func_counter=2;
    param3_value = processpot(MASTER_VOLUME_MIN, MASTER_VOLUME_MAX, pot3); // Master Volume
    param3_fake = processpot(0.0, 100.0, pot3); // Only for visualization
    MasterVolumeMono(DEVICE_ADDR_7bit, MasterVolumeAddr, pow(10, param3_value/20.0)); // Set Master Volume
    oldpot3 = pot3;
  }
  
  adcvalue4 = analogRead(POT4);
  sum4 = ((((64)-1) * sum4)+((uint32_t)adcvalue4*(64)))/(64);
  out4 = sum4/64;
  pot4 = out4;
  if(!isinrange(pot4, oldpot4, POT_THR))
  {
    func_counter=3;
    param4_value = processpot(MIX_MIN, MIX_MAX, pot4); // Technology Mix
    setMix(param4_value);
    
    oldpot4 = pot4;
  }
  #endif

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
    if(func_counter==4)
      func_counter=0;
  }
  else if(push_e_function==2)
  {
    #ifndef STOMPBOX
    bypass ^= 1; // Use 2nd function as bypass switch in normal mode
    #endif
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
        toggletech ^= 1;
        #ifdef READBACK
          maxreadback1 = 0.00;
        #endif
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
        // PUSH_2 PRESSED
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
    #ifdef STOMPBOX
    reinitdisplaycounter++;
    if(reinitdisplaycounter==16) // Sometimes display takes noise and corrupts its RAM...
    {
      lcd.begin(16, 2); // set up the LCD's number of columns and rows
      reinitdisplaycounter = 0;
    }
    #endif
    
    toggleTech(toggletech);// Using PUSH_1 and LED_1
    setBypass(bypass); // Using PUSH_2 and LED_2
    
    #ifdef READBACK
    readBack(DEVICE_ADDR_7bit, ReadBackAlg1Addr, 0x011E, &readback1); // raw abs value
    if(readback1 > maxreadback1)
      maxreadback1 = readback1;
    #endif
    
    if(old_func_counter != func_counter)
    {
      restore = 1;
      old_func_counter = func_counter;
    }
    switch(func_counter)
    {
    case 0: // Drive
      #ifndef STOMPBOX
      if(restore)
      {
        restore = 0;
        setPulses(param1_pulses);
      }
      param1_pulses = getPulses();
      param1_value = processencoder(DRIVE_MIN, DRIVE_MAX, param1_pulses);
      setDrive(param1_value);
      #endif
      break;
    case 1: // Tone
      #ifndef STOMPBOX
      if(restore)
      {
        restore = 0;
        setPulses(param2_pulses);
      }
      param2_pulses = getPulses();
      param2_value = processencoder(TONE_MIN, TONE_MAX, param2_pulses);
      param2_fake = processencoder(0.0, 100.0, param2_pulses);
      tone_eq.f0 = param2_value;
      EQ1stOrd(DEVICE_ADDR_7bit, ToneAddr, &tone_eq);
      #endif
      break;
    case 2: // Master Volume
      #ifndef STOMPBOX
      if(restore)
      {
        restore = 0;
        setPulses(param3_pulses);
      }
      param3_pulses = getPulses();
      param3_value = processencoder(MASTER_VOLUME_MIN, MASTER_VOLUME_MAX, param3_pulses);
      param3_fake = processencoder(0.0, 100.0, param3_pulses);
      MasterVolumeMono(DEVICE_ADDR_7bit, MasterVolumeAddr, pow(10, param3_value/20.0)); // Set Master Volume 
      #endif
      break;
    case 3: // Technology
      #ifndef STOMPBOX
      if(restore)
      {
        restore = 0;
        setPulses(param4_pulses);
      }
      param4_pulses = getPulses();
      param4_value = processencoder(MIX_MIN, MIX_MAX, param4_pulses); // Technology Mix
      setMix(param4_value);
      #endif
      break;
    } // End switch func_counter

    // Display information for user
    #ifndef STOMPBOX
    print_menu_putty();
    #else
    print_menu_lcd();
    #endif

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
  clearAndHome();    // !!!Warning use with real terminal emulation program
  Serial.println(F("********************************"));
  Serial.println(F("*   User control interface     *"));
  Serial.println(F("*   AIDA Tubescreamer 96k      *"));
  Serial.println(F("********************************"));
  Serial.write('\n');
  Serial.print(F("Encoder pulses: "));
  Serial.println(getPulses(), DEC);
  Serial.write('\n');
  
  #ifdef READBACK
  Serial.print(F(" Raw abs : ")); // Print linear values (max +/-1.00) for readback values
  Serial.println(maxreadback1, 3);
  #endif
  
  Serial.print(F("Effect status: "));
  if(bypass)
    Serial.println(F("bypass"));
  else
    Serial.println(F("on"));
  Serial.write('\n');  
  if(func_counter==0)
    Serial.print(F("    "));
  Serial.print(F("Drive: "));
  Serial.print(param1_value, 1);
  Serial.println(F(" %"));
  if(func_counter==1)
    Serial.print(F("    "));
  Serial.print(F("Tone: "));
  Serial.print(param2_fake, 1);
  Serial.println(F(" %"));
  /*Serial.print(param2_value, 2);
  Serial.println(F(" Hz"));*/
  if(func_counter==2)
    Serial.print(F("    "));
  Serial.print(F("Vol: "));
  Serial.print(param3_fake, 1);
  Serial.println(F(" %"));
  /*Serial.print(param3_value, 1);
  Serial.println(F(" dB"));*/
  if(func_counter==3)
    Serial.print(F("    "));
  Serial.print(F("Blend: "));
  Serial.print(param4_value, 1);
  Serial.println(F(" %"));
  if(param4_value<5.0)
    Serial.println(F("Si"));
  else if(param4_value>95.0)
    Serial.println(F("Ge"));
  
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
    lcd.print(F("Tubescreamer 96k")); 
    lcd.setCursor(0, 1);
    switch(func_counter)
    {
      case 0:
        lcd.print(F("Drive:"));
        lcd.print(param1_value, 1);
        lcd.print(F("%"));
        break;
      case 1:
        lcd.print(F("Tone:"));
        lcd.print(param2_fake, 1);
        lcd.print(F("%"));
        /*lcd.print(param2_value, 2);
        lcd.print(F("Hz"));*/
        break;
      case 2:
        lcd.print(F("Vol:"));
        lcd.print(param3_fake, 1);
        lcd.print(F("%"));
        /*lcd.print(param3_value, 2);
        lcd.print(F("dB"));*/
        break;
      case 3:
        lcd.print(F("Blend:"));
        lcd.print(param4_value, 1);
        lcd.print(F("%"));
        if(param4_value<5.0)
          lcd.print(F(" Si"));
        else if(param4_value>95.0)
          lcd.print(F(" Ge"));
        break;
    }
  }
}

void setBypass(uint8_t status)
{
  static uint8_t oldstatus = OFF;
  
  if(oldstatus != status)
  {
    if(status == ON)
    {
      muxnoiseless(DEVICE_ADDR_7bit, BypassAddr, 2); // Bypass
      digitalWrite(LED_2, HIGH);
    }
    else
    {
      muxnoiseless(DEVICE_ADDR_7bit, BypassAddr, 1); // FX
      digitalWrite(LED_2, LOW);
    }
    oldstatus = status;
  }
}

void setMix(float percent)
{
  static float oldpercent = 0;
  float value = 0.00;
  
  if(oldpercent != percent)
  {
    if(percent>100)
      percent = 100;
    value = percent/100.00;
  
    // MIX
    AIDA_SAFELOAD_WRITE_VALUE(DEVICE_ADDR_7bit, TechMixAddr, false, 1.00-value);  // Si	 
    AIDA_SAFELOAD_WRITE_VALUE(DEVICE_ADDR_7bit, TechMixAddr+1, true, value);   // Ge
  }
}

// This function set the linear gain. We cannot have
// gain more than 15.999 on a single cell, so we use two cells to achieve 
// the desired gain
void setDrive(float value)
{
  static float oldvalue = 0.00;
  float drive,a1,a2;
  float coefficients[3];
  uint16_t address = 0x00;
  
  address = OpampAddr;
  
  if(oldvalue != value)
  {
    if(value == 0.00)
      drive = 1.00;
    else
      drive = ((value * 13.0)/100) + 1.0;
    
    gainCell(DEVICE_ADDR_7bit, Drive1Addr, 10.0);
    delayMicroseconds(100);
    gainCell(DEVICE_ADDR_7bit, Drive2Addr, drive);
    delayMicroseconds(100);
    
    oldvalue = value;
  }
}

/* Deprecated version 
// This function calculates the opamp stage filter + gain
// unfortunately we can't set such enormous gain in a single 2nd order
// cell due to fixed point coefficient saturation
// so this function is implemented in another way 
// see my post here https://ez.analog.com/thread/82012
void setDrive(float value)
{
  static float oldvalue = 0.00;
  float drive,a1,a2;
  float coefficients[3];
  uint16_t address = 0x00;
  
  address = OpampAddr;
  
  if(oldvalue != value)
  {
    if(value == 0.00)
      drive = 1.00;
    else
      drive = value / 100.0; // Scaling to 0-1.0 range...

    a1 = 251.3184 + (drive * 2256.0);
    a2 = 21.2064;
    
    coefficients[0] = (1 + a1) / (1 + a2); // B0
    coefficients[1] = (1 - a1) / (1 + a2); // B1
    coefficients[2] = (1 - a2) / (1 + a2); // A1
  
    // Write coefficients to Sigma DSP
    #ifdef ADAU144x
      AIDA_SW_SAFELOAD_WRITE_VALUE(DEVICE_ADDR_7bit, address++, false, coefficients[0]);
      AIDA_SW_SAFELOAD_WRITE_VALUE(DEVICE_ADDR_7bit, address++, false, coefficients[1]);
      AIDA_SW_SAFELOAD_WRITE_VALUE(DEVICE_ADDR_7bit, address, true, coefficients[2]);
    #else
      AIDA_SAFELOAD_WRITE_VALUE(DEVICE_ADDR_7bit, address++, false, coefficients[0]);
      AIDA_SAFELOAD_WRITE_VALUE(DEVICE_ADDR_7bit, address++, false, coefficients[1]);
      AIDA_SAFELOAD_WRITE_VALUE(DEVICE_ADDR_7bit, address, true, coefficients[2]);
    #endif
    
    oldvalue = value;
  }
}
*/

void toggleTech(uint8_t toggle)
{
  static uint8_t oldtoggle = OFF;
  
  if(toggle != oldtoggle)
  {
    func_counter = 3; // Display blend parameter
    
    if(toggle==ON)
    {
      param4_value = 0.00;
      setMix(param4_value); // Si
      digitalWrite(LED_1, HIGH);
    }
    else
    {
      param4_value = 100.0;
      setMix(param4_value); // Ge
      digitalWrite(LED_1, LOW); // Turn Led ON 
    }
    oldtoggle = toggle;
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

