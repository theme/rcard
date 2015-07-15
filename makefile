CFLAGS=-g
# LDFLAGS= -lwiringPi -lm
LDFLAGS= -lpigpio -lrt -lpthread

read: rcard
	sudo ./$<

test: spidev_test
	./$<

.PHONY: test read clean up rmmodspi
rcard: rcard.o
spidev_test: spidev_test.o
clean:
	find . -maxdepth 1 -type f -perm /111 -exec rm {} \;
	find . -maxdepth 1 -type f -name "*.o" -exec rm {} \;

up:
	scp rcard.c pi@localpi:/home/pi/gpio/
	scp makefile pi@localpi:/home/pi/gpio/

rmmodspi:
	sudo modprobe -r spi-bcm2708
	sudo modprobe -r spi-bcm2835
