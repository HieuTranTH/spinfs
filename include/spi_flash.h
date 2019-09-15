#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wiringPiSPI.h>

//#define VERBOSE

/*
 * SPI pintouts:
 *
 * MOSI    P1-19
 * MISO    P1-21
 * SCLK    P1-23   P1-24    CE0
 * GND     P1-25   P1-26    CE1
 */

/*
 * SPI macros
 */
#define SPI_CHANNEL 0  // CE0
//#define SPI_SPEED 25000000  // ~20 MHz?
#define SPI_SPEED 50000000  // ~40 MHz?
#define SPI_MODE 3  // Mode 3: CPOL=1, CPHA=1

/*
 * NOR Flash info
 * All 4-kB sectors have the pattern XXX000h-XXXFFFh
 */
#define MAIN_FLASH_SIZE         8388608 //B = 8 MiB
#define ADDRESS_BITS            24
#define ADDRESS_BYTES           ADDRESS_BITS / 8
#define SECTOR_SIZE             4       //4 KiB
#define BLOCK_SIZE              64      //64 KiB
#define SECTOR_COUNT            2048
#define STARTING_ADDRESS        0x000000
#define ENDING_ADDRESS          0x7FFFFF
// Security Registers info
// has size of 256-byte each
#define SEC_REG_1_START_ADDR    0x001000
#define SEC_REG_1_END_ADDR      0x0010FF
#define SEC_REG_2_START_ADDR    0x002000
#define SEC_REG_2_END_ADDR      0x0020FF
#define SEC_REG_3_START_ADDR    0x003000
#define SEC_REG_3_END_ADDR      0x0030FF

/*
 * NOR Flash command macros
 */
#define READ_STATUS_REGISTER_1  0x05
#define READ_STATUS_REGISTER_2  0x35
#define READ_STATUS_REGISTER_3  0x33
#define WRITE_ENABLE            0x06
#define WRITE_DISABLE           0x04
#define PAGE_PROGRAM            0x02    //1 - 256 B
#define SECTOR_ERASE            0x20    //4 KiB
#define BLOCK_ERASE             0xD8    //64 KiB
#define CHIP_ERASE              0xC7    //8 MiB
#define READ_DATA               0x03
#define READ_JEDEC_ID           0x9F

/*
 * Initialize SPI interface
 */
int spi_init();

int spi_close(int fd);

int spi_read_data(int addr, unsigned char **buf, int count, int bool_output);

int spi_write_enable();

int spi_write_disable();

int spi_write_data(int addr, unsigned char *write_buf, int count, int bool_output);

void dump_flash(const char *name);

/*
 * This function might be obsoleted
 */
unsigned char *str_hex_converter(unsigned char *s);
