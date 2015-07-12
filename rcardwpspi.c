#include <wiringPiSPI.h>
#include <stdio.h>
#include <math.h>

static unsigned long spd = 244000;

void getID(){
    int i;
    unsigned char cmd[] = { 0x81,  // access mem card
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
    unsigned char ret[ sizeof(cmd) ];
    wiringPiSPIDataRW(0, cmd, sizeof(cmd));

    printf("sizeof cmd: %d\n", sizeof(cmd) );

    for(i = 0; i< sizeof(cmd); ++i ){
        if ( i % 16 == 0 ){
            puts("");
            printf( "%d :\t", (i/16) * 16);
        }
        if ( i % 8 == 0 ){
            printf(" ");
        }
        printf( "%0.2x ", cmd[i]);
    }
    puts("");
}

void test(){
    int i = 0;
    for ( i = 0; i < 1; ++i ){
        spd = spd * pow(2,i);
        wiringPiSPISetupMode ( 0, spd, 3 );
        getID();
    }
}

int main(int argc, char *argv[])
{
    // init wiringPi
    wiringPiSPISetupMode ( 0, spd, 3 );

    test();
    return 0;
}
