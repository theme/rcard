CFLAGS=-g
# LDFLAGS= -lwiringPi -lm
# LDFLAGS= -lpigpio -lrt -lpthread
SPIMODDIR=/lib/modules/3.18.11+/kernel/drivers/spi/
RPIADDR=pi@localpi:/home/pi/gpio/

.PHONY: read clean installnewko installorigiko up

# rpi

read: rcard
	sudo ./$<

rcard: rcard.o

installnewko:
	sudo modprobe -r spi-bcm2708 
	sudo modprobe -r spi-bcm2835
	sudo cp ko.new/*.ko $(SPIMODDIR)
	sudo modprobe spi-bcm2708 
	sudo modprobe spi-bcm2835

installorigiko:
	sudo modprobe -r spi-bcm2708 
	sudo modprobe -r spi-bcm2835
	sudo cp ko.origi/*.ko $(SPIMODDIR)
	sudo modprobe spi-bcm2708 
	sudo modprobe spi-bcm2835

clean:
	find . -maxdepth 1 -type f -perm /111 -exec rm {} \;
	find . -maxdepth 1 -type f -name "*.o" -exec rm {} \;

# local
up:
	scp rcard.c $(RPIADDR)
	scp makefile $(RPIADDR)
	scp $(KSRC)/drivers/spi/spi-bcm2708.ko $(RPIADDR)/ko.new/
	scp $(KSRC)/drivers/spi/spi-bcm2835.ko $(RPIADDR)/ko.new/

