up:
	scp rcard.c pi@localpi:/home/pi/gpio/
	# scp spidev_test.c pi@localpi:/home/pi/gpio/
	# scp ./py/rcard.py pi@localpi:/home/pi/gpio/py/

down:
	scp pi@localpi:/home/pi/gpio/rcard.c rcard.c 

.PHONY:up
