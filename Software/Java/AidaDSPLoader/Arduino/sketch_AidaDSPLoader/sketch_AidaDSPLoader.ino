/*
 AIDA DSP Loader Sketch
 	
 This sketch forms in tandem with Java App AidaDSPLoader
 a serial COM programmer which can be called by CMD line or directly
 in Sigma Studio, as explained here: 
 [link]
 This sketch was written for Arduino, and will not work on other boards.
 
 created May 2016
 by Massimo Pennazio All Rights Reserved
 
 AidaDSPLoader code is released under the GNU GPL V3 license
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
  //program_download();    // Here we load program, parameters and hardware configuration to DSP
  //delay(20);
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
        }
        break;
      case ndata:  
        nData = inByte;
        comstate = address;
        checksum += inByte;
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
          dataBytes[comcount]=inByte;
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
            AIDA_WRITE_REGISTER_BLOCK( DEVICE_ADDR_7bit, addr, nData, &dataBytes[0] ); // Write new data to DSP
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

  /*if(digitalRead(ENC_PUSH)==LOW)  
  {
    //digitalWrite(PIN_LED, HIGH);
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
    //digitalWrite(PIN_LED, LOW);
  }*/

  if(function==1)
  {
    mute ^= 1;
  }
  else if(function==2)
  {
    submenu ^=1; 
  }

  /*timec = millis();
  if(timec-prevtimec >= 250)  // Here we manage control interface every 250ms
  {
    
    // Do something!
    //Serial.write(0x55);
    
    prevtimec = timec;
  }*/
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


