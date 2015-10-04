/*
 AIDA Tutorial_1 Sketch
 	
 This sketch controls a stereo master volume using the 
 structure of Template1.
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
 
 created November 2014
 by Massimo Pennazio
 
 This example code is in the public domain.
 
 */

#include <Energia.h>
#include <pins_energia.h>
#include <Wire.h>
#include "AidaDSP.h"

#define EVER (;;)

// DEFINES USER INTERFACE
#define STX 0x02
#define ETX 0x03
#define NTX 0xFF
#define PACKET_MIN_SIZE 0x05

#define ON 1
#define OFF 0

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
enum{stx, ndata, address, data, chksum, etx}comstate;
uint16_t addr = 0x0000;
uint8_t nData = 0;
uint8_t comcount = 0;
uint8_t addrMSB = 0x00;
uint8_t addrLSB = 0x00;
uint16_t checksum = 0x00;

void setup()
{
  // put your setup code here, to run once:
  // I/O
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  
  // open the USBSerial port
  Serial.begin(115200);
  Serial.flush();
  comstate = stx;
  
  // DSP board
  InitAida();	// Initialize DSP board

  spettacolino();
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
          comcount++;
          if(nData==comcount)
          {
            comstate = chksum;
          }
          checksum += inByte;
        }
        break;
      case chksum:
        checksum = ~checksum + 1; // Checksum algorithm
        checksum &= 0xFF;
        if(inByte==checksum)
        {
          // Checksum OK
          comstate = etx;
        }
        else
        {
          // Checksum KO
          comstate = stx; // Reset state machine
          digitalWrite(RED_LED, HIGH);
          digitalWrite(BLUE_LED, LOW);
          // Send NACK to PC
          Serial.write(STX);
          Serial.write(NTX);
          Serial.write((char)checksum);
        }
       case etx:
        if(inByte==ETX)
        {
          // Valid Packet
          digitalWrite(BLUE_LED, HIGH);
          digitalWrite(RED_LED, LOW);
          // Send ACK to PC
          Serial.write(STX);
          Serial.write(ETX);
          comstate = stx; // Reset state machine
        }
        break;
    }
  }

  if(digitalRead(ENC_PUSH)==LOW)  
  {
    digitalWrite(GREEN_LED, HIGH);
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
    digitalWrite(GREEN_LED, LOW);
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
  if(timec-prevtimec >= 250)  // Here we manage control interface every 250ms
  {
    
    prevtimec = timec;
  } 
} // End void loop

void spettacolino()
{
  byte i;
  byte count = 0x00;

  for(i=0;i<6;i++)
  {
    count += 1;
    if(count==1)
      digitalWrite(RED_LED, HIGH);   
    else if(count==2)
      digitalWrite(GREEN_LED, HIGH);
    else{
      digitalWrite(BLUE_LED, HIGH);
      count=0;
    }
    delay(250);
    digitalWrite(RED_LED, LOW);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(BLUE_LED, LOW);
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


