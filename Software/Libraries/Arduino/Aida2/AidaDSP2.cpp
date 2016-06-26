/*
  AidaDSP.cpp - Aida DSP library
 Copyright (c) 2016 Massimo Pennazio <maxipenna@libero.it>
 
 Version: 0.19 ADAU144x (Arduino)
 
 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.
 
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
 
#include "AidaDSP2.h"

#define ADAU144x

#ifndef ADAU144x
  #ifndef ADAU170x
    #error DSP not selected.
  #endif
#endif

#ifdef __AVR__ 
#define WIRE Wire
#else
#define WIRE Wire1 // Arduino2
#endif
#define FULLRANGEVAL 1024.0f
#define MIDDLEVAL (FULLRANGEVAL/2)

#ifdef ENC_RES_X4
#define N_ENC 24*4 // We manage the quadrature encoder with x4 resolution so 24*4 every turn.
#else
#define N_ENC 24
#endif
#define MAX_PULSES_ROUGH  N_ENC*1
//#define MAX_PULSES_FINE  N_ENC*10
#define MAX_PULSES_FINE  N_ENC*25

/************************************************
 *        PRIVATE VARIABLES (do not edit)       *
 ************************************************/
// ENCODER
volatile int32_t Pulses = 0;  // Modified in interrupt, so declared as volatile
volatile uint8_t chA = 0, chB = 0;
volatile uint8_t curr_state = 0x00;
volatile uint8_t prev_state = 0x00;
uint8_t enc_freeze = 0x00; // if 0x01, encoder stops count positive pulses, if 0x10 encoder stops count negative pulses
int32_t max_number_of_pulses = MAX_PULSES_ROUGH; // rough regulation default
float regulation_precision = 0.1; // decimal increment/decrement per encoder pulse used in processencoder2
// SAFELOAD PROCEDURE
uint8_t sw_safeload_count = 0;
uint8_t safeload_count = 0;


/************************************************
 *           AIDA HIGH LEVEL FUNCTIONS          *
 ************************************************/	
 
/**
 * Set fine or rough regulation for processencoder function
 * @param fine - if 1/true fine mode is activated
 */		  
void set_regulation_precision(uint8_t fine)
{
  if(fine)
    max_number_of_pulses = MAX_PULSES_FINE;
  else
    max_number_of_pulses = MAX_PULSES_ROUGH;
}

/**
 * Get fine or rough regulation setting used with processencoder function
 * @return fine - if 1/true fine mode is activated
 */		  
uint8_t get_regulation_precision(void)
{
  if(max_number_of_pulses == MAX_PULSES_FINE)
    return 1; // fine regulation
  else
    return 0; // rough regulation 
}

/**
 * Set fine or rough regulation for processencoder2 function
 * @param precision set decimal increment/decrement per encoder pulse
 */		  
void set_regulation_precision2(float precision)
{
  regulation_precision = precision;
}

/**
 * Get regulation precision setting used with processencoder2 function
 * @return regulation precision 
 */		  
float get_regulation_precision2(void)
{
  return regulation_precision;
}

/**
 * This function transform pulses from encoder in user defined range values
 * @param minval
 * @param maxval
 * @param pulses - the actual pulses count see getPulses()
 * @return float - return a value between minval and maxval when user turn encoder knob
 */
float processencoder(float minval, float maxval, int32_t pulses)
{
  float val = 0.00;
  
  if(minval < 0 && maxval <= 0)
  {
    if(pulses>=0)
      return maxval;
    else
    {
      val = ((abs(pulses)*(minval-maxval))/max_number_of_pulses)+maxval;
      if(val < minval)
        return minval;
      else
        return val;
    }
  }
  else if(minval >= 0 && maxval > 0)
  {
    if(pulses >= 0)
    {
      val = (pulses*((maxval-minval)/max_number_of_pulses))+minval;
      if(val > maxval)
        return maxval;
      else
        return val;
    }
    else
      return minval;
  }
  else if(minval < 0 && maxval > 0)
  {
    if(pulses >= 0)
    {
      if(pulses == 0) 
        return 0.00;
      else
      {
        val = pulses*(maxval/max_number_of_pulses);
        if(val > maxval)
          return maxval;
        else
          return val; 
      }
    }
    else // pulses < 0
    {
      val = abs(pulses)*(minval/max_number_of_pulses);
      if(val < minval)
        return minval;
      else
        return val;
    }    
  }
}

/**
 * This function transform pulses from encoder in user defined range values. 
 * At startup Pulses = 0 since most common knob encoder types are relative, and
 * this function returns zero/minval. Pulses can be initialized with setPulses() function. 
 * @param minval
 * @param maxval
 * @return float - return a value between minval and maxval when user turn encoder knob
 */
float processencoder2(float minval, float maxval)
{
  float tmp = 0.00;
  
  tmp = (Pulses*regulation_precision);
  
  if(tmp>maxval)
  {
    tmp = maxval;
    enc_freeze = 0x01; // Stop counting positive
  }
  else if(tmp<minval)
  {
    tmp = minval;
    enc_freeze = 0x10; // Stop counting negative
  }
  else
  {
    enc_freeze = 0x00; // Free run
  }
  
  return tmp;
}

/**
 * !!!DEPRECATED!!! Do not use this function, still here for implementation example purpose
 * This function transform pulses from encoder in user defined range values. 
 * At startup Pulses = 0 since most common knob encoder types are relative, and
 * this function returns middleval. Pulses can be initialized with setPulses() function. 
 * @param minval
 * @param maxval
 * @return float - return a value between minval and maxval when user turn encoder knob
 */
/*float processencoder2(float minval, float maxval)
{
  float tmp = 0.00;
  float middleval = 0.00;
  
  if(minval < 0 && maxval <= 0)
  {
    middleval = ((minval-maxval)/2.0f) + maxval;
  }
  else if(minval >= 0 && maxval > 0)
  {
    middleval = ((maxval-minval)/2.0f) + minval;
  }
  else if(minval < 0 && maxval > 0)
  {
    middleval = 0.00;
  }
  
  tmp = middleval + (Pulses*regulation_precision);
  #ifdef ENC_RES_X4
  tmp = middleval + ((Pulses/4)*regulation_precision);
  #else
    #ifdef ENC_RES_X2
      tmp = middleval + ((Pulses/2)*regulation_precision);
    #else  
      tmp = middleval + (Pulses*regulation_precision);
    #endif
  #endif
      
  if(Pulses==0)
  {
    return middleval;
  }
  else if(Pulses>0)
  {
    if(tmp>maxval)
      tmp = maxval;
    return tmp;
  }
  else if(Pulses<0)
  {
    if(tmp<minval)
      tmp = minval;
    return tmp;
  }
}*/

/**
 * This function transform pulses from encoder in a selector which returns integer indexes 
 * useful for mux switch operation or menu entries
 * @param pulses - the actual pulses count see getPulses()
 * @param bits - number of bits for selector: 2=1:4, 3=1:8, etc...
 * @return uint16 - return a value between minval and maxval when user turn encoder knob
 */
uint16_t selectorwithencoder(int32_t pulses, uint8_t bits)
{
  uint16_t result = 1;
  
  if(pulses>16 && bits>0)
  {
	result = (pulses>>4)&0x0F;
	if(result > (1<<bits))
	  result = (1<<bits);
  }
  
  return result;  // Because we manage encoder in 4x resolution so every step on the encoder gives 4 increment 
}

/**
 * This function transform values from pot mounted on adc input in user defined range values
 * @param minval
 * @param maxval
 * @param potval - the actual adc value returned by analogRead(Ax)
 * @return float - return a value between minval and maxval when user turn a pot mounted on adc input
 */
float processpot(float minval, float maxval, uint16_t potval)
{
  if(minval < 0 && maxval <= 0)
  {
    return (((potval*((abs(minval)-abs(maxval))/FULLRANGEVAL)))+minval);    
  }
  else if(minval >= 0 && maxval > 0)
  {
    return ((((potval*(maxval-minval)/FULLRANGEVAL)))+minval);
  }
  else if(minval < 0 && maxval > 0)
  {
    if(potval >= MIDDLEVAL)
        return ((potval-MIDDLEVAL)*(maxval/MIDDLEVAL));
      else
        return ((MIDDLEVAL-potval)*(minval/MIDDLEVAL));
  }
}

/**
 * This function transform values from pot mounted on adc input in a selector which returns integer indexes 
 * useful for mux switch operation or menu entries
 * @param potval - the actual adc value returned by analogRead(Ax)
 * @param bits - number of bits for selector: 2=1:4, 3=1:8, etc...
 * @return uint16 - return a value between minval and maxval when user turn encoder knob
 */
uint16_t selectorwithpot(uint16_t potval, uint8_t bits)
{
  uint16_t result = 1;
  
  #ifdef __AVR__  // 10 bits ADCs
    if(bits>0 && bits<=10)
      result = potval >> (10-bits);
  #else // 12 bits ADCs
    if(bits>0 && bits<=12)
      result = potval >> (12-bits);
  #endif
  
  return result;
}

/**
 * This function returns 1/true if input value is between a reference value with threshold
 * useful for know if a knob has been turned by user, without picking noise
 * @param value - the actual value you want to compare
 * @param reference - your reference value
 * @return threshold - value of threshold 
 */
uint8_t isinrange(int16_t value, int16_t reference, int16_t threshold)
{
  if(value < (reference+threshold) && value > (reference-threshold))
  {
    return 1;
  }
  else 
    return 0;
}

/**
 * This function initialize Aida DSP board by configuring I/O and
 * leaves DSP in reset state when exits
 */
void InitAida(void)
{
  #ifdef __AVR__
	  pinMode(ENC_PUSH, INPUT); // 3.3v pull-up provided on Aida DSP board
	  pinMode(ENCA, INPUT);
	  pinMode(ENCB, INPUT);
	  #ifdef ENC_RES_X1
      attachInterrupt(1, enc_manager, RISING);  // Arduino Mega: interrupts are on fixed pins only (ENCA, Pin 3)
	  #else 
      #ifdef ENC_RES_X2
        attachInterrupt(0, enc_manager, RISING);  // Arduino Mega: interrupts are on fixed pins only (ENCB, Pin 2)
        attachInterrupt(1, enc_manager, RISING);  // Arduino Mega: interrupts are on fixed pins only (ENCA, Pin 3)
      #else
        attachInterrupt(0, enc_manager, CHANGE);  // Arduino Mega: interrupts are on fixed pins only (ENCB, Pin 2)
        attachInterrupt(1, enc_manager, CHANGE);  // Arduino Mega: interrupts are on fixed pins only (ENCA, Pin 3)
      #endif
	  #endif
  #else
    pinMode(ENC_PUSH, INPUT_PULLUP);
    pinMode(ENCA, INPUT_PULLUP);
    pinMode(ENCB, INPUT_PULLUP);
    #ifdef ENC_RES_X1
      attachInterrupt(ENCA, enc_manager, RISING); // Interrupt is fired whenever ENC changes state
    #else
      #ifdef ENC_RES_X2
        attachInterrupt(ENCA, enc_manager, RISING); // Interrupt is fired whenever ENC changes state
        attachInterrupt(ENCB, enc_manager, RISING); // Both channels, rising for x2 resolution 360°/24*2
      #else
        attachInterrupt(ENCA, enc_manager, CHANGE); // Interrupt is fired whenever ENC changes state
        attachInterrupt(ENCB, enc_manager, CHANGE); // Both channels, rising-falling for x4 resolution 360°/24*4
      #endif
    #endif
  #endif
  
  pinMode(SBOOT, OUTPUT);
  pinMode(RESET, OUTPUT);
 
  digitalWrite(SBOOT, LOW);  // Self-boot: DSP in uC control mode
  digitalWrite(RESET, LOW);  // Hold DSP in RESET state (halted) 

  WIRE.begin(); // join i2c bus (address optional for master)
  WIRE.setClock(400000UL); // use i2c peripheral module in fast-mode (400kHz i2c clk)
  //WIRE.setClock(100000UL); // use i2c peripheral module in default-mode (100kHz i2c clk)
  delay(10); 
}

/**
 * Interrupt for encoder management
 */
void enc_manager(void)
{
  uint8_t change = 0;
  
  chA = digitalRead(ENCA);
  chB = digitalRead(ENCB);
  
  prev_state = curr_state;
  curr_state = (chA<<1 | chB);		// 2-bit state
  
  //Entered a new valid state.
  #ifdef ENC_RES_X1
    if(chB == 1 && chA == 0)
      Pulses--;
    else
      Pulses++;
  #endif
  #ifdef ENC_RES_X4
    if (((curr_state ^ prev_state) != INVALID) && (curr_state != prev_state))
    {
      //2 bit state. Right hand bit of prev XOR left hand bit of current
      //gives 0 if clockwise rotation and 1 if counter clockwise rotation.
      change = (prev_state & PREV_MASK) ^ ((curr_state & CURR_MASK) >> 1);
      if (change == 0)
      {
        if(enc_freeze!=0x10) // if negative limit is not set
          Pulses--;
      }
      else
      {
        if(enc_freeze!=0x01) // if positive limit is not set
          Pulses++;
      }
    }
    else
    {
      // Error
    }
  #endif  
}

// Methods for get/set encoder's pulses value
int32_t getPulses(void)
{
	return Pulses;
}

void setPulses(int32_t value)
{
    Pulses = value;
}

/**
 * This function controls a standard gain cell 
 * @param dspAddress - the physical I2C address (7-bit format)
 * @param address - the param address of the cell
 * @param value - the desired gain value (linear)
 */
void gainCell(uint8_t dspAddress, uint16_t address, float value)
{
	#ifdef ADAU144x
    AIDA_SW_SAFELOAD_WRITE_VALUE(dspAddress, address, true, value);
  #else
    AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address, true, value);
  #endif
}

/**
 * This function controls a mono volume cell 
 * @param dspAddress - the physical I2C address (7-bit format)
 * @param address - the param address of the cell
 * @param value - the desired gain value (linear)
 */
void MasterVolumeMono(uint8_t dspAddress, uint16_t address, float value)
{
  #ifdef ADAU144x
    AIDA_SW_SAFELOAD_WRITE_VALUE(dspAddress, address, true, value);
  #else
    AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address, true, value);
  #endif
}

/**
 * This function controls a stereo volume cell
 * @param dspAddress - the physical I2C address (7-bit format)
 * @param address - the param address of the cell
 * @param value - the desired gain value (linear) for both channels of stereo pairs 
 */
void MasterVolumeStereo(uint8_t dspAddress, uint16_t address, float value)
{
  #ifdef ADAU144x
    AIDA_SW_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, value);
    AIDA_SW_SAFELOAD_WRITE_VALUE(dspAddress, address, true, value);
  #else
    AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, value);
    AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address, true, value);
  #endif
}

/**
 * This function manages a general 1st order filter block
 * @param dspAddress - the physical I2C address (7-bit format)
 * @param address - the param address of the cell
 * @param equalizer - the struct which contains multiple settings for equalizer (see AidaDSP.h)
 */
void EQ1stOrd(uint8_t dspAddress, uint16_t address, equalizer_t* equalizer){

  float w0,gainLinear;
  float b0,b1,a1;
  float coefficients[3];

  w0=2*pi*equalizer->f0/FS;		          //2*pi*f0/FS
  gainLinear = pow(10,(equalizer->gain/20));      //10^(gain/20)

  switch(equalizer->type)
  {
    case Lowpass:
      a1 = pow(2.7,-w0);
      b0 = gainLinear * (1.0 - a1);
      b1 = 0;
      break;   
    case Highpass:
      a1 = pow(2.7,-w0);
      b0 = gainLinear * a1;
      b1 = -a1 * gainLinear;
      break;
  }

  if(equalizer->onoff == true)
  {
    if(equalizer->phase == false) // 0°
    {
      coefficients[0] = b0;
      coefficients[1] = b1;
      coefficients[2] = a1;			
    }
    else // 180°
    {
      coefficients[0] = -1*b0;
      coefficients[1] = -1*b1;
      coefficients[2] = a1; // This coefficient does not change sign
    }
  }
  else
  {
    coefficients[0] = 1.00;
    coefficients[1] = 0.00;
    coefficients[2] = 0.00;
  }

  // Write coefficients to Sigma DSP
  #ifdef ADAU144x
    AIDA_SW_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, coefficients[0]);
    AIDA_SW_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, coefficients[1]);
    AIDA_SW_SAFELOAD_WRITE_VALUE(dspAddress, address, true, coefficients[2]);
  #else
    AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, coefficients[0]);
    AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, coefficients[1]);
    AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address, true, coefficients[2]);
  #endif

  //Serial.write('\n'); //!!!Debug!!!
  //Serial.println(coefficients[0], 3);
  //Serial.println(coefficients[1], 3);
  //Serial.println(coefficients[2], 3);
}

/**
 * This function manages a general 2nd order filter block
 * @param dspAddress - the physical I2C address (7-bit format)
 * @param address - the param address of the cell
 * @param equalizer - the struct which contains multiple settings for equalizer (see AidaDSP.h)
 */
void EQ2ndOrd(uint8_t dspAddress, uint16_t address, equalizer_t* equalizer){

  float A,w0,alpha,gainLinear;
  float b0,b1,b2,a0,a1,a2;
  float coefficients[4];
    
  A=pow(10,(equalizer->boost/40));                //10^(boost/40)
  w0=2*pi*equalizer->f0/FS;		          //2*pi*f0/FS
  gainLinear = pow(10,(equalizer->gain/20));      //10^(gain/20)

  switch(equalizer->type)
  {
    case Parametric:
    case Peaking: // Peaking filter is a Parametric filter with fixed Q???
      alpha = sin(w0)/(2*equalizer->Q); 
      a0 =  1 + alpha/A;
      a1 = -2 * cos(w0);
      a2 =  1 - alpha/A;
      b0 = (1 + alpha*A) * gainLinear;
      b1 = -(2 * cos(w0)) * gainLinear;
      b2 = (1 - alpha*A) * gainLinear;
      break;
    case LowShelf:
      alpha=sin(w0)/2*sqrt((A+1/A)*(1/equalizer->S-1)+2);
      a0 =  (A+1)+(A-1)*cos(w0)+2*sqrt(A)*alpha;
      a1 = -2*((A-1)+(A+1)*cos(w0));
      a2 =  (A+1)+(A-1)*cos(w0)-2*sqrt(A)*alpha;
      b0 = A*((A+1)-(A-1)*cos(w0)+2*sqrt(A)*alpha)*gainLinear;
      b1 = 2*A*((A-1)-(A+1)*cos(w0)) * gainLinear;
      b2 = A*((A+1)-(A-1)*cos(w0)-2*sqrt(A)*alpha)*gainLinear;
      break;
    case HighShelf:
      alpha = sin(w0)/2 * sqrt( (A + 1/A)*(1/equalizer->S - 1) + 2 ); 
      a0 =       (A+1) - (A-1)*cos(w0) + 2*sqrt(A)*alpha;
      a1 =   2*( (A-1) - (A+1)*cos(w0) );
      a2 =       (A+1) - (A-1)*cos(w0) - 2*sqrt(A)*alpha;
      b0 =   A*( (A+1) + (A-1)*cos(w0) + 2*sqrt(A)*alpha ) * gainLinear;
      b1 = -2*A*( (A-1) + (A+1)*cos(w0) ) * gainLinear;
      b2 =   A*( (A+1) + (A-1)*cos(w0) - 2*sqrt(A)*alpha ) * gainLinear;
      break;
    case Lowpass:
      alpha = sin(w0)/(2*equalizer->Q);
      a0 =   1 + alpha;
      a1 =  -2*cos(w0);
      a2 =   1 - alpha;
      b0 =  (1 - cos(w0)) * (gainLinear/2);
      b1 =   1 - cos(w0)  * gainLinear;
      b2 =  (1 - cos(w0)) * (gainLinear/2);
      break;
    case Highpass:
      alpha = sin(w0)/(2*equalizer->Q);
      a0 =   1 + alpha;
      a1 =  -2*cos(w0);
      a2 =   1 - alpha;
      b0 =  (1 + cos(w0)) * (gainLinear/2);
      b1 = -(1 + cos(w0)) * gainLinear;
      b2 =  (1 + cos(w0)) * (gainLinear/2);
      break;
    case Bandpass:
      alpha = sin(w0) * sinh(log(2)/(2 * equalizer->bandwidth * w0/sin(w0)));
      a0 =   1 + alpha;
      a1 =  -2*cos(w0);
      a2 =   1 - alpha;
      b0 =   alpha * gainLinear;
      b1 =   0;
      b2 =  -alpha * gainLinear;
      break;
    case Bandstop:
      alpha = sin(w0) * sinh( log(2)/(2 * equalizer->bandwidth * w0/sin(w0)));
      a0 =   1 + alpha;
      a1 =  -2*cos(w0);
      a2 =   1 - alpha;
      b0 =   1 * gainLinear;
      b1 =  -2*cos(w0) * gainLinear;  
      b2 =   1 * gainLinear;
      break;
    case ButterworthLP:
      alpha = sin(w0) / 2.0 * 1/sqrt(2);
      a0 =   1 + alpha;
      a1 =  -2*cos(w0);
      a2 =   1 - alpha;
      b0 =  (1 - cos(w0)) * gainLinear / 2;
      b1 =   1 - cos(w0)  * gainLinear;
      b2 =  (1 - cos(w0)) * gainLinear / 2;
      break;
    case ButterworthHP:
      alpha = sin(w0) / 2.0 * 1/sqrt(2);
      a0 =   1 + alpha;
      a1 =  -2*cos(w0);
      a2 =   1 - alpha;
      b0 = (1 + cos(w0)) * gainLinear / 2;
      b1 = -(1 + cos(w0)) * gainLinear;
      b2 = (1 + cos(w0)) * gainLinear / 2;
      break;
    case BesselLP:
      alpha = sin(w0) / 2.0 * 1/sqrt(3) ;
      a0 =   1 + alpha;
      a1 =  -2*cos(w0);
      a2 =   1 - alpha;
      b0 =  (1 - cos(w0)) * gainLinear / 2;
      b1 =   1 - cos(w0)  * gainLinear;
      b2 =  (1 - cos(w0)) * gainLinear / 2;
      break;
    case BesselHP:
      alpha = sin(w0) / 2.0 * 1/sqrt(3) ;
      a0 =   1 + alpha;
      a1 =  -2*cos(w0);
      a2 =   1 - alpha;
      b0 = (1 + cos(w0)) * gainLinear / 2;
      b1 = -(1 + cos(w0)) * gainLinear;
      b2 = (1 + cos(w0)) * gainLinear / 2;
      break;
  }
  
  // For Sigma DSP implementation we need to normalize all the coefficients respect to a0
  // and inverting by sign a1 and a2  
  if(a0 != 0.00 && equalizer->onoff == true)
  {
    if(equalizer->phase == false) // 0°
    {
      coefficients[0]=b0/a0;
      coefficients[1]=b1/a0;
      coefficients[2]=b2/a0;
      coefficients[3]=-1*a1/a0;
      coefficients[4]=-1*a2/a0;
    }
    else // 180°
    {
      coefficients[0]=-1*b0/a0;
      coefficients[1]=-1*b1/a0;
      coefficients[2]=-1*b2/a0;
      coefficients[3]=-1*a1/a0; // This coefficient does not change sign!
      coefficients[4]=-1*a2/a0; // This coefficient does not change sign!
    }
  }
  else    // off or disable position
  {
    coefficients[0]=1.00;
    coefficients[1]=0;
    coefficients[2]=0;
    coefficients[3]=0;
    coefficients[4]=0;
  }

  // Write coefficients to Sigma DSP
  #ifdef ADAU144x
    AIDA_SW_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, coefficients[0]);
    AIDA_SW_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, coefficients[1]);
    AIDA_SW_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, coefficients[2]);
    AIDA_SW_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, coefficients[3]);
    AIDA_SW_SAFELOAD_WRITE_VALUE(dspAddress, address, true, coefficients[4]);
  #else
    AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, coefficients[0]);
    AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, coefficients[1]);
    AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, coefficients[2]);
    AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, coefficients[3]);
    AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address, true, coefficients[4]);
  #endif
  
  /*Serial.write('\n'); //!!!Debug!!!
  Serial.println(coefficients[0], 3);
  Serial.println(coefficients[1], 3);
  Serial.println(coefficients[2], 3);
  Serial.println(coefficients[3], 3);
  Serial.println(coefficients[4], 3);*/
}

/**
 * This function manages a baxandall low-high dual tone control
 * @param dspAddress - the physical I2C address (7-bit format)
 * @param address - the param address of the cell
 * @param toneCtrl - the struct which contains multiple settings for tone control (see AidaDSP.h)
 */
void ToneControl(uint8_t dspAddress, uint16_t address, toneCtrl_t *toneCtrl){
 
  float tb,bb,wT,wB,Knum_T,Kden_T,Knum_B,Kden_B,alpha0,beta1,alpha1,beta2,alpha2,beta3,alpha3,beta4;
  float b0,b1,b2,a0,a1,a2;
  float coefficients[4];
  
  tb = pow(10, toneCtrl->Boost_Treble_dB/20.0);
  bb = pow(10, toneCtrl->Boost_Bass_dB/20.0);

  wT = tan(pi * toneCtrl->Freq_Treble / FS);
  wB = tan(pi * toneCtrl->Freq_Bass / FS);

  Knum_T = 2 / (1 + (1.0 / tb));
  Kden_T = 2 / (1 + tb);
  Knum_B = 2.0 / (1.0 + (1.0 / bb));
  Kden_B = 2.0 / (1.0 + bb);

  alpha0 = wT + Kden_T;
  beta1 = wT + Knum_T;
  alpha1 = wT - Kden_T; 
  beta2 = wT - Knum_T;

  alpha2 = (wB*Kden_B) + 1;
  beta3 = (wB*Knum_B) - 1;
  alpha3 = (wB*Kden_B) - 1;
  beta4 = (wB*Knum_B) + 1;
  
  a0 = alpha0 * alpha2;
  a1 = (alpha0 * alpha3) + (alpha1 * alpha2);
  a2 = alpha1 * alpha3; 
  b0 = beta1 * beta3;
  b1 = (beta1 * beta4) + (beta2 * beta3);
  b2 = beta2 * beta4; 
  
  // For Sigma DSP implementation we need to normalize all the coefficients respect to a0
  // and inverting a1 and a2 by sign 
  if(a0 != 0.00 && toneCtrl->onoff == true)
  {
    if(toneCtrl->phase == false) // 0°
    {
      coefficients[0]=b0/a0;
      coefficients[1]=b1/a0;
      coefficients[2]=b2/a0;
      coefficients[3]=-1*a1/a0;
      coefficients[4]=-1*a2/a0;
    }
    else // 180°
    {
      coefficients[0]=-1*b0/a0;
      coefficients[1]=-1*b1/a0;
      coefficients[2]=-1*b2/a0;
      coefficients[3]=a1/a0;
      coefficients[4]=a2/a0;
    }
  }
  else    // off or disable position
  {
    coefficients[0]=1.00;
    coefficients[1]=0;
    coefficients[2]=0;
    coefficients[3]=0;
    coefficients[4]=0;
  }

  // Write coefficients to Sigma DSP
  #ifdef ADAU144x
    AIDA_SW_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, coefficients[0]);
    AIDA_SW_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, coefficients[1]);
    AIDA_SW_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, coefficients[2]);
    AIDA_SW_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, coefficients[3]);
    AIDA_SW_SAFELOAD_WRITE_VALUE(dspAddress, address, true, coefficients[4]);
  #else
    AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, coefficients[0]);
    AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, coefficients[1]);
    AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, coefficients[2]);
    AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, coefficients[3]);
    AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address, true, coefficients[4]);
  #endif
}

/**
 * This function control a state variable filter block
 * @param dspAddress - the physical I2C address (7-bit format)
 * @param address - the param address of the cell
 * @param frequency - frequency range: 1-19148 Hz 
 * @param q - q range: 1.28:10
 */			
void StateVariable(uint8_t dspAddress, uint16_t address, float frequency, float q){

  float param1 = 0.00, param2 = 0.00;
	
  param1 = 2*sin(pi*frequency/FS);	
  param2 = 1/q;
	
  // Write parameters to Sigma DSP
  #ifdef ADAU144x
    AIDA_SW_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, param1);
    AIDA_SW_SAFELOAD_WRITE_VALUE(dspAddress, address++, true, param2);
  #else
    AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, param1);
    AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, true, param2);
  #endif
}

/*
This function calulates the points of a linearly spaced vector
 */
void linspace(float x1, float x2, float n, float vect[])
{
  uint8_t i;
  float k;

  k = (abs(x1) + abs(x2))/(n);

  for(i=0;i<n;i++)
    vect[i] = x1+(k*i);    
}

 /**
 * This function calculates the curve and the other parameters of a rms compressor block 
 * @param dspAddress - the physical I2C address (7-bit format)
 * @param address - the param address of the cell
 * @param compressor - the struct which contains multiple settings for compressor control (see AidaDSP.h)
 * compressor_t: struct containing classic compressor parameters
 *   thresold: [dB]
 *   ratio: compressor's ratio. If ratio = 6, gain = 1/ratio = 1/6 so compression ratio 6:1
 *   attack: range 1-500 [ms] 
 *   hold: range 1-attack [ms]
 *   decay: range 1-2000 [ms]
 */
void CompressorRMS(uint8_t dspAddress, uint16_t address, compressor_t* compressor)    // set ratio = 1 to disable compressor
{
  uint8_t i,count;

  float curve[34];
  float x[34];
  float y[34];
  float delta=0.00;
  float coeff=0.00;

  float dbps = 0.00;
  float attack_par = 0.00;
  float hold_par = 0.00;
  float decay_par = 0.00;
  float postgain_par = 0.00;

  linspace(-90, 6, 34, x);

  count = 0;

  coeff = 1/compressor->ratio;
  for(i=0;i<34;i++) // This algorithm creates the curve with 1 1 1 1 1 1 1 0.9 0.8 0.72 0.64 and so on coefficients 
  {
    if(x[i]>=compressor->threshold)
    {
      count++;
      if(count==1)
      {
        delta = x[i]*coeff-x[i]*1;
      }
      y[i] = (x[i] * coeff)-delta;
    }
    else
      y[i] = x[i] * 1;
  }

  for(i=0;i<34;i++)  // Coefficients of the curve calculation
  {
    curve[i] = (pow(10, y[i]/20)) / (pow(10, x[i]/20));  // Coefficients are the ratio between the linearized values of vect. y and x
  }

  // Parameter Load into Sigma DSP
  for(i=0;i<34;i++)
  {
    #ifdef ADAU144x
      AIDA_SW_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, curve[i]);
    #else
      AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, curve[i]);
    #endif
  }
  // Conversion dbps -> ms
  // dbps = 121;
  // TCms = (20/(dbps*2.3))*1000

  // RMS TC (db/s)
  dbps = (20/(compressor->attack*2.3))*1e3; 
  attack_par = abs(1.0 - pow(10,(dbps/(10*FS))));
  #ifdef ADAU144x
    AIDA_SW_SAFELOAD_WRITE_VALUE(dspAddress, address++, true, attack_par);
  #else
    AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, true, attack_par);
  #endif
  
  #ifdef COMPRESSORWITHPOSTGAIN
    postgain_par = pow(10, compressor->postgain/40);
    #ifdef ADAU144x
      AIDA_SW_SAFELOAD_WRITE_VALUE(dspAddress, address++, true, postgain_par);
    #else
      AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, true, postgain_par);
    #endif
  #endif

  // Hold
  hold_par = compressor->hold*FS/1000;  
  #ifdef ADAU144x
    AIDA_SW_SAFELOAD_WRITE_VALUE(dspAddress, address++, true, hold_par);
  #else
    AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, true, hold_par);
  #endif
  
  // Decay (db/s)
  dbps = (20/(compressor->decay*2.3))*1e3;
  decay_par = dbps/(96*FS); 
  #ifdef ADAU144x
    AIDA_SW_SAFELOAD_WRITE_VALUE(dspAddress, address, true, decay_par);
  #else
    AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address, true, decay_par);
  #endif
}

/**
 * This function calculates the curve and the other parameters of a peak compressor block 
 * @param dspAddress - the physical I2C address (7-bit format)
 * @param address - the param address of the cell
 * @param compressor - the struct which contains multiple settings for compressor control (see AidaDSP.h)
 * compressor_t: struct containing classic compressor parameters
 *   thresold: [dB]
 *   ratio: compressor's ratio. If ratio = 6, gain = 1/ratio = 1/6 so compression ratio 6:1
 *   attack: there is no attack parameter for peak compressor (very fast) 
 *   hold: range 1-? [ms]
 *   decay: range 1-2000 [ms]
 */
void CompressorPeak(uint8_t dspAddress, uint16_t address, compressor_t* compressor)    // set ratio = 1 to disable compressor
{
  uint8_t i,count;

  float curve[33];
  float x[33];
  float y[33];
  float delta=0.00;
  float coeff=0.00;

  float dbps = 0.00;
  float attack_par = 0.00;
  float hold_par = 0.00;
  float decay_par = 0.00;
  float postgain_par = 0.00;

  linspace(-90, 6, 33, x);

  count = 0;

  coeff = 1/compressor->ratio;
  for(i=0;i<33;i++) // This algorithm creates the curve with 1 1 1 1 1 1 1 0.9 0.8 0.72 0.64 and so on coefficients 
  {
    if(x[i]>=compressor->threshold)
    {
      count++;
      if(count==1)
      {
        delta = x[i]*coeff-x[i]*1;
      }
      y[i] = (x[i] * coeff)-delta;
    }
    else
      y[i] = x[i] * 1;
  }

  for(i=0;i<33;i++)  // Coefficients of the curve calculation
  {
    curve[i] = (pow(10, y[i]/20)) / (pow(10, x[i]/20));  // Coefficients are the ratio between the linearized values of vect. y and x
  }

  // Parameter Load into Sigma DSP
  for(i=0;i<33;i++)
  {
    #ifdef ADAU144x
      AIDA_SW_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, curve[i]);
    #else
      AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, curve[i]);
    #endif
  }

  #ifdef COMPRESSORWITHPOSTGAIN
    postgain_par = pow(10, compressor->postgain/40);
    #ifdef ADAU144x
      AIDA_SW_SAFELOAD_WRITE_VALUE(dspAddress, address++, true, postgain_par);
    #else
      AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, true, postgain_par);
    #endif
  #endif

  // Hold
  hold_par = compressor->hold*FS/1000;
  #ifdef ADAU144x
    AIDA_SW_SAFELOAD_WRITE_VALUE(dspAddress, address++, true, hold_par);
  #else
    AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, true, hold_par);
  #endif

  // Decay (db/s)
  dbps = (20/(compressor->decay*2.3))*1e3;
  decay_par = dbps/(96*FS); 
  #ifdef ADAU144x
    AIDA_SW_SAFELOAD_WRITE_VALUE(dspAddress, address, true, decay_par);
  #else
    AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address, true, decay_par);
  #endif 
}

/**
 * Warning!!! ADAU177x Only!!!
 * This function reads value of signal inside DSP chain, useful for monitoring
 * levels from inside DSP algorithm, uses a readback cell
 * @param dspAddress - the physical I2C address (7-bit format)
 * @param address - the param address of the cell
 * @param capturecount - the program count position at which you placed readback cell (see examples) 
 * @param value - the return linear value in floating point of audio level on readback cell. Range: +/-1.0
 */
void readBack(uint8_t dspAddress, uint16_t address, uint16_t capturecount, float *value){

  uint8_t buf[3];
  int32_t word32 = 0;

  buf[0] = capturecount >> 8;
  buf[1] = (uint8_t)capturecount & 0xFF;
  AIDA_WRITE_REGISTER(dspAddress, address, 2, buf);    // readBack operation 

  memset(buf, 0, 3);

  AIDA_READ_REGISTER(dspAddress, address, 3, buf);

  word32 = ((uint32_t)buf[0]<<24 | (uint32_t)buf[1]<<16 | (uint32_t)buf[2]<<8)&0xFFFFFF00; // MSB first, convert from 5.19 to 5.27 format
  
  if(word32==0)
    word32 = 1;

  *value = ((float)word32/((uint32_t)1 << 27)); // I'm converting from 5.27 int32 to maintain sign
}

/**
 * Warning!!! ADAU144x Only!!!
 * This function reads value of signal inside DSP chain, useful for monitoring
 * levels from inside DSP algorithm, uses a readback cell
 * @param dspAddress - the physical I2C address (7-bit format)
 * @param address - the param address of the cell
 * @param value - the return linear value in floating point of audio level on readback cell. Range: +/-1.0
 */
void readBack2(uint8_t dspAddress, uint16_t address, float *value){

  uint8_t buf[4];
  int32_t word32 = 0;

  memset(buf, 0, 4);
  
  AIDA_READ_REGISTER(dspAddress, address, 4, buf);
  
  word32 = ((uint32_t)buf[0]<<24 | (uint32_t)buf[1]<<16 | (uint32_t)buf[2]<<8 | (uint32_t)buf[3]); // MSB first, 5.23 
  word32 = (word32 << 4)&0xFFFFFFF0; // MSB first, 5.27
   
  if(word32==0)
    word32 = 1;
  
  //*value = ((float)word32/((uint32_t)1 << 23)); // Standard fixed point 5.23 conversion
  *value = ((float)word32/((uint32_t)1 << 27)); // I'm converting from 5.27 int32 to maintain sign
}

/**
 * This function controls an audio multiplexer cell (switch audio signals)
 * @param dspAddress - the physical I2C address (7-bit format)
 * @param address - the param address of the cell
 * @param select - the index (the signal) you want to switch to
 * @param nchannels - the total number of channels switchable in mux cell
 */
void mux(uint8_t dspAddress, uint16_t address, uint8_t select, uint8_t nchannels)
{
  uint8_t i;

  for(i=1;i<=nchannels;i++)
  {
    if(i==select)
      AIDA_WRITE_VALUE(dspAddress, address++, 1.00);  
    else
      AIDA_WRITE_VALUE(dspAddress, address++, 0.00);  
  }  
}

/**
 * This function controls an audio multiplexer cell (switch audio signals)
 * noiseless (clickless) version
 * @param dspAddress - the physical I2C address (7-bit format)
 * @param address - the param address of the cell
 * @param select - the index (the signal) you want to switch to range 1-N
 */
void muxnoiseless(uint8_t dspAddress, uint16_t address, uint8_t select)
{
  uint8_t buf[4];
  uint32_t value;
  
  if(select==0)
    value = 0;
  else
    value = (uint32_t)select-1; 
  buf[0] = (value>>24)&0xFF; // MSB first
  buf[1] = (value>>16)&0xFF;
  buf[2] = (value>>8)&0xFF;
  buf[3] = value&0xFF; 
  
  AIDA_WRITE_REGISTER(dspAddress, address, 4, buf);
}

/**
 * This function controls an hard saturation cell
 * with separate negative and positive threshold
 * @param dspAddress - the physical I2C address (7-bit format)
 * @param address - the param address of the cell
 * @param th_high - the high threshold 0-1.0
 * @param th_low - the low threshold -1.0-0
 */
void hard_clip(uint8_t dspAddress, uint16_t address, float th_high, float th_low)
{
  uint8_t buffer[8];
  
  float_to_fixed(th_high, &buffer[0]);
  float_to_fixed(th_low, &buffer[4]);
  
  AIDA_WRITE_REGISTER(dspAddress, address, 8, buffer);
}

/**
 * This function controls an soft saturation cell
 * @param dspAddress - the physical I2C address (7-bit format)
 * @param address - the param address of the cell
 * @param alpha - range 0.1-10.0 the main coefficient for soft clipping curve, the
 * higher the coefficient, the smoothest the curve
 */
void soft_clip(uint8_t dspAddress, uint16_t address, float alpha)
{
  float onethird = 0.333;
  float twothird = 0.666;
  
  #ifdef ADAU144x
    AIDA_SW_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, alpha);
    AIDA_SW_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, 1/alpha);
    AIDA_SW_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, onethird);
    AIDA_SW_SAFELOAD_WRITE_VALUE(dspAddress, address++, true, twothird);
  #else
    AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, alpha);
    AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, 1/alpha);
    AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, onethird);
    AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, true, twothird);
  #endif
}

/**
 * This function controls a DC source cell
 * @param dspAddress - the physical I2C address (7-bit format)
 * @param address - the param address of the cell
 * @param value - dc value level range +/-1.0
 */
void dc_source(uint8_t dspAddress, uint16_t address, float value)
{
  #ifdef ADAU144x
    AIDA_SW_SAFELOAD_WRITE_VALUE(dspAddress, address, true, value);
  #else
    AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address, true, value);
  #endif
}

/**
 * This function controls a sine source cell
 * @param dspAddress - the physical I2C address (7-bit format)
 * @param address - the param address of the cell
 * @param frequency - the desired frequency for the signal
 */
void sine_source(uint8_t dspAddress, uint16_t address, float frequency)
{
	float value = (1.00/24000.00)*frequency;
	uint8_t buffer[4]={0x00, 0x00, 0x00, 0xFF};
  
  #ifdef ADAU144x
    AIDA_SW_SAFELOAD_WRITE_REGISTER(dspAddress, address++, false, buffer);		 // mask
    AIDA_SW_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, value);	 // increment
    AIDA_SW_SAFELOAD_WRITE_VALUE(dspAddress, address, true, 1.0);	 	 // ison
  #else
    AIDA_SAFELOAD_WRITE_REGISTER(dspAddress, address++, false, buffer);		 // mask
    AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, value);	 // increment
    AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address, true, 1.0);	 	 // ison
  #endif
}

/**
 * This function controls a sawtooth source cell
 * @param dspAddress - the physical I2C address (7-bit format)
 * @param address - the param address of the cell
 * @param frequency - the desired frequency for the signal
 */
void sawtooth_source(uint8_t dspAddress, uint16_t address, float frequency)
{
	float value = (0.50/24000.00)*frequency;
	
  #ifdef ADAU144x
    AIDA_SW_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, value);	 // increment
    AIDA_SW_SAFELOAD_WRITE_VALUE(dspAddress, address, true, 1.0);	 	 // ison
  #else
    AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, value);	 // increment
    AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address, true, 1.0);	 	 // ison
  #endif
}

/**
 * This function controls a square source cell
 * @param dspAddress - the physical I2C address (7-bit format)
 * @param address - the param address of the cell
 * @param frequency - the desired frequency for the signal
 */
void square_source(uint8_t dspAddress, uint16_t address, float frequency)
{
	sine_source(dspAddress, address, frequency);	// same as sine source
}

/**
 * This function controls a triangle source cell
 * @param dspAddress - the physical I2C address (7-bit format)
 * @param address - the param address of the cell
 * @param frequency - the desired frequency for the signal
 */
void triangle_source(uint8_t dspAddress, uint16_t address, float frequency)
{
	float value = (0.50/24000.00)*frequency;
	uint8_t buffer[4]={0x00, 0x00, 0x00, 0x03};
	
  #ifdef ADAU144x
    AIDA_SW_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, 0.00);	// Triangle algorithm
    AIDA_SW_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, 1.00);
    AIDA_SW_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, 0.00);
    AIDA_SW_SAFELOAD_WRITE_VALUE(dspAddress, address++, true, -1.00);
    AIDA_SW_SAFELOAD_WRITE_REGISTER(dspAddress, address++, false, buffer);		 // mask
    AIDA_SW_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, value);	 // increment
    AIDA_SW_SAFELOAD_WRITE_VALUE(dspAddress, address, true, 1.0);	 	 // ison
  #else
    AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, 0.00);	// Triangle algorithm
    AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, 1.00);
    AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, 0.00);
    AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, true, -1.00);
    AIDA_SAFELOAD_WRITE_REGISTER(dspAddress, address++, false, buffer);		 // mask
    AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, value);	 // increment
    AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address, true, 1.0);	 	 // ison
  #endif
}

/**
 * This function controls a delay cell
 * @param dspAddress - the physical I2C address (7-bit format)
 * @param address - the param address of the cell
 * @param delay - ranges: 
 *    0.0-42.6ms (ADAU170x) @ 48kHz
 *    0.0-21.3ms (ADAU170x) @ 96kHz
 *    0.0-10.6ms (ADAU170x) @ 192kHz
 *    0.0-170.6ms (ADAU140x) @ 48kHz
 *    0.0-85.3ms (ADAU140x) @ 96kHz
 *    0.0-42.6ms (ADAU140x) @ 192kHz
 * WARNING!!! Delays calculated are theoretical assuming you have 100% data memory available
 * in your Sigma Studio design. Data memory is shared among other blocks in Sigma Studio so
 * in practice, this much data memory is not available to the user
 * because every block in a design uses a few data memory locations
 * for its processing. The SigmaStudio compiler
 * manages the data RAM and indicates if the number of addresses
 * needed in the design exceeds the maximum available.
 */
void delayCell(uint8_t dspAddress, uint16_t address, float delay)
{
  uint32_t ticks = 0;
  uint8_t data[4];
  
  ticks = (uint32_t)(delay*0.001/(1/FS));
  
  data[0] = (ticks>>24)&0xFF; // MSB First
  data[1] = (ticks>>16)&0xFF;
  data[2] = (ticks>>8)&0xFF;
  data[3] = ticks&0xFF; 
  
  #ifdef ADAU170x
    if(ticks>2048)
      ticks=2048;
    AIDA_SAFELOAD_WRITE_REGISTER(dspAddress, address, true, data);    
  #else // ADAU144x
    if(ticks>8192)
      ticks=8192;
    AIDA_SW_SAFELOAD_WRITE_REGISTER(dspAddress, address, true, data);
  #endif
}

/************************************************
 *  PRIVATE FUNCTIONS DEFINITIONS (DO NOT EDIT)	*          
 ************************************************/
void float_to_fixed(float value, uint8_t *buffer)  
{
  int32_t fixedval = 0;
  
  fixedval = FLOAT_TO_FIXED(value);
  buffer[0] = (fixedval>>24)&0xFF;
  buffer[1] = (fixedval>>16)&0xFF;
  buffer[2] = (fixedval>>8)&0xFF;
  buffer[3] = fixedval&0xFF; 
}

void print_fixed_number(int32_t fixedval)
{
	uint8_t c = 0, i;
	uint8_t buffer[4];
	
	Serial.print(F("Fixed val: 0x"));
	buffer[0] = (fixedval>>24)&0xFF;
	buffer[1] = (fixedval>>16)&0xFF;
	buffer[2] = (fixedval>>8)&0xFF;
	buffer[3] = fixedval&0xFF;
	
	for(i=0;i<4;i++)
	{
		c = buffer[i];
		if(c&0xF0)
			Serial.print(c, HEX);
		else
		{
			Serial.print(0, HEX);
			Serial.print(c, HEX);
		}
	}
	Serial.print(F("\n\r"));
}

/************************************************
 *    AIDA LOW LEVEL FUNCTIONS (do not edit)    *
 ************************************************/
void AIDA_WRITE_REGISTER(uint8_t dspAddress, uint16_t address, uint8_t length, uint8_t *data)  
{
  uint8_t i;
  byte LSByte = 0x00;
  byte MSByte = 0x00;
  byte res = 0x00;

  WIRE.beginTransmission(dspAddress);  // Begin write
  LSByte = (byte)address & 0xFF;
  MSByte = address >> 8;
  WIRE.write(MSByte);             // Sends High Address
  WIRE.write(LSByte);             // Sends Low Address

  for(i=0;i<length;i++)
  {
    WIRE.write((byte)data[i]);             // sends bytes 
  }
  WIRE.endTransmission(true);     // Write out data to I2C and stop transmitting
}

void AIDA_WRITE_REGISTER_BLOCK(uint8_t dspAddress, uint16_t address, uint16_t length, const uint8_t *data)
{ 
  uint16_t res = 0;

  WIRE.beginTransmission(dspAddress);  // Begin write

  res = WIRE.writeBlock(data, length, address);
  
  WIRE.endTransmission(true);     // Write out data to I2C and stop transmitting
}

void AIDA_WRITE_VALUE(uint8_t dspAddress, uint16_t address, float value)
{
  uint8_t buf[4];

  float_to_fixed(value, buf);

  AIDA_WRITE_REGISTER(dspAddress, address, 4, buf);
}

void AIDA_WRITE_VALUE28(uint8_t dspAddress, uint16_t address, uint32_t value)
{
  uint8_t buf[4];
  
  value&=0x0FFFFFFF; // Four bits don't care since I have 28 bits and 32 bits data
  
  buf[0] = value>>24;
  buf[1] = (value&0x00FFFFFF)>>16;
  buf[2] = (value&0x0000FFFF)>>8;
  buf[3] = value&0x000000FF;
  
  AIDA_WRITE_REGISTER(dspAddress, address, 4, buf);
}

void AIDA_SAFELOAD_WRITE_REGISTER(uint8_t dspAddress, uint16_t address, boolean finish, uint8_t *data)
{
	uint8_t buf[5];
	
	buf[0] = (address>>8)&0xFF;
	buf[1] = address&0xFF;
	AIDA_WRITE_REGISTER(dspAddress, 0x0815+safeload_count, 2, buf); // load safeload address 0
	// Q: Why are the safeload registers five bytes long, while I'm loading four-byte parameters into the RAM using these registers?
	// A: The safeload registers are also used to load the slew RAM data, which is five bytes long. For parameter RAM writes using safeload, 
	// the first byte of the safeload register can be set to 0x00.
	buf[0] = 0;
	buf[1] = data[0];
	buf[2] = data[1];
	buf[3] = data[2];
	buf[4] = data[3];
	AIDA_WRITE_REGISTER(dspAddress, 0x0810+safeload_count, 5, buf);  // load safeload data 0
	
  safeload_count++;
	if(finish == true || safeload_count == 5)  // Max 5 safeload memory registers
	{
		buf[0] = 0x00;
		buf[1] = 0x3C;
		AIDA_WRITE_REGISTER(dspAddress, 0x081C, 2, buf);  //  IST (initiate safeload transfer bit)
		safeload_count = 0;
	}
}

void AIDA_SAFELOAD_WRITE_VALUE(uint8_t dspAddress, uint16_t address, boolean finish, float value)
{
  uint8_t buf[5];

  buf[0] = (address>>8)&0xFF;
  buf[1] = address&0xFF;
  AIDA_WRITE_REGISTER(dspAddress, 0x0815+safeload_count, 2, buf); // load safeload address 0
  // Q: Why are the safeload registers five bytes long, while I'm loading four-byte parameters into the RAM using these registers?
  // A: The safeload registers are also used to load the slew RAM data, which is five bytes long. For parameter RAM writes using safeload, 
  // the first byte of the safeload register can be set to 0x00.
  buf[0] = 0;
  float_to_fixed(value, &buf[1]);
  AIDA_WRITE_REGISTER(dspAddress, 0x0810+safeload_count, 5, buf);  // load safeload data 0

  safeload_count++;
  if(finish == true || safeload_count == 5)  // Max 5 safeload memory registers
  {
    buf[0] = 0x00;
    buf[1] = 0x3C;
    AIDA_WRITE_REGISTER(dspAddress, 0x081C, 2, buf);  //  IST (initiate safeload transfer bit)
    safeload_count = 0;
  } 
}

void AIDA_SW_SAFELOAD_WRITE_REGISTER(uint8_t dspAddress, uint16_t address, boolean finish, uint8_t *data)
{
	uint8_t i, buf[4];
	uint32_t value32b = 0;
	uint16_t value16b = 0;

	AIDA_WRITE_REGISTER(dspAddress, 0x0001+sw_safeload_count, 4, data);  //  Write values in 0x0001-0x0005
	
	if(sw_safeload_count==0) // Note that multi-word safeload writes can only be accomplished when the target addresses are sequential.
	{
		value16b = address;
		value32b = value16b-1; 
		buf[0] = (value32b>>24)&0xFF; // MSB first
		buf[1] = (value32b>>16)&0xFF;
		buf[2] = (value32b>>8)&0xFF;
		buf[3] = (value32b)&0xFF;
		AIDA_WRITE_REGISTER(dspAddress, 0x0006, 4, buf);  //  Write destination address-1 in 0x0006
	}
	
  sw_safeload_count++;
	if(finish == true || sw_safeload_count == 5)  // Max 5 safeload memory registers
	{
		value32b = sw_safeload_count+1;
		buf[0] = (value32b>>24)&0xFF; // MSB first
		buf[1] = (value32b>>16)&0xFF;
		buf[2] = (value32b>>8)&0xFF;
		buf[3] = (value32b)&0xFF;
		AIDA_WRITE_REGISTER(dspAddress, 0x0007, 4, buf);  //  Write nvalues in 0x0007, start Safeload Write
		sw_safeload_count = 0; // Reset counter		
	}
}

void AIDA_SW_SAFELOAD_WRITE_VALUE(uint8_t dspAddress, uint16_t address, boolean finish, float value)
{
	uint8_t i, buf[4];
	uint32_t value32b = 0;
	uint16_t value16b = 0;

	float_to_fixed(value, buf);
	AIDA_WRITE_REGISTER(dspAddress, 0x0001+sw_safeload_count, 4, buf);  //  Write values in 0x0001-0x0005
	
	if(sw_safeload_count==0) // Note that multi-word safeload writes can only be accomplished when the target addresses are sequential.
	{
		value16b = address;
		value32b = value16b-1; 
		buf[0] = (value32b>>24)&0xFF; // MSB first
		buf[1] = (value32b>>16)&0xFF;
		buf[2] = (value32b>>8)&0xFF;
		buf[3] = (value32b)&0xFF;
		AIDA_WRITE_REGISTER(dspAddress, 0x0006, 4, buf);  //  Write destination address-1 in 0x0006
	}
	
  sw_safeload_count++;
	if(finish == true || sw_safeload_count == 5)  // Max 5 safeload memory registers
	{
		value32b = sw_safeload_count;
		buf[0] = (value32b>>24)&0xFF; // MSB first
		buf[1] = (value32b>>16)&0xFF;
		buf[2] = (value32b>>8)&0xFF;
		buf[3] = (value32b)&0xFF;
		AIDA_WRITE_REGISTER(dspAddress, 0x0007, 4, buf);  //  Write nvalues in 0x0007, start Safeload Write
		sw_safeload_count = 0; // Reset counter		
	}
}

void AIDA_SW_SAFELOAD_WRITE_VALUES(uint8_t dspAddress, uint16_t address, uint8_t nvalues, float *values)
{
	uint8_t i, buf[4];
	uint32_t value32b = 0;
	uint16_t value16b = 0;

	for(i=0;i<nvalues;i++)
	{
		float_to_fixed(values[i], buf);
		AIDA_WRITE_REGISTER(dspAddress, 0x0001+i, 4, buf);  //  Write values in 0x0001-0x0005 
		if(i==5)
			break;
	}
	value16b = address;
	value32b = value16b-1; 
	buf[0] = (value32b>>24)&0xFF; // MSB first
	buf[1] = (value32b>>16)&0xFF;
	buf[2] = (value32b>>8)&0xFF;
	buf[3] = (value32b)&0xFF;
	AIDA_WRITE_REGISTER(dspAddress, 0x0006, 4, buf);  //  Write destination address-1 in 0x0006
	value32b = nvalues;
	buf[0] = (value32b>>24)&0xFF; // MSB first
	buf[1] = (value32b>>16)&0xFF;
	buf[2] = (value32b>>8)&0xFF;
	buf[3] = (value32b)&0xFF;
	AIDA_WRITE_REGISTER(dspAddress, 0x0007, 4, buf);  //  Write nvalues in 0x0007
}

void AIDA_READ_REGISTER(uint8_t dspAddress, uint16_t address, uint8_t length, uint8_t *data)
{
  uint8_t index = 0;
  byte LSByte = 0x00;
  byte MSByte = 0x00;

  #ifdef __AVR__
  WIRE.beginTransmission(dspAddress);  // Begin write

  // Send the internal address I want to read
  LSByte = (byte)address & 0xFF;
  MSByte = address >> 8;
  WIRE.write(MSByte);             // Sends High Address
  WIRE.write(LSByte);             // Sends Low Address

  WIRE.endTransmission(false);    // Write out data to I2C but don't send stop condition on I2C bus
  
  WIRE.requestFrom(dspAddress, length);    // request n bytes from slave device 
  #else
  WIRE.requestFromReg16(dspAddress, address, length, true); // Arduino 2 doesn't support restart natively O.o
  #endif
  
  while(WIRE.available())         // slave may send less than requested
  { 
    data[index++] = WIRE.read();  // receive a byte as character
  }
}

