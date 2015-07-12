#include <wiringPi.h>
#include <unistd.h>
#include <stdio.h>

// broadcom gpio schema
#define SPI_CE0  8 // GPIO8, SPI_CE0
#define SPI_SCLK  11 // GPIO 11, SPI_SCLK
#define SPI_MOSI  10 // GPIO 10, SPI_MOSI
#define SPI_MISO  9 // GPIO 9, SPI_MISO
#define P25  25 // GPIO 25, P25
#define GPCLK0  4 // GPIO 4

// user schema for PSX
#define PSX_SEL  SPI_CE0
#define PSX_CLK  GPCLK0 // need re-wiring
#define PSX_CMD  SPI_MOSI
#define PSX_DAT  SPI_MISO
#define PSX_ACK  P25

static const useconds_t T = 32; // 1s / 256 KHz for 1 bit

char ioByte( char obyte ){
    char ibyte = 0x00;
    char bit = 0x01; // LSB first
    int i;
    for( i = 0; i < 8; ++i ){
        // out
        digitalWrite(PSX_CMD, HIGH);
        digitalWrite(PSX_CLK, LOW);
        // delay
        usleep( T/2 );
        // read
        if ( digitalRead( PSX_DAT ) )
            ibyte |= bit;
        digitalWrite(PSX_CLK, HIGH);
        // delay
        usleep( T/2 );
        bit <<= 1 ;

        // wait for ACK
        while ( !digitalRead( PSX_ACK ) ){
            usleep(1);
        }
    }
    digitalWrite(PSX_CMD, LOW);
    digitalWrite(PSX_CLK, HIGH);
    return ibyte;
}

void ioByteArray( char arr[], char ret[], int len ){
    int i;
    printf( "ioByteArray() T = %f", T );
    digitalWrite( PSX_SEL, LOW );
    usleep( T/4 );
    for ( i = 0; i < len; ++i ) {
        ret[i] = ioByte( arr[i] );
    }
    digitalWrite(PSX_SEL, HIGH);
}

void getID(){
    int i;
    char cmd[] = { 0x81,  // access mem card
            0x53,  // 'S', cmd: get id
            0x0,  
            0x0,  
            0x0,  
            0x0,  
            0x0,  
            0x0,  
            0x0,  
            0x0,  
            0x0  };
    char ret[ sizeof(cmd) ];
    ioByteArray( cmd, ret, sizeof(cmd) );

    for(i = 0; i< sizeof(cmd); ++i ){
        printf( "%c ", ret[i]);
        if ( i % 16 == 0 )
            puts("");
    }
}

int main(int argc, char *argv[])
{
    // init wiringPi
    wiringPiSetupGpio (); // Broadcom gpio numbering scheme init
    pinMode (PSX_SEL, OUTPUT);
    pinMode (PSX_CLK, GPIO_CLOCK);
    pinMode (PSX_CMD, OUTPUT);
    pinMode (PSX_DAT, INPUT);
    pinMode (PSX_ACK, INPUT);

    getID();

    return 0;
}
