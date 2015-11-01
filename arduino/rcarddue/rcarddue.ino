/*  This file is for Arduino DUE
 *
 *  Thanks to:
    Martin Korth of the NO$PSX - documented Memory Card protocol.
    Andrew J McCubbin - documented PS1 SPI interface.
    Arduino PS1 Memory Card Reader - MemCARDuino, Shendo 2013

    Connecting a Memory Card to Arduino:
    ------------------------------------
    Looking at the Memory Card:
    _________________
    |_ _|_ _ _|_ _ _|
     1 2 3 4 5 6 7 8

     1 DATA - MISO ( SPI ) on Arduino
     2 CMND - MOSI ( SPI ) on Arduino
     3 7.6V - 5V
     4 GND - GND
     5 3.6V - 5V
     6 ATT -  CS ( SPI ) on Arduino
     7 CLK -  SCK ( SPI ) on Arduino
     8 ACK - External IRQ on Arduino

     If your card still isn't readable and it's a 3rd party card,
     you will need to supply it with 7.6 V extrenal power supply.
*/

#include "Arduino.h"

#include "softspi.h"
#include "serialcmd.h"

//Memory Card Responses
//0x47 - Good
//0x4E - BadChecksum
//0xFF - BadSector

//Define pins
#define MOSI 33
#define MISO 31
#define SCK 39
#define SS 37

#define ACK 41           //Acknowledge            // external IRQ, (All digital pin on Arduino DUE is supported)

// user schema for PSX
#define PSX_ACK  ACK

// SPI setting var
unsigned long SPI_CLOCK = 240000;   // 240 KHz
unsigned long SPI_BYTE_DELAY  =  100000;
SPIpins pins(MOSI, MISO, SCK, SS);
SoftSPISettings sets(SPI_CLOCK, SPI_BYTE_DELAY, LSBFIRST, 0);
SoftSPI sspi(84000000, sets, pins); // due is 84 MHz  // BUG: can only achieve 89 kHz, mem card no response

// LED
void led_setup(){pinMode(13, OUTPUT);}
void led_on(){digitalWrite(13, HIGH);}   // turn the LED on (HIGH is the voltage level)
void led_off(){digitalWrite(13, LOW);}    // turn the LED off by making the voltage LOW

// ISR: set ack flag
boolean f_psx_ack = false;
void psx_ack_isr() {
  f_psx_ack = true;
  sspi.interruptTransferDelay();
}

void psx_spi_setup() {
  // pin mode
  pinMode(PSX_ACK, INPUT);
  // ISR flag: init : no ack received yet 
  f_psx_ack = false;
  attachInterrupt(digitalPinToInterrupt(PSX_ACK), psx_ack_isr, FALLING); 
  sspi.setup();
}

// small sub routine
byte psx_spi_xfer_byte(byte Byte) {
  led_on();
  byte rcvByte = sspi.mode3Transfer(Byte);
  led_off();
  return rcvByte; // return the received byte
}

// data frame buffer

char fb[1 + 10 + 128 + 2 + 8];  // read cmd header + frame data + 2 checksum + 8 byte 0x5C if 3rd party card.
unsigned int fbp, datp;

//Read a frame from Memory Card and send it to serial port
void psx_read_frame(byte AddressMSB, byte AddressLSB)
{
  sspi.beginTransfer();
  
  fbp = 1;
  fb[fbp++] = psx_spi_xfer_byte(0x81);      //Access Memory Card // FF (Error code)
  fb[fbp++] = psx_spi_xfer_byte(0x52);      //Send read command // 00
  fb[fbp++] = psx_spi_xfer_byte(0x00);      //Memory Card ID1  //5A
  fb[fbp++] = psx_spi_xfer_byte(0x00);      //Memory Card ID2  //5D
  fb[fbp++] = psx_spi_xfer_byte(AddressMSB);      //Address MSB //00
  fb[fbp++] = psx_spi_xfer_byte(AddressLSB);      //Address LSB //00
  fb[fbp++] = psx_spi_xfer_byte(0x00);      //Memory Card ACK1  //5C //;<-- late /ACK after this byte-pair
  sspi.delay(SPI_BYTE_DELAY);
  fb[fbp++] = psx_spi_xfer_byte(0x00);      //Memory Card ACK2  //5C
  fb[fbp++] = psx_spi_xfer_byte(0x00);      //Confirm MSB // 5D
  fb[fbp++] = psx_spi_xfer_byte(0x00);      //Confirm LSB // FF

  datp = fbp;
  //Get 128 byte data from the frame
  for (int i = 0; i < 128; i++)
  {
    fb[fbp++] = psx_spi_xfer_byte(0x00);
  }
  fb[fbp++] = psx_spi_xfer_byte(0x00);      //Checksum (MSB xor LSB xor Data)
  fb[fbp++] = psx_spi_xfer_byte(0x00);      //Memory Card status byte
  fb[fbp++] = psx_spi_xfer_byte(0x00);      // 3rd party tail

debug:
  sspi.endTransfer();
}

void readFrameToSerial(byte AddressMSB, byte AddressLSB){
  // read a frame from memory card
  psx_read_frame(AddressMSB, AddressLSB);
  // wite back to serial
  fb[0] = FRAMEDATA;
  for (int i = 0; i < sizeof fb; i++) {
    Serial.write(fb[i]);
  }
}

// Get ID from Memory Card
// ID buffer
char idb[1+10];  // idb[2] -> id 1, idb[3] -> id 2
unsigned int idbp;
void psx_read_id()
{
  sspi.beginTransfer();
  
  idbp = 1;
  idb[idbp++] = psx_spi_xfer_byte(0x81);      //Access Memory Card // FF (Error code)
  idb[idbp++] = psx_spi_xfer_byte(0x53);      //Send get id command // flag
  idb[idbp++] = psx_spi_xfer_byte(0x00);      //Memory Card ID1  //5A
  idb[idbp++] = psx_spi_xfer_byte(0x00);      //Memory Card ID2  //5D
  idb[idbp++] = psx_spi_xfer_byte(0x00);      //Receive command ack//5C
  idb[idbp++] = psx_spi_xfer_byte(0x00);      //Receive command ack//5D
  idb[idbp++] = psx_spi_xfer_byte(0x00);  // 04
  idb[idbp++] = psx_spi_xfer_byte(0x00);  // 00
  idb[idbp++] = psx_spi_xfer_byte(0x00);  // 00
  idb[idbp++] = psx_spi_xfer_byte(0x00);  // 80

debug:
  sspi.endTransfer();
}

void readIdToSerial(){
  psx_read_id();
  // wite back to serial
  idb[0] = CARDID;
  for (int i = 0; i < sizeof idb; i++) {
    Serial.write(idb[i]);
  }
}

void setup()
{
  psx_spi_setup();
  led_setup();
  digitalWrite(13, LOW);    // turn the LED off by making the voltage LOW
  Serial.begin(38400);
}

byte cmdbuf[CMDLEN] = {0};
unsigned cmdlen = 0;

void parseAndExeCmd() {
  switch (cmdbuf[0])
  {
    case READ:
      delay(5); // avoid continus read
      readFrameToSerial(cmdbuf[1], cmdbuf[2]);
      break;

    case SETDELAY:
      SPI_BYTE_DELAY = cmdbuf[1];
      SPI_BYTE_DELAY <<= 8;
      SPI_BYTE_DELAY += cmdbuf[2];
      // ACK
      Serial.write(ACKSETDELAY);
      Serial.write(SPI_BYTE_DELAY >> 8);
      Serial.write(SPI_BYTE_DELAY);
      break;

    case GETID:   // test read frame 0, 0
      readIdToSerial();
      break;

    default:
      Serial.write(UNKNOWNCMD);
      Serial.write(cmdbuf[0]);
      break;
  }
}

void loop()
{ 
  if (Serial.available() > 0)
  {
    cmdbuf[cmdlen++] = Serial.read();

    // process
    if ( cmdlen >= CMDLEN ) {   // protocol: all cmd must be of same length CMDLEN
      parseAndExeCmd();
      memset(cmdbuf, 0 , CMDLEN);
      cmdlen = 0;
    }
  }
}

