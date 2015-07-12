up:
	scp rcardwp.c pi@localpi:/home/pi/gpio/
	scp rcardwpspi.c pi@localpi:/home/pi/gpio/
	# scp rcard.c pi@localpi:/home/pi/gpio/
	# scp spidev_test.c pi@localpi:/home/pi/gpio/
	# scp ./py/rcard.py pi@localpi:/home/pi/gpio/py/

down:
	scp pi@localpi:/home/pi/gpio/rcardwp.c rcardwp.c

.PHONY:up
