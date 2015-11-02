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
#include "SPI.h"

#include "serialcmd.h"

//Memory Card Responses
//0x47 - Good
//0x4E - BadChecksum
//0xFF - BadSector

//Define pins
#define AttPin 24          //Attention (Select)     // SPI SS
#define AckPin 32           //Acknowledge            // external IRQ, (All digital pin on Arduino DUE is supported)

// user schema for PSX
#define PSX_SEL  AttPin
#define PSX_ACK  AckPin

// SPI setting var
unsigned long SPI_SPEED = 250000;   // 200 KHz  // bug: due HW SPI can only as slow as 84M/255 = 329.9KHz
unsigned long SPI_SPEED_DIV = 1;   // speed divider
unsigned long BYTE_DELAY  =   1500; // time of 1500 cycles of 250KHz

// LED
void led_setup(){pinMode(13, OUTPUT);}
void led_on(){digitalWrite(13, HIGH);}   // turn the LED on (HIGH is the voltage level)
void led_off(){digitalWrite(13, LOW);}    // turn the LED off by making the voltage LOW

// ISR: set ack flag
boolean f_psx_ack = false;
void psx_ack_isr() {
  f_psx_ack = true;
}

void psx_spi_setup() {
  // pin mode
  pinMode(PSX_SEL, OUTPUT);
  pinMode(PSX_ACK, INPUT);
  // ISR flag: init : no ack received yet 
  f_psx_ack = false;
  attachInterrupt(digitalPinToInterrupt(PSX_ACK), psx_ack_isr, FALLING); 
  SPI.begin();
}

// small sub routine
byte psx_spi_xfer_byte(byte Byte, unsigned long Delay) {
  led_on();
  byte rcvByte = SPI.transfer(Byte);
  while ( Delay > 0 ) // Poll for the ACK signal from the Memory Card
  {
    Delay--;
    __asm__("nop\n\t"); 
    // test irq
    if ( f_psx_ack ) {
      f_psx_ack = false; // reset irq flag
      break;
    }
  }
  led_off();
  return rcvByte; // return the received byte
}

// data frame buffer

char fb[1 + 10 + 128 + 2 + 1];  //acktype + read cmd header + frame data + 2 checksum + 8 byte 0x5C if 3rd party card.
unsigned int fbp;

//Read a frame from Memory Card and send it to serial port
void psx_read_frame(byte AddressMSB, byte AddressLSB)
{
  // PSX is LSB first, (CPOL=1, PSX clock idle when 1. CPHA=1, PSX host read data at clock up edge)
  SPI.beginTransaction(SPISettings(SPI_SPEED/SPI_SPEED_DIV, LSBFIRST, SPI_MODE3));
  digitalWrite( PSX_SEL, LOW ); //Activate device
  
  fbp = 0;
  fb[fbp++] = FRAMEDATA;  // ACK type: for serial protocol
  
  fb[fbp++] = psx_spi_xfer_byte(0x81, BYTE_DELAY);      //Access Memory Card // N/A (dummy response)
  fb[fbp++] = psx_spi_xfer_byte(0x52, BYTE_DELAY);      //Send read command // FLAG
  fb[fbp++] = psx_spi_xfer_byte(0x00, BYTE_DELAY);      //Memory Card ID1  //5A
  fb[fbp++] = psx_spi_xfer_byte(0x00, BYTE_DELAY);      //Memory Card ID2  //5D
  fb[fbp++] = psx_spi_xfer_byte(AddressMSB, BYTE_DELAY);      // sector number MSB //(00)
  fb[fbp++] = psx_spi_xfer_byte(AddressLSB, BYTE_DELAY);      // sector number LSB //(pre)
  fb[fbp++] = psx_spi_xfer_byte(0x00, BYTE_DELAY * 22);       // ACK 1 //5C //;<-- late /ACK after this byte-pair
  fb[fbp++] = psx_spi_xfer_byte(0x00, BYTE_DELAY);            // ACK 2  //5D
  fb[fbp++] = psx_spi_xfer_byte(0x00, BYTE_DELAY);      //Confirm MSB // MSB  // (Sony) FFFFh for invalid addr, (3rd) data of Sector & 3FFh
  fb[fbp++] = psx_spi_xfer_byte(0x00, BYTE_DELAY);      //Confirm LSB // LSB
  
  for (int i = 0; i < 128; i++)
  {
    fb[fbp++] = psx_spi_xfer_byte(0x00, BYTE_DELAY);    //Get 128 byte data from the frame
  }
  
  fb[fbp++] = psx_spi_xfer_byte(0x00, BYTE_DELAY);      //Checksum (MSB xor LSB xor Data)
  fb[fbp++] = psx_spi_xfer_byte(0x00, BYTE_DELAY);      //Memory Card status byte (should be 47h="G"=Good for Read)
  
  fb[fbp++] = psx_spi_xfer_byte(0x00, BYTE_DELAY);      // 5Ch (3rd party tail ?) /  FEh(BETOP TM?)

debug:
  digitalWrite( PSX_SEL, HIGH); //Deactivate device
  SPI.endTransaction();
}

void readFrameToSerial(byte AddressMSB, byte AddressLSB){
  // read a frame from memory card
  psx_read_frame(AddressMSB, AddressLSB);
  for (int i = 0; i < sizeof fb; i++) {
    Serial.write(fb[i]);
  }
}

// Get ID from Memory Card
// ID buffer
char idb[1 + 10];  // idb[2] -> id 1, idb[3] -> id 2
unsigned int idbp;
void psx_read_id()
{
  SPI.beginTransaction(SPISettings(SPI_SPEED/SPI_SPEED_DIV, LSBFIRST, SPI_MODE3));
  digitalWrite( PSX_SEL, LOW ); //Activate device
  
  idbp = 0;
  idb[idbp++] = CARDID;  // ACK type: for serial protocol
  
  idb[idbp++] = psx_spi_xfer_byte(0x81, BYTE_DELAY);      //Access Memory Card // N/A (dummy response)
  idb[idbp++] = psx_spi_xfer_byte(0x53, BYTE_DELAY);      //Send get id command // flag (only Sony card supports)
  idb[idbp++] = psx_spi_xfer_byte(0x00, BYTE_DELAY);      //Memory Card ID1  //5A
  idb[idbp++] = psx_spi_xfer_byte(0x00, BYTE_DELAY);      //Memory Card ID2  //5D
  idb[idbp++] = psx_spi_xfer_byte(0x00, BYTE_DELAY);      //Receive command ack//5C
  idb[idbp++] = psx_spi_xfer_byte(0x00, BYTE_DELAY);      //Receive command ack//5D
  idb[idbp++] = psx_spi_xfer_byte(0x00, BYTE_DELAY);  // 04
  idb[idbp++] = psx_spi_xfer_byte(0x00, BYTE_DELAY);  // 00
  idb[idbp++] = psx_spi_xfer_byte(0x00, BYTE_DELAY);  // 00
  idb[idbp++] = psx_spi_xfer_byte(0x00, BYTE_DELAY);  // 80

  digitalWrite( PSX_SEL, HIGH); //Deactivate device
  SPI.endTransaction();
}

void readIdToSerial(){
  psx_read_id();
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
    case READFRAME:
      delay(5); // avoid continus read
      readFrameToSerial(cmdbuf[1], cmdbuf[2]);
      break;

    case SETDELAY:
      BYTE_DELAY = cmdbuf[1];
      BYTE_DELAY <<= 8;
      BYTE_DELAY += cmdbuf[2];
      // ACK
      Serial.write(ACKSETDELAY);
      Serial.write(BYTE_DELAY >> 8);
      Serial.write(BYTE_DELAY);
      break;

    case SETSPEEDDIV:   // set spi speed divider (one byte)
      SPI_SPEED_DIV = cmdbuf[1];
      SPI_SPEED_DIV <<= 8;
      SPI_SPEED_DIV += cmdbuf[2];
      // ACK
      Serial.write(ACKSETSPEEDDIV);
      Serial.write(SPI_SPEED_DIV >> 8);
      Serial.write(SPI_SPEED_DIV);
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

