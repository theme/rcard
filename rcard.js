var SPI = require('pi-spi');
// var spi = SPI.initialize("/dev/spidev0.0");

var PSXspiOnDev= function ( dev ) {
    var spi = SPI.initialize(dev);
    spi.clockSpeed(256000);
    spi.dataMode(SPI.mode.CPOL | SPI.mode.CPHA);
    spi.bitOrder(SPI.order.LSB_FIRST);
    return spi;
};

var readFrame = function() {
    var spi = PSXspiOnDev("/dev/spidev0.0");
    var cmd = '\x81';   // Access mem card
    cmd += '\x52';      // send read command
    cmd += '\x00';      // mem card id 1 ??
    cmd += '\x00';      // mem card id 2 ??
    cmd += '\x00';      // Address MSB
    cmd += '\x00';      // Address LSB
    cmd += '\x00';      // mem card ACK1
    cmd += '\x00';      // mem card ACK2
    cmd += '\x00';      // Confirm MSB
    cmd += '\x00';      // Confirm LSB

    for (var i = 0; i< 128; i++){
        cmd += '\x00';
    }
    cmd += '\x00';      // Checksum (MSB xor LSB)
    cmd += '\x00';      // mem card status byte

    var b = Buffer(cmd);
    spi.transfer( b, b.length, function(e, d){
        if (e) console.error(e);
        else console.log(d.toString());
    });
};

readFrame();
