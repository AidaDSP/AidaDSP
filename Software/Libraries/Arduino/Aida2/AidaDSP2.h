/*
  AidaDSP.h - Aida DSP library
 Copyright (c) 2016 Massimo Pennazio <maxipenna@libero.it>
 
 Version: 0.20 ADAU144x (Arduino)
 
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

#ifndef _AIDA_DSP_H_
#define _AIDA_DSP_H_

#include <Arduino.h>
#include <pins_arduino.h>
#include <Wire.h>

/************************************************
 *      DEFINES FOR HW USER CONFIGURATION       *
 ************************************************/
#define SBOOT 12
#define RESET 11

#define ENCB 2
#define ENCA 3
#define PUSH1 4
#define ENC_PUSH PUSH1

#define POT4 A3
#define POT3 A2
#define POT2 A1
#define POT1 A0

#define XXXX    POT4
#define XXX     POT3
#define XX      POT2
#define X       POT1
/************************************************
 *     END DEFINES FOR HW USER CONFIGURATION    *
 ************************************************/

/************************************************
 *        ENCODER DEFINES (do not edit)         *
 ************************************************/
#define PREV_MASK	0x1 //Mask for the previous state in determining direction of rotation.
#define CURR_MASK 0x2 //Mask for the current state in determining direction of rotation.
#define INVALID   0x3 //XORing two states where both bits have changed.

#define ENC_RES_X4
//define ENC_RES_X1
//define ENC_RES_X2 // Not managed yet
/************************************************
 *      END ENCODER DEFINES (do not edit)       *
 ************************************************/

/************************************************
 *          FIXED POINT (do not edit)           *
 ************************************************/
#define FIXED_BITS               32
#define FIXED_WBITS               5
#define FIXED_FBITS              23
#define FIXED_TO_INT(a)          ((int32_t)a >> FIXED_FBITS)
#define FIXED_FROM_INT(a)        ((int32_t)a << FIXED_FBITS)
#define FLOAT_TO_FIXED(a)        (a*((int32_t)1 << FIXED_FBITS))
#define FIXED_TO_FLOAT(a)        ((float)a/((int32_t)1 << FIXED_FBITS))

inline int32_t FIXED_Mul(int32_t a, int32_t b)  // This function has to be declared inline for code efficiency
{
  return(((int32_t)a*(int32_t)b) >> FIXED_FBITS);
} 
/***********************************************
 *         END FIXED POINT (do not edit)       *
 ***********************************************/

/************************************************
 *          DSP DEFINES (do not edit)           *
 ************************************************/
#define pi 3.1415926f
#define FS 48000.00f
#define SR (uint32_t)(1.0f/FS)

// 2nd order equalizer defines
#define Peaking         0
#define Parametric      1
#define LowShelf        2
#define HighShelf       3
#define Lowpass         4
#define Highpass        5
#define Bandpass        6
#define Bandstop        7
#define ButterworthLP   8
#define ButterworthHP   9
#define BesselLP       10
#define BesselHP       11

/************************************************
 *          DSP TYPEDEFS (do not edit)          *
 ************************************************/
#define COMPRESSORWITHPOSTGAIN
typedef struct compressor_t{
  float threshold;  // -90/+6 [dB]
  float ratio;      // 1 - 100
  float attack;     // 1 - 500 [ms]
  float hold;       // 1 - attack [ms]
  float decay;      // 2000 [ms]
  float postgain;   // -30/+24 [dB]
}compressor;

typedef struct equalizer_t{
  float Q;          // Parametric, Peaking, range 0-16
  float S;          // Slope (Shelving), range 0-2
  float bandwidth;   // Bandwidth in octaves, range 0-11 
  float gain;       // Range +/-15 [dB]
  float f0;         // Range 20-20000 [Hz]
  float boost;      // Range +/-15 [dB]
  unsigned char type;     // See defines section...
  unsigned char phase;    // 0 or False -> in phase (0°) 1 or True -> 180°
  unsigned char onoff;    // False -> off True -> on
}equalizer;

typedef struct toneCtrl_t{
  float Boost_Bass_dB;
  float Boost_Treble_dB;
  float Freq_Bass;
  float Freq_Treble;
  unsigned char phase;    // 0 or False -> in phase (0°) 1 or True -> 180°    
  unsigned char onoff;    // False -> off True -> on
}toneCtrl;

/************************************************
 *           AIDA HIGH LEVEL FUNCTIONS          *
 ************************************************/
// Utilities
void linspace(float x1, float x2, float n, float vect[]);
void set_regulation_precision(uint8_t fine);
uint8_t get_regulation_precision(void);
void set_regulation_precision2(float precision);
float get_regulation_precision2(void);
float processencoder(float minval, float maxval, int32_t pulses);
float processencoder2(float minval, float maxval);
uint16_t selectorwithencoder(int32_t pulses, uint8_t bits);
float processpot(float minval, float maxval, uint16_t potvalue);
uint16_t selectorwithpot(uint16_t potval, uint8_t bits);
uint8_t isinrange(int16_t value, int16_t reference, int16_t threshold);
void print_fixed_number(int32_t fixedval);

// Setup
void InitAida(void);
void enc_manager(void); // Encoder interrupt function
int32_t getPulses(void);
void setPulses(int32_t value);

// DSP Blocks
void gainCell(uint8_t dspAddress, uint16_t address, float value);
void MasterVolumeMono(uint8_t dspAddress, uint16_t address, float value);
void MasterVolumeStereo(uint8_t dspAddress, uint16_t address, float value);
void EQ1stOrd(uint8_t dspAddress, uint16_t address, equalizer_t* equalizer);
void EQ2ndOrd(uint8_t dspAddress, uint16_t address, equalizer_t* equalizer);
void ToneControl(uint8_t dspAddress, uint16_t address, toneCtrl_t* toneCtrl);
void StateVariable(uint8_t dspAddress, uint16_t address, float frequency, float q);
void CompressorRMS(uint8_t dspAddress, uint16_t address, compressor_t* compressor);
void CompressorPeak(uint8_t dspAddress, uint16_t address, compressor_t* compressor);   
void readBack(uint8_t dspAddress, uint16_t address, uint16_t capturecount, float *value);
void readBack2(uint8_t dspAddress, uint16_t address, float *value);
void mux(uint8_t dspAddress, uint16_t address, uint8_t select, uint8_t nchannels);
void muxnoiseless(uint8_t dspAddress, uint16_t address, uint8_t select);
void hard_clip(uint8_t dspAddress, uint16_t address, float th_high, float th_low);
void soft_clip(uint8_t dspAddress, uint16_t address, float alpha);
void dc_source(uint8_t dspAddress, uint16_t address, float value);
void sine_source(uint8_t dspAddress, uint16_t address, float frequency);
void sawtooth_source(uint8_t dspAddress, uint16_t address, float frequency);
void square_source(uint8_t dspAddress, uint16_t address, float frequency);
void triangle_source(uint8_t dspAddress, uint16_t address, float frequency);
void delayCell(uint8_t dspAddress, uint16_t address, float delay);

/************************************************
 *    AIDA LOW LEVEL FUNCTIONS (do not edit)    *
 ************************************************/
void float_to_fixed(float value, uint8_t *buffer);
void AIDA_WRITE_REGISTER(uint8_t dspAddress, uint16_t address, uint8_t length, uint8_t *data);
void AIDA_WRITE_REGISTER_BLOCK(uint8_t dspAddress, uint16_t address, uint16_t length, const uint8_t *data);
void AIDA_WRITE_VALUE(uint8_t dspAddress, uint16_t address, float value);
void AIDA_WRITE_VALUE28(uint8_t dspAddress, uint16_t address, uint32_t value);
void AIDA_SAFELOAD_WRITE_REGISTER(uint8_t dspAddress, uint16_t address, boolean finish, uint8_t *data);
void AIDA_SAFELOAD_WRITE_VALUE(uint8_t dspAddress, uint16_t address, boolean finish, float value);
void AIDA_SW_SAFELOAD_WRITE_REGISTER(uint8_t dspAddress, uint16_t address, boolean finish, uint8_t *data);
void AIDA_SW_SAFELOAD_WRITE_VALUE(uint8_t dspAddress, uint16_t address, boolean finish, float value);
void AIDA_SW_SAFELOAD_WRITE_VALUES(uint8_t dspAddress, uint16_t address, uint8_t nvalues, float *values);
void AIDA_READ_REGISTER(uint8_t dspAddress, uint16_t address, uint8_t length, uint8_t *data);

#endif

