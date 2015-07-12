import RPi.GPIO as GPIO
import time

# T = 1.0/250000 # 4usec
T = 1.0/64000 # 4usec

GPIO.setwarnings(True)

GPIO.setmode(GPIO.BCM)
SEL = 8 # GPIO8, SPI_CE0
CLK = 11 # GPIO 11, SPI_SCLK
CMD = 10 # GPIO 10, SPI_MOSI
DAT = 9 # GPIO 9, SPI_MISO
ACK = 25 # GPIO 25, P25

GPIO.setup([SEL, CLK, CMD], GPIO.OUT)
GPIO.setup([DAT, ACK], GPIO.IN)

def ioByte( obyte ):
    ibyte = 0x00
    bit = 0x01 # LSB first

    # GPIO.output(CLK, GPIO.HIGH)
    # GPIO.output(CMD, GPIO.LOW)
    # time.sleep( T/2 )

    for i in range(0,8):
        # out
        GPIO.output( CMD, GPIO.HIGH if obyte & bit else GPIO.LOW)
        GPIO.output( CLK, GPIO.LOW)
        # delay
        time.sleep( T/2 )
        # read
        if (GPIO.input(DAT)):
            ibyte |= bit
        GPIO.output( CLK, GPIO.HIGH)
        # delay
        time.sleep( T/2 )
        bit <<= 1

        # TODO here wait for ACK

    GPIO.output(CLK, GPIO.HIGH)
    GPIO.output(CMD, GPIO.LOW)
    time.sleep( T/4 )
    return ibyte

def ioByteArray(arr):
    print "ioByteArray() T = ", T
    ret = range(0, len(arr))
    GPIO.output(SEL, GPIO.LOW)
    time.sleep( T/4 )
    for i in range(0, len(arr)):
        ret[i] = ioByte( arr[i] )
    GPIO.output(SEL, GPIO.HIGH)
    return ret

# simple test
def getID():
    cmd = [ 0x81,  # access mem card
            0x53,  # 'S', cmd: get id
            0x0,  
            0x0,  
            0x0,  
            0x0,  
            0x0,  
            0x0,  
            0x0,  
            0x0,  
            0x0  ]
    return ioByteArray(cmd)

print getID()

GPIO.cleanup()
