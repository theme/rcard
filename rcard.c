/*
 * SPI testing utility (using spidev driver)
 *
 * Copyright (c) 2007  MontaVista Software, Inc.
 * Copyright (c) 2007  Anton Vorontsov <avorontsov@ru.mvista.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * Cross-compile with cross-gcc -I/path/to/cross-kernel/include
 */

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

// Reverse table is used because we don't know how to change bit-order on SPI settings
static const uint8_t BitReverseTable256[256] = {
    0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0,
    0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8, 0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8,
    0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4, 0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4,
    0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC,
    0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2, 0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2,
    0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA, 0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
    0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6, 0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6,
    0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE, 0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
    0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1, 0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
    0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9,
    0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5, 0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,
    0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED, 0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
    0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3, 0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3,
    0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB, 0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,
    0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7, 0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7,
    0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF, 0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF
};

static void reverseBitsInArray( uint8_t a[], int len ){
    int i = 0;
    for ( i = 0; i < len; ++i ){
        a[i] = BitReverseTable256[a[i]];
    }
}

static void pabort(const char *s)
{
    perror(s);
    abort();
}

static const char *device = "/dev/spidev0.0";
static uint8_t mode;
static uint8_t lsb_first = 0;
static uint8_t bits = 8;
static uint32_t speed = 244000;
static uint16_t xfr_delay;

// broadcom gpio schema
#define SPI_CE0  8 // GPIO8, SPI_CE0
#define SPI_SCLK  11 // GPIO 11, SPI_SCLK
#define SPI_MOSI  10 // GPIO 10, SPI_MOSI
#define SPI_MISO  9 // GPIO 9, SPI_MISO
#define P25  25 // GPIO 25, P25
#define GPCLK0  4 // GPIO 4

// user schema for PSX
#define PSX_SEL  SPI_CE0
#define PSX_CLK  SPI_SCLK
#define PSX_CMD  SPI_MOSI
#define PSX_DAT  SPI_MISO
#define PSX_ACK  P25

#define PSX_SPI_SPEED 244000 // Hz
#define PSX_SPI_BYTE_XFR_DELAY 6 // usec
#define PSX_SPI_BITS_PER_WORD 8 // usec
#define PSX_ACK_WAIT 8 // usec

static void print_buffer( uint8_t rx[], int len){
    int ret;
    for (ret = 0; ret < len; ret++) {
        if (!(ret % 8))
            printf("    ");
        if (!(ret % 16 ) )
            puts("");
        printf("%.2X ", rx[ret]);
    }
    puts("");
}

static void print_xfr( struct spi_ioc_transfer xfr ){
    printf("spi_ioc_transfer: %p\n", &xfr);
    printf(" .tx_buf: %p\n", xfr.tx_buf);
    printf(" .rx_buf: %p\n", xfr.rx_buf);
    printf(" .len: %ld\n", xfr.len);
    printf(" .speed_hz: %d\n", xfr.speed_hz);
    printf(" .bits_per_word: %d\n", xfr.bits_per_word);
    printf(" .delay_usecs: %d\n", xfr.delay_usecs);
    printf(" .cs_change: %d\n", xfr.cs_change);
}

static void spi_dump_stat(int fd)
{
    __u8    lsb=0, bits=0;
    __u32   mode=0, speed=0;

    if (ioctl(fd, SPI_IOC_RD_MODE, &mode) < 0) {
        perror("SPI rd_mode");
        return;
    }

    if (ioctl(fd, SPI_IOC_RD_LSB_FIRST, &lsb) < 0) {
        perror("SPI rd_lsb_fist");
        return;
    }
    if (ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits) < 0) {
        perror("SPI bits_per_word");
        return;
    }
    if (ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed) < 0) {
        perror("SPI max_speed_hz");
        return;
    }

    printf("spi mode 0x%x, %d bits %sper word, %d Hz max\n",
            mode, bits, lsb ? "(lsb first) " : "", speed);
}

static void psx_spi_setup( int fd ){
    int ret;

    speed = 244000;
    mode |= SPI_CPHA;
    mode |= SPI_CPOL;
    bits = PSX_SPI_BITS_PER_WORD ;
    xfr_delay = PSX_SPI_BYTE_XFR_DELAY;
    lsb_first = 1;  // HACK, spi-bcm2708 (driver?) do not support set lsb first.

    ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
    if (ret == -1)
        pabort("can't set spi mode");

    /* ret = ioctl(fd, SPI_IOC_WR_LSB_FIRST, &mode); */
    /* if (ret == -1) */
    /* 	pabort("can't set LSB first"); */

    ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
    if (ret == -1)
        pabort("can't set bits per word");

    ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
    if (ret == -1)
        pabort("can't set max speed hz");
}


static void psx_spi_do_msg(int fd, char *cmd, char *dat, unsigned int len){
    /*
     *  struct spi_ioc_transfer - describes a single SPI transfer 
     *  @tx_buf: Holds pointer to userspace buffer with transmit data, or null. 
     *       If no data is provided, zeroes are shifted out. 
     *  @rx_buf: Holds pointer to userspace buffer for receive data, or null. 
     *  @len: Length of tx and rx buffers, in bytes. 
     *  @speed_hz: Temporary override of the device's bitrate. 
     *  @bits_per_word: Temporary override of the device's wordsize. 
     *  @delay_usecs: If nonzero, how long to delay after the last bit transfer 
     *       before optionally deselecting the device before the next transfer. 
     *  @cs_change: True to deselect device before starting the next transfer. 
     */
    struct spi_ioc_transfer xfer;
    memset( &xfer, 0, sizeof xfer );
    
    xfer.tx_buf = (unsigned long) cmd;
    xfer.rx_buf = (unsigned long) dat;
    xfer.len = len;
    /* xfer.speed_hz = PSX_SPI_SPEED; */
    /* xfer.bits_per_word = PSX_SPI_BITS_PER_WORD; */
    /* xfer.delay_usecs = PSX_SPI_BYTE_XFR_DELAY; */
    /* xfer.cs_change = 0; */

    /* print_xfr( xfer ); */

    int status;

    if ( lsb_first ){
        printf("lsb trans cmd\n");
        reverseBitsInArray(cmd, len);  // soft reverse bit order
    }

    status = ioctl(fd, SPI_IOC_MESSAGE(1), &xfer);

    if (status < 0) {
        perror("SPI_IOC_MESSAGE");
    }
    
    // soft reverse bit order rx
    if (lsb_first) {
        printf("lsb trans dat\n");
        reverseBitsInArray(dat, len);
    }
}

static int psx_get_id( const char* spi_device ){
    /* This command is supported only by original Sony memory cards.
     * Not sure if all sony cards are responding with the same values,
     * and what meaning they have,
     * might be number of sectors (0400h) and sector size (0080h) or whatever.
     */
    uint8_t cmd[] = {
        /* Send Reply Comment*/
        0x81 ,// N/A   Memory Card Access (unlike 01h=Controller access), dummy response
        0x53 ,// FLAG  Send Get ID Command (ASCII "S"), Receive FLAG Byte
        0x00 ,// 5Ah   Receive Memory Card ID1
        0x00 ,// 5Dh   Receive Memory Card ID2
        0x00 ,// 5Ch   Receive Command Acknowledge 1
        0x00 ,// 5Dh   Receive Command Acknowledge 2
        0x00 ,// 04h   Receive 04h
        0x00 ,// 00h   Receive 00h
        0x00 ,// 00h   Receive 00h
        0x00  // 80h   Receive 80h
    };
    uint8_t dat[ARRAY_SIZE(cmd)] = {0, };
    int ret = 0;
    int fd;

    memset(dat, 0xff, ARRAY_SIZE(cmd));     // DEBUG

    fd = open(spi_device, O_RDWR);
    if (fd < 0)
        pabort("psx_get_id() can't open device");

    psx_spi_setup(fd);
    spi_dump_stat(fd);
    psx_spi_do_msg(fd, cmd, dat, ARRAY_SIZE(cmd) );

    close(fd);

    printf("PSX get id\n");
    print_buffer(dat, ARRAY_SIZE(dat) );
    return ret;
}

static int psx_read( const char* spi_device, unsigned long addr, unsigned long read_len ){
    unsigned long len = read_len + 12;
    uint8_t LSB = 0xFF & addr;
    uint8_t MSB = 0xFF & (addr >> 8);
    uint8_t *cmd = calloc( len, sizeof (uint8_t) );
    uint8_t *dat= calloc( len, sizeof (uint8_t) );
    /* Send Reply Comment */
    cmd[0] = 0x81; // N/A   Memory Card Access (unlike 01h=Controller access), dummy response
    cmd[1] = 0x52; // FLAG  Send Read Command (ASCII "R"), Receive FLAG Byte
    cmd[2] = 0x00; // 5Ah   Receive Memory Card ID1
    cmd[3] = 0x00; // 5Dh   Receive Memory Card ID2
    cmd[4] = MSB ; // (00h) Send Address MSB  ;\sector number (0..3FFh)
    cmd[5] = LSB ; // (pre) Send Address LSB  ;/
    /* [6]   0x00     5Ch   Receive Command Acknowledge 1  ;<-- late /ACK after this byte-pair */
    /* [7]   0x00     5Dh   Receive Command Acknowledge 2 */
    /* [8]   0x00     MSB   Receive Confirmed Address MSB */
    /* [9]   0x00     LSB   Receive Confirmed Address LSB */
    /* [10]   0x00    ...   Receive Data Sector (128 bytes) |)}># */
    /* [len-2]   0x00 CHK   Receive Checksum (MSB xor LSB xor Data bytes) */
    /* [len-1]   0x00 47h   Receive Memory End Byte (should be always 47h="G"=Good for Read) */
    /* Non-sony cards additionally send eight 5Ch bytes after the end flag. */
    /* When sending an invalid sector number, 
     * original Sony memory cards respond with FFFFh as Confirmed Address 
     * (and do then abort the transfer without sending any data, checksum, or end flag), 
     * third-party memory cards typically respond with the sector number ANDed with 3FFh
     * (and transfer the data for that adjusted sector number).
     */
    int ret = 0;
    int fd;

    memset(dat, 0xff, len);     // DEBUG

    fd = open(spi_device, O_RDWR);
    if (fd < 0)
        pabort("psx_get_id() can't open device");

    psx_spi_setup(fd);
    spi_dump_stat(fd);
    psx_spi_do_msg(fd, cmd, dat, len );

    close(fd);

    printf("PSX read data at %ld, len %ld\n", addr, read_len);
    print_buffer(dat+10, read_len);
    return ret;
}

static int psx_read_frame( const char* spi_device, unsigned long block, unsigned long frame){
    /* block 0 - 15 , each 8KB*/
    /* frame 0 - 63 , each 128 B */ 
    if ( ! (block < 16) ){
        printf("psx_read_frame() not a block No. : %ld", block);
        abort();
    }

    if ( ! (frame < 64) ){
        printf("psx_read_frame() not a frame No. : %ld", frame);
        abort();
    }

    psx_read( spi_device, block * 8 * 1024 + frame * 128, 128 );
}

int main(int argc, char *argv[])
{
    int ret = 0;

    /* ret = psx_get_id( device ) ; */
    /* ret = psx_read( device, 0x00, 64) ; */
    ret = psx_read_frame( device, 0, 3) ;

    return ret;
}
