/*
 AIDA DSP Loader 2 Sketch
 	
 This sketch forms in tandem with Java App AidaDSPLoader
 a serial COM programmer which can be called by CMD line or directly
 in Sigma Studio, as explained here: 
 https://github.com/AidaDSP/AidaDSP/tree/master/Software/Java
 In this version there is also a minimal interface running after the program
 had been loaded. Also remember to update addresses of blocks used by the sketch
 according to the one defined in Sigma Studio. 
 This sketch was written for Arduino, and will not work on other boards.
 
 created May 2016
 by Massimo Pennazio All Rights Reserved
 
 Aida DSP Loader 2 code is released under the GNU GPL V3 license
 you can download a copy here
 http://www.gnu.org/licenses/lgpl-3.0.txt
 
 */

#include <Arduino.h>
#include <pins_arduino.h>
#include <Wire.h>
#include "AidaDSP.h"

#define EVER (;;)

// DEFINES I/O
#define PIN_LED  13

// DEFINES COM INTERFACE
#define STX 0x02
#define ETX 0x03
#define NTX 0xFF
#define PACKET_MIN_SIZE 0x05

#define ON 1
#define OFF 0

// DEFINES DSP 
#define DEVICE_ADDR 0x6C
#define DEVICE_ADDR_7bit DEVICE_ADDR>>1

#define CoreRegisterR0Addr 	2076
#define CoreRegisterR0Size 	2
const PROGMEM unsigned char CoreRegisterR0Data[2]={0x00, 0x18};

#define CoreRegisterR4Addr 	2076
#define CoreRegisterR4Size 	2
const PROGMEM unsigned char CoreRegisterR4Data[2]={0x00, 0x1C};

#define COMPLETE_PROGRAM_SIZE 9244 // 2 + 5120 + 4096 + 24 + 2

#define SINE_SOURCE_ADDR 8u
#define TRIANGLE_SOURCE_ADDR 11u

// FUNCTION PROTOTYPES
void spettacolino();
void clearAndHome(void);

// GLOBAL VARIABLES
// ENCODER
int32_t OldPulses = 0;

// UI
uint8_t count = 0;
uint8_t function = 0;
uint8_t sw1_f1 = OFF;
uint8_t sw1_f2 = OFF;
uint16_t Pot1 = 0;
uint32_t timec=0, prevtimec=0;
int32_t param1_pulses = 0;
int32_t oldparam1_pulses = 0;
uint8_t param1_value = 0;

// COM TX RX VARIABLES
char inByte = 0x00;
uint8_t send_nack = 0;
enum{stx, ndata, address, data, chksum, etx}comstate;
uint16_t addr = 0x0000;
uint8_t nData = 0;
uint8_t comcount = 0;
uint8_t addrMSB = 0x00;
uint8_t addrLSB = 0x00;
uint16_t checksum = 0x00;
uint8_t dataBytes[24];
uint16_t ProgramByteCounter = 0;
uint8_t ProgramByteCounterLock = 0;
uint8_t NewProgramReady = 0;

void setup()
{
  // put your setup code here, to run once:
  // I/O
  pinMode(PIN_LED, OUTPUT);
  
  // open the USBSerial port
  Serial.begin(115200);
  //Serial.flush();
  comstate = stx;
  
  // DSP board
  InitAida();	// Initialize DSP board
  digitalWrite(RESET, HIGH); // Wake up DSP
  delay(100);  // Start-up delay for DSP
  AIDA_WRITE_REGISTER_BLOCK( DEVICE_ADDR_7bit, CoreRegisterR0Addr, CoreRegisterR0Size, CoreRegisterR0Data ); // Mute DAC immediately after DSP wake up
  //spettacolino();
}

void loop()
{
  // put your main code here, to run repeatedly:
  
  if(Serial.available()>0)
  {
    inByte = Serial.read();
    switch(comstate)
    {
      case stx:
        if(inByte==STX)
        {
          comstate = ndata;
          checksum = 0;
          comcount = 0;
          nData = 0;
          if(ProgramByteCounterLock==0)
          {
            ProgramByteCounter = 0;
            ProgramByteCounterLock = 1; // Lock semaphore, receiving new program
          }
        }
        break;
      case ndata:  
        nData = inByte;
        comstate = address;
        checksum += inByte;
        if(ProgramByteCounterLock!=0)
          ProgramByteCounter+=nData;
        break;
      case address:
        comcount++;
        if(comcount==1)
        {
          addrMSB = inByte;
        }
        else
        {
          addrLSB = inByte;
          addr = ((uint16_t)addrMSB<<8 | addrLSB&0xFF);
          comstate = data;
          comcount = 0;
        }
        checksum += inByte; 
        break;
      case data:
        if(nData==0)
        {
          comstate = chksum;
          comcount = 0;
        }
        else
        {
          dataBytes[comcount]=(uint8_t)inByte;
          comcount++;
          if(nData==comcount)
          {
            comstate = chksum;
          }
        }
        checksum += inByte;
        break;
      case chksum:
        checksum = ~checksum + 1; // Checksum algorithm
        checksum &= 0xFF;
        if(inByte==(char)checksum)
        {
          // Checksum OK
          send_nack = 0; // Send ACK to PC
          //send_nack = 1; // !!!Debug
        }
        else
        {
          // Checksum KO
          send_nack = 1; // Send NACK to PC
        }
        comstate = etx;
        break;
       case etx:
        if(inByte==ETX)
        {
          if(send_nack == 0) // ACK
          {
            digitalWrite(PIN_LED, LOW);
            Serial.write(STX);
            Serial.write(ETX);
            #ifdef __AVR__ // AVR based Arduinos: Uno, Mega, etc...
            AIDA_WRITE_REGISTER( DEVICE_ADDR_7bit, addr, nData, &dataBytes[0] ); // Write new data to DSP 
            #else // Arduino 2
            AIDA_WRITE_REGISTER_BLOCK( DEVICE_ADDR_7bit, addr, nData, &dataBytes[0] ); // Write new data to DSP
            #endif 
            if(ProgramByteCounterLock!=0)
            {
              if(ProgramByteCounter==COMPLETE_PROGRAM_SIZE)
              {
                ProgramByteCounterLock=0; // Unlock semaphore
                NewProgramReady = 1; 
              }
            } 
        }
          
          else // NACK
          {
            digitalWrite(PIN_LED, HIGH);
            Serial.write(STX);
            Serial.write(NTX);
            Serial.write((char)checksum);
          }
          comstate = stx; // Reset state machine
        }
        break;
    }
  }

  if(NewProgramReady)
  {
    if(digitalRead(ENC_PUSH)==LOW)  
    {
      digitalWrite(PIN_LED, HIGH);
      delay(5);  // debounce
      if(digitalRead(ENC_PUSH)==LOW)
      {
        count++;
      }   
    }
    else
    {
      if(count>3 && count<100)
        function = 1;
      else if(count>100 && count<200)
        function = 2;
      else
        function = 0;  // No function triggered on switch
      count = 0;
      digitalWrite(PIN_LED, LOW);
    }

    if(function==1)
    {
      sw1_f1 ^= 1;
    }
    else if(function==2)
    {
      sw1_f2 ^=1; 
    }

    timec = millis();
    if(timec-prevtimec >= 100)  // Here we manage control interface every 100ms
    {
      
      // Do something!
      param1_pulses = getPulses();
      if(param1_pulses != oldparam1_pulses)
      {
        param1_value = selectorwithencoder(param1_pulses, 2);
        switch(param1_value)
        {
          case 1:
            sine_source(DEVICE_ADDR_7bit, SINE_SOURCE_ADDR, 110);
            delayMicroseconds(25);
            triangle_source(DEVICE_ADDR_7bit, TRIANGLE_SOURCE_ADDR, 220);
            break;
          case 2:
            sine_source(DEVICE_ADDR_7bit, SINE_SOURCE_ADDR, 220);
            delayMicroseconds(25);
            triangle_source(DEVICE_ADDR_7bit, TRIANGLE_SOURCE_ADDR, 440);
            break;
          case 3:
            sine_source(DEVICE_ADDR_7bit, SINE_SOURCE_ADDR, 440);
            delayMicroseconds(25);
            triangle_source(DEVICE_ADDR_7bit, TRIANGLE_SOURCE_ADDR, 880);
            break;
          case 4:
            sine_source(DEVICE_ADDR_7bit, SINE_SOURCE_ADDR, 880);
            delayMicroseconds(25);
            triangle_source(DEVICE_ADDR_7bit, TRIANGLE_SOURCE_ADDR, 1760);
            break;
        }
        oldparam1_pulses = param1_pulses;
      }
      
      prevtimec = timec;
    }
  }
} // End void loop

void spettacolino()
{
  byte i;
  byte status = 0x00;

  for(i=0;i<3;i++)
  {
    status ^= 1;
    digitalWrite(PIN_LED, status);
    delay(250);
  }
  digitalWrite(PIN_LED, LOW);
}

void clearAndHome(void)
{
  Serial.write(0x1b); // ESC
  Serial.print(F("[2J")); // clear screen
  Serial.write(0x1b); // ESC
  Serial.print(F("[H")); // cursor to home
}


