/*
  AidaDSP.cpp - Aida DSP library
 Copyright (c) 2015 Massimo Pennazio.  All right reserved.
 
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
 
#include "AidaDSP.h"

#define FULLRANGEVAL 4096
#define MIDDLEVAL (4096.00/2)

#ifdef ENC_RES_X4
#define N_ENC 24*4 // We manage the quadrature encoder with x4 resolution so 24*4 every turn.
#else
#define N_ENC 24
#endif
#define MAX_PULSES_ROUGH  N_ENC*1
//#define MAX_PULSES_FINE  N_ENC*10
#define MAX_PULSES_FINE  N_ENC*25

// PRIVATE VARIABLES
// ENCODER
volatile int32_t Pulses = 0;  // Modified in interrupt, so declared as volatile
volatile uint8_t chA = 0, chB = 0;
volatile uint8_t curr_state = 0x00;
volatile uint8_t prev_state = 0x00;
int32_t max_number_of_pulses = MAX_PULSES_ROUGH; // rough regulation default


// PUBLIC FUNCTIONS DEFINITIONS	
		  
void set_regulation_precision(uint8_t fine)
{
  if(fine)
    max_number_of_pulses = MAX_PULSES_FINE;
  else
    max_number_of_pulses = MAX_PULSES_ROUGH;
}

uint8_t get_regulation_precision(void)
{
  if(max_number_of_pulses == MAX_PULSES_FINE)
    return 1; // fine regulation
  else
    return 0; // rough regulation 
}

// This function manage the value of the encoder for user control of Sigma DSP parameters
float processencoder(float minval, float maxval, int32_t pulses)
{
  float val = 0.00;
  
  if(minval < 0)
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
  else // minval >= 0
  {
    if(pulses >= 0)
    {
      if(pulses == 0)
        return minval;
      else // pulses > 0
      {
        val = (pulses*((maxval-minval)/max_number_of_pulses))+minval;
		if(val > maxval)
		  return maxval;
		else
		  return val;
      }
    }
  }
}

// The behaviour of this function is the same of selector with pot with the difference here we do not need to specify n bits of the selector
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

// This function manage the value of a pot for user control of Sigma DSP parameters
float processpot(float minval, float maxval, uint16_t potval)
{
  if(minval < 0)
  {
    if(potval >= MIDDLEVAL)
      return ((potval-MIDDLEVAL)*(maxval/MIDDLEVAL));
    else
      return ((MIDDLEVAL-potval)*(minval/MIDDLEVAL));
  } 
  else
  {
    return (((potval*((maxval-minval)/FULLRANGEVAL)))+minval);
  }
}

uint16_t selectorwithpot(uint16_t potval, uint8_t bits)
{
  uint16_t result = 1;
  
  if(bits>0 && bits<(12+1))
	result = potval >> (12-bits);

  return result;
}

uint8_t isinrange(int16_t value, int16_t reference, int16_t threshold)
{
  if(value < (reference+threshold) && value > (reference-threshold))
  {
    return 1;
  }
  else 
    return 0;
}

void InitAida(void)
{
  pinMode(ENC_PUSH, INPUT_PULLUP);
  pinMode(ENCA, INPUT_PULLUP);
  pinMode(ENCB, INPUT_PULLUP);
  #ifdef ENC_RES_X1
    attachInterrupt(ENCA, enc_manager, RISING);  
  #else 
    #ifdef ENC_RES_X2
      attachInterrupt(ENCB, enc_manager, RISING);  
      attachInterrupt(ENCA, enc_manager, RISING);  
    #else
      attachInterrupt(ENCB, enc_manager, CHANGE);  
      attachInterrupt(ENCA, enc_manager, CHANGE);  
    #endif
  #endif
  
  pinMode(SBOOT, OUTPUT);
  pinMode(RESET, OUTPUT);
 
  digitalWrite(SBOOT, LOW);  // Self-boot: DSP in uC control mode
  digitalWrite(RESET, LOW);  // Hold DSP in RESET state (halted) 

  Wire.setModule(0, I2C_SPEED_FASTMODE); // use 3rd i2c peripheral module in fast-mode (400kHz i2c clk)
  Wire.begin(); // join i2c bus (address optional for master)
  delay(10); 
}

// Interrupt for encoder management
void enc_manager(void)
{
  uint8_t change = 0;
  
  chA = digitalRead(ENCA);
  chB = digitalRead(ENCB);
  
  prev_state = curr_state;
  curr_state = (chA<<1 | chB);		// 2-bit state
  
  #ifdef ENC_RES_X1
    if(chB == 1 && chA == 0)
      Pulses--;
    else
      Pulses++;
  #endif
  #ifdef ENC_RES_X4
  //Entered a new valid state.
    if (((curr_state ^ prev_state) != INVALID) && (curr_state != prev_state))
    {
      //2 bit state. Right hand bit of prev XOR left hand bit of current
      //gives 0 if clockwise rotation and 1 if counter clockwise rotation.
      change = (prev_state & PREV_MASK) ^ ((curr_state & CURR_MASK) >> 1);
      if (change == 0)
        Pulses++;
      else
        Pulses--;
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

void gainCell(uint8_t dspAddress, uint16_t address, float value)
{
	AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address, false, value);
}

/*
This function manage a mono volume control 
 */
void MasterVolumeMono(uint8_t dspAddress, uint16_t address, float value)
{
  AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address, false, value);
}

/*
This function manage a stereo volume control 
 */
void MasterVolumeStereo(uint8_t dspAddress, uint16_t address, float value)
{
  AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, value);
  AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address, true, value);
}

/*
This function manage a 1st order equalizer of two: low pass and high pass
 */
void EQ1stOrd(uint8_t dspAddress, uint16_t address, equalizer_t* equalizer){

  float w0,gainLinear;
  float b0,b1,a1;
  float coefficients[3];

  w0=2*pi*equalizer->f0/FS;		          //2*pi*f0/FS
  gainLinear = pow(10,(equalizer->gain/20));      //10^(gain/20)

  if(equalizer->type == Lowpass){
    a1 = pow(2.7,-w0);
    b0 = gainLinear * (1.0 - a1);
    b1 = 0;
  }
  if(equalizer->type == Highpass){
    a1 = pow(2.7,-w0);
    b0 = gainLinear * a1;
    b1 = -a1 * gainLinear;
  }

  if(equalizer->onoff == true){
    if(equalizer->phase == true)
    {
      coefficients[0] = b0;
      coefficients[1] = b1;
      coefficients[2] = a1;			
    }
    else	// !!!Warning!!! In Sigma Studio parameters don't change when phase is changed
    {
      coefficients[0] = b0;
      coefficients[1] = b1;
      coefficients[2] = a1;
    }
  }
  else
  {
    coefficients[0] = 1.00;
    coefficients[1] = 0.00;
    coefficients[2] = 0.00;
  }

  // Write coefficients to Sigma DSP
  AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, coefficients[0]);
  AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, coefficients[1]);
  AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address, false, coefficients[2]);

  //Serial.write('\n'); //!!!Debug!!!
  //Serial.println(coefficients[0], 3);
  //Serial.println(coefficients[1], 3);
  //Serial.println(coefficients[2], 3);
}

/*
This function manage a 2nd order equalizer of many types: low pass, parametric...and so on
 */
void EQ2ndOrd(uint8_t dspAddress, uint16_t address, equalizer_t* equalizer){

  float A,w0,alpha,gainLinear;
  float b0,b1,b2,a0,a1,a2;
  float coefficients[4];
    
  A=pow(10,(equalizer->boost/40));                //10^(boost/40)
  w0=2*pi*equalizer->f0/FS;		          //2*pi*f0/FS
  gainLinear = pow(10,(equalizer->gain/20));      //10^(gain/20)

  if(equalizer->type==Parametric || equalizer->type==Peaking){       // Peaking filter is a Parametric filter with fixed Q???
    alpha = sin(w0)/(2*equalizer->Q); 
    a0 =  1 + alpha/A;
    a1 = -2 * cos(w0);
    a2 =  1 - alpha/A;
    b0 = (1 + alpha*A) * gainLinear;
    b1 = -(2 * cos(w0)) * gainLinear;
    b2 = (1 - alpha*A) * gainLinear;
  }
  else if(equalizer->type==LowShelf){
    alpha=sin(w0)/2*sqrt((A+1/A)*(1/equalizer->S-1)+2);
    a0 =  (A+1)+(A-1)*cos(w0)+2*sqrt(A)*alpha;
    a1 = -2*((A-1)+(A+1)*cos(w0));
    a2 =  (A+1)+(A-1)*cos(w0)-2*sqrt(A)*alpha;
    b0 = A*((A+1)-(A-1)*cos(w0)+2*sqrt(A)*alpha)*gainLinear;
    b1 = 2*A*((A-1)-(A+1)*cos(w0)) * gainLinear;
    b2 = A*((A+1)-(A-1)*cos(w0)-2*sqrt(A)*alpha)*gainLinear;
  }
  else if(equalizer->type==HighShelf){
    alpha = sin(w0)/2 * sqrt( (A + 1/A)*(1/equalizer->S - 1) + 2 ); 
    a0 =       (A+1) - (A-1)*cos(w0) + 2*sqrt(A)*alpha;
    a1 =   2*( (A-1) - (A+1)*cos(w0) );
    a2 =       (A+1) - (A-1)*cos(w0) - 2*sqrt(A)*alpha;
    b0 =   A*( (A+1) + (A-1)*cos(w0) + 2*sqrt(A)*alpha ) * gainLinear;
    b1 = -2*A*( (A-1) + (A+1)*cos(w0) ) * gainLinear;
    b2 =   A*( (A+1) + (A-1)*cos(w0) - 2*sqrt(A)*alpha ) * gainLinear;
  }
  else if(equalizer->type==Lowpass){
    alpha = sin(w0)/(2*equalizer->Q);
    a0 =   1 + alpha;
    a1 =  -2*cos(w0);
    a2 =   1 - alpha;
    b0 =  (1 - cos(w0)) * (gainLinear/2);
    b1 =   1 - cos(w0)  * gainLinear;
    b2 =  (1 - cos(w0)) * (gainLinear/2);
  }
  else if(equalizer->type==Highpass){
    alpha = sin(w0)/(2*equalizer->Q);
    a0 =   1 + alpha;
    a1 =  -2*cos(w0);
    a2 =   1 - alpha;
    b0 =  (1 + cos(w0)) * (gainLinear/2);
    b1 = -(1 + cos(w0)) * gainLinear;
    b2 =  (1 + cos(w0)) * (gainLinear/2);
  }	
  else if(equalizer->type==Bandpass){
    alpha = sin(w0) * sinh(log(2)/(2 * equalizer->bandwidth * w0/sin(w0)));
    a0 =   1 + alpha;
    a1 =  -2*cos(w0);
    a2 =   1 - alpha;
    b0 =   alpha * gainLinear;
    b1 =   0;
    b2 =  -alpha * gainLinear;
  }
  else if(equalizer->type==Bandstop){
    alpha = sin(w0) * sinh( log(2)/(2 * equalizer->bandwidth * w0/sin(w0)));
    a0 =   1 + alpha;
    a1 =  -2*cos(w0);
    a2 =   1 - alpha;
    b0 =   1 * gainLinear;
    b1 =  -2*cos(w0) * gainLinear;  
    b2 =   1 * gainLinear;
  }
  else if(equalizer->type==ButterworthLP){
    alpha = sin(w0) / 2.0 * 1/sqrt(2);
    a0 =   1 + alpha;
    a1 =  -2*cos(w0);
    a2 =   1 - alpha;
    b0 =  (1 - cos(w0)) * gainLinear / 2;
    b1 =   1 - cos(w0)  * gainLinear;
    b2 =  (1 - cos(w0)) * gainLinear / 2;
  }
  else if(equalizer->type==ButterworthHP){
    alpha = sin(w0) / 2.0 * 1/sqrt(2);
    a0 =   1 + alpha;
    a1 =  -2*cos(w0);
    a2 =   1 - alpha;
    b0 = (1 + cos(w0)) * gainLinear / 2;
    b1 = -(1 + cos(w0)) * gainLinear;
    b2 = (1 + cos(w0)) * gainLinear / 2;
  }
  else if(equalizer->type==BesselLP){
    alpha = sin(w0) / 2.0 * 1/sqrt(3) ;
    a0 =   1 + alpha;
    a1 =  -2*cos(w0);
    a2 =   1 - alpha;
    b0 =  (1 - cos(w0)) * gainLinear / 2;
    b1 =   1 - cos(w0)  * gainLinear;
    b2 =  (1 - cos(w0)) * gainLinear / 2;
  }
  else if(equalizer->type==BesselHP){
    alpha = sin(w0) / 2.0 * 1/sqrt(3) ;
    a0 =   1 + alpha;
    a1 =  -2*cos(w0);
    a2 =   1 - alpha;
    b0 = (1 + cos(w0)) * gainLinear / 2;
    b1 = -(1 + cos(w0)) * gainLinear;
    b2 = (1 + cos(w0)) * gainLinear / 2;
  } 
  
  // For Sigma DSP implementation we need to normalize all the coefficients respect to a0
  // and inverting a1 and a2 inverting by sign 
  if(a0 != 0.00 && equalizer->boost != 0 && equalizer->onoff == true)
  {
    if(equalizer->phase == true)
    {
      coefficients[0]=b0/a0;
      coefficients[1]=b1/a0;
      coefficients[2]=b2/a0;
      coefficients[3]=-1*a1/a0;
      coefficients[4]=-1*a2/a0;
    }
    else
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
  AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, coefficients[0]);
  AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, coefficients[1]);
  AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, coefficients[2]);
  AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, coefficients[3]);
  AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address, false, coefficients[4]);
  
  /*Serial.write('\n'); //!!!Debug!!!
  Serial.println(coefficients[0], 3);
  Serial.println(coefficients[1], 3);
  Serial.println(coefficients[2], 3);
  Serial.println(coefficients[3], 3);
  Serial.println(coefficients[4], 3);*/
}

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
  // and inverting a1 and a2 inverting by sign 
  if(a0 != 0.00 && toneCtrl->onoff == true)
  {
    if(toneCtrl->phase == true)
    {
      coefficients[0]=b0/a0;
      coefficients[1]=b1/a0;
      coefficients[2]=b2/a0;
      coefficients[3]=-1*a1/a0;
      coefficients[4]=-1*a2/a0;
    }
    else
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
  AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, coefficients[0]);
  AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, coefficients[1]);
  AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, coefficients[2]);
  AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, coefficients[3]);
  AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address, false, coefficients[4]);
}

// Frequency range: 1-19148 Hz
// q range:			1.28:10			
void StateVariable(uint8_t dspAddress, uint16_t address, float frequency, float q){

  float param1 = 0.00, param2 = 0.00;
	
  param1 = 2*sin(pi*frequency/FS);	
  param2 = 1/q;
	
  // Write parameters to Sigma DSP
  AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, param1);
  AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, param2);
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

/*
This function calculates the curve and the other parameters of a compressor rms block in Sigma Studio and send them over I2C
 dspAddress: device physical addr
 address: starting address for a compressor block
 compressor_t: struct containing classic compressor parameters
 thresold: [dB]
 ratio: compressor's ratio. If ratio = 6, gain = 1/ratio = 1/6 so compression ratio 6:1
 attack: range 1-500 [ms] 
 hold: range 1-attack [ms]
 decay: range 868-2000 [ms]
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
    AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, curve[i]);
  }
  // Conversion dbps -> ms
  // dbps = 121;
  // TCms = (20/(dbps*2.3))*1000

  // RMS TC (db/s)
  dbps = (20/(compressor->attack*2.3))*1e3; 
  attack_par = abs(1.0 - pow(10,(dbps/(10*FS))));
  AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, true, attack_par);

#ifdef COMPRESSORWITHPOSTGAIN
  postgain_par = pow(10, compressor->postgain/40);
  AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, true, postgain_par);
#endif

  // Hold
  hold_par = compressor->hold*FS/1000;  
  AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, true, hold_par);

  // Decay (db/s)
  dbps = (20/(compressor->decay*2.3))*1e3;
  decay_par = dbps/(96*FS); 
  AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address, true, decay_par);

}

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
    AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, curve[i]);
  }

#ifdef COMPRESSORWITHPOSTGAIN
  postgain_par = pow(10, compressor->postgain/40);
  AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, true, postgain_par);
#endif

  // Hold
  hold_par = compressor->hold*FS/1000;  
  AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, true, hold_par);

  // Decay (db/s)
  dbps = (20/(compressor->decay*2.3))*1e3;
  decay_par = dbps/(96*FS); 
  AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address, true, decay_par);
}

// Read audio level signal in 5.19 fixed point format
// from ReadBack block in Sigma Studio (dbl)
// and returns in dB
void readBack(uint8_t dspAddress, uint16_t address, uint16_t capturecount, float *value){

  uint8_t buf[3];
  
  int32_t word32 = 0;

  buf[0] = capturecount >> 8;
  buf[1] = (uint8_t)capturecount & 0xFF;
  AIDA_WRITE_REGISTER(dspAddress, address, 2, buf);    // readBack operation 

  memset(buf, 0, 3);

  AIDA_READ_REGISTER(dspAddress, address, 3, buf);

  word32 = (buf[0]<<24 | buf[1]<<16 | buf[2]<<8)&0xFFFFFF00; // MSB first, convert to 5.27 format

  if(word32==0)     // Do not calculate log of 0! Unless you want to deal with huge negative numbers!
    word32 = 1; // Absolute minimum, when word32 value is 0x01, value = 1.49011612e-8

  *value = ((float)word32/(1 << 27)); // I'm converting to 5.27 to maintain sign
}

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

void hard_clip(uint8_t dspAddress, uint16_t address, float th_high, float th_low)
{
  uint8_t buffer[12];
  
  float_to_fixed(th_high, &buffer[0]);
  float_to_fixed(th_low, &buffer[4]);
  float_to_fixed(th_low, &buffer[8]); // !!! Write two times BUG of stereo hard clipper section !!! 
  
  AIDA_WRITE_REGISTER(dspAddress, address, 12, buffer);
}

void soft_clip(uint8_t dspAddress, uint16_t address, float alpha)
{
  float onethird = 0.333;
  float twothird = 0.666;
  
  AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, alpha);
  AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, 1/alpha);
  AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, onethird);
  AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, true, twothird);
}

void dc_source(uint8_t dspAddress, uint16_t address, float value)
{
  AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address, true, value);
}

void sine_source(uint8_t dspAddress, uint16_t address, float frequency)
{
	float value = (1.00/24000.00)*frequency;
	uint8_t buffer[4]={0x00, 0x00, 0x00, 0xFF};
	
	AIDA_SAFELOAD_WRITE_REGISTER(dspAddress, address++, false, buffer);		 // mask
	AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, value);	 // increment
	AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address, true, 1.0);	 	 // ison
}

void sawtooth_source(uint8_t dspAddress, uint16_t address, float frequency)
{
	float value = (0.50/24000.00)*frequency;
	
	AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, value);	 // increment
	AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address, true, 1.0);	 	 // ison
}

void square_source(uint8_t dspAddress, uint16_t address, float frequency)
{
	sine_source(dspAddress, address, frequency);	// same as sine source
}

void triangle_source(uint8_t dspAddress, uint16_t address, float frequency)
{
	float value = (0.50/24000.00)*frequency;
	uint8_t buffer[4]={0x00, 0x00, 0x00, 0x03};
	
	AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, 0.00);	// Triangle algorithm
	AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, 1.00);
	AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, 0.00);
	AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, true, -1.00);
	//Delay(1);	// ???Necessary???
	AIDA_SAFELOAD_WRITE_REGISTER(dspAddress, address++, false, buffer); // mask
	AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address++, false, value);	 	// increment
	AIDA_SAFELOAD_WRITE_VALUE(dspAddress, address, true, 1.0);	 	 	// ison
}

// PRIVATE FUNCTIONS DEFINITIONS (DO NOT EDIT)	             

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
void AIDA_WRITE_REGISTER(uint8_t dspAddress, uint16_t address, uint8_t length, uint8_t *data)  
{
  uint8_t i;
  byte LSByte = 0x00;
  byte MSByte = 0x00;
  byte res = 0x00;

  Wire.beginTransmission(dspAddress);  // Begin write
  LSByte = (byte)address & 0xFF;
  MSByte = address >> 8;
  Wire.write(MSByte);             // Sends High Address
  Wire.write(LSByte);             // Sends Low Address

  for(i=0;i<length;i++)
  {
    Wire.write((byte)data[i]);             // sends bytes 
  }
  Wire.endTransmission(true);     // Write out data to I2C and stop transmitting
}

void AIDA_WRITE_REGISTER_BLOCK(uint8_t dspAddress, uint16_t address, uint16_t length, const uint8_t *data)
{ 
  uint16_t res = 0;

  Wire.beginTransmission(dspAddress);  // Begin write

  res = Wire.writeBlock((uint8_t *)data, length, address);

  Wire.endTransmission(true);     // Write out data to I2C and stop transmitting
}

void AIDA_WRITE_VALUE(uint8_t dspAddress, uint16_t address, float value)
{
  uint8_t buf[4];

  float_to_fixed(value, buf);

  AIDA_WRITE_REGISTER(dspAddress, address, 4, buf);

}

void AIDA_SAFELOAD_WRITE_REGISTER(uint8_t dspAddress, uint16_t address, boolean finish, uint8_t *data)
{
	uint8_t buf[5];
	static uint16_t count = 0;
	
	buf[0] = (address>>8)&0xFF;
	buf[1] = address&0xFF;
	AIDA_WRITE_REGISTER(dspAddress, 0x0815+count, 2, buf); // load safeload address 0
	// Q: Why are the safeload registers five bytes long, while I'm loading four-byte parameters into the RAM using these registers?
	// A: The safeload registers are also used to load the slew RAM data, which is five bytes long. For parameter RAM writes using safeload, 
	// the first byte of the safeload register can be set to 0x00.
	buf[0] = 0;
	buf[1] = data[0];
	buf[2] = data[1];
	buf[3] = data[2];
	buf[4] = data[3];
	AIDA_WRITE_REGISTER(dspAddress, 0x0810+count, 5, buf);  // load safeload data 0
	
	if(finish == true || count == 4)  // Max 5 safeload memory registers
	{
		buf[0] = 0x00;
		buf[1] = 0x3C;
		AIDA_WRITE_REGISTER(dspAddress, 0x081C, 2, buf);  //  IST (initiate safeload transfer bit)
		count = 0;
	}
	else
	{ 
		count++;
	}
}

void AIDA_SAFELOAD_WRITE_VALUE(uint8_t dspAddress, uint16_t address, boolean finish, float value)
{
  uint8_t buf[5];
  static uint16_t count = 0;

  buf[0] = (address>>8)&0xFF;
  buf[1] = address&0xFF;
  AIDA_WRITE_REGISTER(dspAddress, 0x0815+count, 2, buf); // load safeload address 0
  // Q: Why are the safeload registers five bytes long, while I'm loading four-byte parameters into the RAM using these registers?
  // A: The safeload registers are also used to load the slew RAM data, which is five bytes long. For parameter RAM writes using safeload, 
  // the first byte of the safeload register can be set to 0x00.
  buf[0] = 0;
  float_to_fixed(value, &buf[1]);
  AIDA_WRITE_REGISTER(dspAddress, 0x0810+count, 5, buf);  // load safeload data 0

  if(finish == true || count == 4)  // Max 5 safeload memory registers
  {
    buf[0] = 0x00;
    buf[1] = 0x3C;
    AIDA_WRITE_REGISTER(dspAddress, 0x081C, 2, buf);  //  IST (initiate safeload transfer bit)
    count = 0;
  }
  else
  { 
    count++;
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
		if(nvalues==5)
			break;
	}
	value16b = address;
	value32b = value16b-1; 
	buf[0] = (value32b>>24)&0xFF; // MSB first
	buf[0] = (value32b>>16)&0xFF;
	buf[0] = (value32b>>8)&0xFF;
	buf[0] = (value32b)&0xFF;
	AIDA_WRITE_REGISTER(dspAddress, 0x0006, 4, buf);  //  Write destination address-1 in 0x0006
	value32b = nvalues;
	buf[0] = (value32b>>24)&0xFF; // MSB first
	buf[0] = (value32b>>16)&0xFF;
	buf[0] = (value32b>>8)&0xFF;
	buf[0] = (value32b)&0xFF;
	AIDA_WRITE_REGISTER(dspAddress, 0x0007, 4, buf);  //  Write nvalues in 0x0007
}
void AIDA_READ_REGISTER(uint8_t dspAddress, uint16_t address, uint8_t length, uint8_t *data)
{
  uint8_t index = 0;
  byte LSByte = 0x00;
  byte MSByte = 0x00;

  Wire.beginTransmission(dspAddress);  // Begin write

  // Send the internal address I want to read
  LSByte = (byte)address & 0xFF;
  MSByte = address >> 8;
  Wire.write(MSByte);             // Sends High Address
  Wire.write(LSByte);             // Sends Low Address

  Wire.endTransmission(false);    // Write out data to I2C but don't send stop condition on I2C bus

  Wire.requestFrom(dspAddress, length);    // request 3 bytes from slave device 

  while(Wire.available())         // slave may send less than requested
  { 
    data[index++] = Wire.read();  // receive a byte as character    
  } 
}

