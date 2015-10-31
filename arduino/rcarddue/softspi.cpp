#include "softspi.h"

SPIpins::SPIpins( int MOSI, int MISO, int SCK, int SS):
    MOSI_(MOSI), MISO_(MISO), SCK_(SCK), SS_(SS)
{
}

SPIpins::SPIpins(SPIpins& other){
    MOSI_ = other.MOSI_;
    MISO_ = other.MISO_;
    SCK_ = other.SCK_;
    SS_ = other.SS_;
}

SoftSPISettings::SoftSPISettings(uint32_t clock, uint32_t delay, uint8_t bitOrder, uint8_t dataMode):
    clock_(clock),delay_(delay),bitOrder_(bitOrder),dataMode_(dataMode)
{
    calcMode();
}

SoftSPISettings::SoftSPISettings(SoftSPISettings& other){
    clock_ = other.clock_;
    delay_ = other.delay_;
    bitOrder_ = other.bitOrder_;
    dataMode_ = other.dataMode_;
    calcMode();
}

void
SoftSPISettings::calcMode(){
    switch(dataMode_){
        case 0:
            CPOL_=0;
            CPHA_=0;
            break;
        case 1:
            CPOL_=0;
            CPHA_=1;
            break;
        case 2:
            CPOL_=1;
            CPHA_=0;
            break;
        case 3:
            CPOL_=1;
            CPHA_=1;
            break;
        default:    // mode 3
            CPOL_=1;
            CPHA_=1;
            break;
    }
}

SoftSPI::SoftSPI(uint32_t cpu_feq, const SoftSPISettings& sets, const SPIpins& pins):
    cpu_feq_(cpu_feq), sets_(sets), pins_(pins)
{
    halfBitCycle_ = cpu_feq_ / sets_.clock_;
    if( halfBitCycle_ == 0 )
        halfBitCycle_ = cpu_feq_;

    delayCycle_ = cpu_feq_ / sets_.delay_;
    if( delayCycle_ == 0 )
        delayCycle_ = cpu_feq_;
}

void
SoftSPI::setup(){
    pinMode(pins_.MOSI_, OUTPUT);
    pinMode(pins_.MISO_, INPUT);
    pinMode(pins_.SCK_, OUTPUT);
    pinMode(pins_.SS_, OUTPUT);
}

void
SoftSPI::beginTransfer(){
    // clock
    if (sets_.CPOL_)
        digitalWrite(pins_.SCK_, HIGH);
    else
        digitalWrite(pins_.SCK_, LOW);
    // ss enable
    digitalWrite(pins_.SS_, LOW);
}

uint8_t
SoftSPI::transfer(uint8_t data){
    int i = 0;
    uint8_t tmp = data;
    // reverse data when LSBFIRST
    if (sets_.bitOrder_ == LSBFIRST){
        while( i++ < 8 ){
            data <<= 1;
            data += (tmp & 1) ? 1 : 0;
            tmp >>= 1;
        }
    }
    // clock
    if (sets_.CPOL_)
        digitalWrite(pins_.SCK_, HIGH);
    else
        digitalWrite(pins_.SCK_, LOW);
    // data and clock
    i = 0;
    while ( i++ < 8 ){
        // IO
        if (0 == sets_.CPHA_) {
            data <<= i;
            digitalWrite(pins_.MOSI_, (data & 0x80) ? HIGH: LOW);
        }

        // half T
        wait(halfBitCycle_);
        if (sets_.CPOL_)
            digitalWrite(pins_.SCK_, LOW);
        else
            digitalWrite(pins_.SCK_, HIGH);

        // IO
        if (0 == sets_.CPHA_) {
            tmp <<= 1;
            if ( HIGH == digitalRead( pins_.MISO_ ) )
                tmp += 1;
        }
        else // CPHA_ == 1
        {
            data <<= i;
            digitalWrite(pins_.MOSI_, (data & 0x80) ? HIGH: LOW);
        }

        // half T
        wait(halfBitCycle_);
        if (sets_.CPOL_)
            digitalWrite(pins_.SCK_, HIGH);
        else
            digitalWrite(pins_.SCK_, LOW);

        // IO
        if (1 == sets_.CPHA_) {
            tmp <<= 1;
            if ( HIGH == digitalRead( pins_.MISO_ ) )
                tmp += 1;
        }
    }

    // byte delay
    for( delayCounter_ = 0 ; delayCounter_ < delayCycle_ ; delayCounter_++ ){}

    return tmp;
}

void SoftSPI::interruptTransferDelay(){
    delayCounter_ = delayCycle_;
}

void SoftSPI::delay(uint32_t clock){
    while ( clock-- > 0 ) {
    };
}

void SoftSPI::endTransfer(){
    // ss disable
    digitalWrite(pins_.SS_, HIGH);
}

