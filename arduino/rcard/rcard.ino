/*
    Arduino PS1 Memory Card Reader - MemCARDuino, Shendo 2013

    Thanks to:
    Martin Korth of the NO$PSX - documented Memory Card protocol.
    Andrew J McCubbin - documented PS1 SPI interface.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see http://www.gnu.org/licenses.

    Connecting a Memory Card to Arduino:
    ------------------------------------
    Looking at the Memory Card:
    _________________
    |_ _|_ _ _|_ _ _|
     1 2 3 4 5 6 7 8

     1 DATA - Pin 12 on Arduino
     2 CMND - Pin 11 on Arduino
     3 7.6V - 5V Pin on Arduino
     4 GND - GND Pin on Arduino
     5 3.6V - 5V Pin on Arduino
     6 ATT - Pin 10 on Arduino
     7 CLK - Pin 13 on Arduino
     8 ACK - Pin 2 on Arduino

     If your card still isn't readable and it's a 3rd party card,
     you will need to supply it with 7.6 V extrenal power supply.

     Interfacing a MemCARDuino:
     ---------------------------
     Communication is done at 38400 bps.
     To check if the MemCARDuino is connected to the selected COM port send a GETID command.
     Device should respond with IDENTIFIER.
     Optionally you can send a GETVER to get the version of the firmware.

     To Read a 128byte frame send a MCR command with MSB byte and LSB byte of the frame you want to read.
     MemCARDduino will respond with 128 byte frame data, [MSB xor LSB xor Data] and Memory Card status byte.

     To Write a frame send a MCW command with MSB byte and LSB byte, 128 byte data and [MSB xor LSB xor Data].
     MemCARDduino will respond with Memory Card status byte.

     Checking if the Memory Card is inserted:
     -----------------------------------------
     Read a frame of the card and verify the returned status byte.
     If it's 0x47 then card is connected. If it's 0xFF card is not connected.
*/

#include "Arduino.h"

//Memory Card Responses
//0x47 - Good
//0x4E - BadChecksum
//0xFF - BadSector

//Define pins
#define DataPin 12         //Data                   // SPI MISO
#define CmdPin 11          //Command                // SPI MOSI 
#define AttPin 10          //Attention (Select)     // SPI SS
#define ClockPin 13        //Clock                  // SPI SCK      // build in LED
#define AckPin 2           //Acknowledge            // external IRQ, (2 or 3 is supported)

// user schema for PSX
#define PSX_SEL  AttPin
#define PSX_CLK  ClockPin
#define PSX_CMD  CmdPin
#define PSX_DAT  DataPin
#define PSX_ACK  AckPin

#define SPI_XFER_BYTE_DELAY_MAX     4500 // micro seconds
#define SPI_ATT_DELAY    16 // micro seconds

// SPI example
// SPI.beginTransaction(SPISettings(14000000, MSBFIRST, SPI_MODE0));
//If other libraries use SPI from interrupts, they will be prevented from accessing SPI until you call SPI.endTransaction(). Your settings remain in effect for the duration of your "transaction". You should attempt to minimize the time between before you call SPI.endTransaction(), for best compatibility if your program is used together with other libraries which use SPI.
//With most SPI devices, after SPI.beginTransaction(), you will write the slave select pin LOW, call SPI.transfer() any number of times to transfer data, then write the SS pin HIGH, and finally call SPI.endTransaction().

boolean f_psx_ack = false;

void spi_setup() {
  // junk clr variable
  byte clr;

  // pin mode
  pinMode(PSX_DAT, INPUT);
  pinMode(PSX_CMD, OUTPUT);
  pinMode(PSX_SEL, OUTPUT);
  pinMode(PSX_CLK, OUTPUT);
  pinMode(PSX_ACK, INPUT);

  // init status
  digitalWrite(PSX_DAT, HIGH); // pull up (INPUT)
  digitalWrite(PSX_CMD, LOW);
  digitalWrite(PSX_SEL, HIGH); // un select slave
  digitalWrite(PSX_CLK, HIGH);
  digitalWrite(PSX_ACK, HIGH); // pull up (INPUT)

  // https://www.arduino.cc/en/Tutorial/SPIEEPROM
  // SPI Control Register (SPCR)
  // PCR
  // | 7    | 6    | 5    | 4    | 3    | 2    | 1    | 0    |
  // | SPIE | SPE  | DORD | MSTR | CPOL | CPHA | SPR1 | SPR0 |
  //
  // SPIE - Enables the SPI interrupt when 1
  // SPE - Enables the SPI when 1
  // DORD - Sends data least Significant Bit First when 1, most Significant Bit first when 0
  // MSTR - Sets the Arduino in master mode when 1, slave mode when 0
  // CPOL - Sets the data clock to be idle when high if set to 1, idle when low if set to 0
  // CPHA - Samples data on the falling edge of the data clock when 1, rising edge when 0
  // SPR1 and SPR0 - Sets the SPI speed, 00 is fastest (4MHz) 11 is slowest (250KHz)
  SPCR = (0 << SPIE) | (1 << SPE) | (1 << DORD) | (1 << MSTR) | (1 << CPOL) | (1 << CPHA) | (1 << SPR1) | (1 << SPR0) ;

  // SPI data register (SPDR): holds the byte which is about to be shifted out the MOSI line,
  //                           and the data which has just been shifted in the MISO line.
  clr = SPDR;     // read will cause hardware clear of flags

  // SPI status register (SPSR): gets set to 1 when a value is shifted in or out of the SPI.
  clr = SPSR;     // read will cause hardware clear of flags

  // ISR flag: init : no ack received yet
  f_psx_ack = false;
  delay(10);
}

byte spi_xfer_byte(byte cmdByte, unsigned int Delay) {
  SPDR = cmdByte;             // Start the transmission
  while (!(SPSR & (1 << SPIF))) // Poll for the end of the transmission
  {
  };
  while ( ! f_psx_ack && Delay > 0 ) // Poll for the ACK signal from the Memory Card
  {
    Delay--;
    delayMicroseconds(1);
  }
  if ( f_psx_ack ) { // ACK interrupt
    f_psx_ack = false;
  } else {  // ACK time out
  }
  return SPDR; // return the received byte
}

void psx_ack_isr() {
  f_psx_ack = true;
}

//Send a command to PlayStation port using SPI
byte psx_spi_cmd(byte cmdByte, int Delay)
{
  return spi_xfer_byte( cmdByte, Delay);
}

// frame buffer
char fb[10 + 128 + 2 + 8];  // read cmd header + frame data + 2 checksum + 8 byte 0x5C if 3rd party card.
unsigned int fbp, datp;
//Read a frame from Memory Card and send it to serial port
void psx_read_frame(byte AddressMSB, byte AddressLSB)
{
  digitalWrite( PSX_SEL, LOW ); //Activate device

  fbp = 0;
  fb[fbp++] = psx_spi_cmd(0x81, SPI_XFER_BYTE_DELAY_MAX);      //Access Memory Card // FF (Error code)
  fb[fbp++] = psx_spi_cmd(0x52, SPI_XFER_BYTE_DELAY_MAX);      //Send read command // 00
  fb[fbp++] = psx_spi_cmd(0x00, SPI_XFER_BYTE_DELAY_MAX);      //Memory Card ID1  //5A
  fb[fbp++] = psx_spi_cmd(0x00, SPI_XFER_BYTE_DELAY_MAX);      //Memory Card ID2  //5D
  fb[fbp++] = psx_spi_cmd(AddressMSB, SPI_XFER_BYTE_DELAY_MAX);      //Address MSB //00
  fb[fbp++] = psx_spi_cmd(AddressLSB, SPI_XFER_BYTE_DELAY_MAX);      //Address LSB //00
  fb[fbp++] = psx_spi_cmd(0x00, SPI_XFER_BYTE_DELAY_MAX);      //Memory Card ACK1  //5C
  fb[fbp++] = psx_spi_cmd(0x00, SPI_XFER_BYTE_DELAY_MAX);      //Memory Card ACK2  //5C
  fb[fbp++] = psx_spi_cmd(0x00, SPI_XFER_BYTE_DELAY_MAX);      //Confirm MSB // 5D
  fb[fbp++] = psx_spi_cmd(0x00, SPI_XFER_BYTE_DELAY_MAX);      //Confirm LSB // FF

  datp = fbp;
  //Get 128 byte data from the frame
  for (int i = 0; i < 128; i++)
  {
    fb[fbp++] = psx_spi_cmd(0x00, SPI_XFER_BYTE_DELAY_MAX);
  }
  fb[fbp++] = psx_spi_cmd(0x00, SPI_XFER_BYTE_DELAY_MAX);      //Checksum (MSB xor LSB xor Data)
  fb[fbp++] = psx_spi_cmd(0x00, SPI_XFER_BYTE_DELAY_MAX);      //Memory Card status byte

  digitalWrite( PSX_SEL, HIGH); //Deactivate device

  // wite back to serial
  for (int i = 0; i < sizeof fb; i++) {
    Serial.write(fb[i]);
  }
}

void setup()
{
  Serial.begin(38400);
  spi_setup();
  // attachInterrupt(interrupt, ISR, mode);
  // interrupt: numbers 0 (on digital pin 2) and 1 (on digital pin 3)
  // mode: LOW / CHANGE / RISING / FALLING
  attachInterrupt(0, psx_ack_isr, FALLING);
}

#define CMDLEN_MAX 3
byte cmd[CMDLEN_MAX] = {0};
unsigned cmdlen = 0;

void parseCmd() {
  switch (cmd[0])
  {
    default:
      Serial.write(0xEE);
      break;

    case 'R':
      //delay(5);
      //psx_read_frame(1, 1);
      Serial.write('R');
      Serial.write(cmd[1]);  // echo addr
      Serial.write(cmd[2]);
      break;

    case 'S':
      Serial.write('S');
      break;
  }
}
//    cmd[cmdlen++] = serialByte;
//    if (CMDLEN_MAX == cmdlen) {
//      runCmd();
//      cmdlen = 0;
//    }

byte serialByte;
void loop()
{
  if (Serial.available() > 0)
  {
    serialByte = Serial.read();
    Serial.print("Hello!");
  }
}

