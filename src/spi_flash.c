#include "spi_flash.h"

//Debug code segments
/*
   printf("Checking buffer of %d byte(s) at address %06x is:\n", count, addr);
   for (int i = 0; i < buf_size; i++) {
   printf("%02x ", (*buf)[i]);
   if (((i-4) % 8) == 7) printf(" ");
   if (((i-4) % 16) == 15) printf("\n");
   }
   printf("\n\n");
   */

/*
 * Global variables
 */
unsigned char static_buffer[MAX_BUFFER_SIZE + 4];

/*
 * Generic functions
 */
void print_buffer(unsigned char *buf, int count){
        int i = 0;
        for (; i < count; i++) {
                printf("%02x", buf[i]);
                if ((i % 16) == 7)
                        printf("  ");
                else if ((i % 16) == 15)
                        printf("\n");
                else
                        printf(" ");
        }
        if ((i % 16) != 0) printf("\n");
}

int spi_init()
{
        int fd;

        printf("Initializing...\n");
        fd = wiringPiSPISetupMode(SPI_CHANNEL, SPI_SPEED, SPI_MODE);
        //sleep(1);

        printf("Done initialized.\n");
        printf("SPI channel: %d\n", SPI_CHANNEL);
        printf("SPI speed: %d\n", SPI_SPEED);
        printf("SPI mode: %d\n", SPI_MODE);
        printf("\n");

        return fd;
}

int spi_close(int fd)
{
        int ret;
        ret = close(fd);
        return ret;
}

int spi_erase_sector(int addr)
{
        int ret = 0;
        int buf_size = 4;               // buffer size for erase operation always is contant

        unsigned char *buf = calloc(buf_size, sizeof(*buf));
        if (buf == NULL) {
                perror("Realloc error:");
                exit(5);
        }

        //Populate buffer to send
        buf[0] = SECTOR_ERASE;
        //taking little endian into account
        buf[1] = *((char*)&addr + 2);
        buf[2] = *((char*)&addr + 1);
        buf[3] = *(char*)&addr;

        /*
        printf("Checking buffer of %d byte(s) at address %06x is:\n", buf_size, addr);
        for (int i = 0; i < buf_size; i++) {
                printf("%02x ", buf[i]);
                if (((i-4) % 8) == 7) printf(" ");
                if (((i-4) % 16) == 15) printf("\n");
        }
        printf("\n\n");
        exit(0);
        */

        printf("Erasing a sector of 4 KiB at address %06x ...\n", addr);
        spi_write_enable();
        ret = wiringPiSPIDataRW(SPI_CHANNEL, buf, buf_size);
        //TODO: poll BUSY bit until erase operation is finished
        printf("Finish erasing!\n");

        free(buf);
        return ret;
}

int spi_erase_block(int addr)
{
        int ret = 0;
        int buf_size = 4;               // buffer size for erase operation always is contant

        unsigned char *buf = calloc(buf_size, sizeof(*buf));
        if (buf == NULL) {
                perror("Realloc error:");
                exit(5);
        }

        //Populate buffer to send
        buf[0] = BLOCK_ERASE;
        //taking little endian into account
        buf[1] = *((char*)&addr + 2);
        buf[2] = *((char*)&addr + 1);
        buf[3] = *(char*)&addr;

        /*
        printf("Checking buffer of %d byte(s) at address %06x is:\n", buf_size, addr);
        for (int i = 0; i < buf_size; i++) {
                printf("%02x ", buf[i]);
                if (((i-4) % 8) == 7) printf(" ");
                if (((i-4) % 16) == 15) printf("\n");
        }
        printf("\n\n");
        exit(0);
        */

        printf("Erasing a block of 64 KiB at address %06x ...\n", addr);
        spi_write_enable();
        ret = wiringPiSPIDataRW(SPI_CHANNEL, buf, buf_size);
        //TODO: poll BUSY bit until erase operation is finished
        printf("Finish erasing!\n");

        free(buf);
        return ret;
}

int spi_erase_chip(void)
{
        int ret = 0;

        unsigned char buf = CHIP_ERASE;

        printf("Erasing the whole chip (8 MiB) ...\n");
        spi_write_enable();
        ret = wiringPiSPIDataRW(SPI_CHANNEL, &buf, 1);
        //TODO: poll BUSY bit until erase operation is finished
        printf("Finish erasing!\n");

        return ret;
}

/*
int spi_read_data(int addr, unsigned char **buf, int count, int bool_output)
{
        int ret = 0;
        //Calculate buffer size. 1 byte for command + 3 bytes for
        //address + count bytes for how many following bytes to read
        int buf_size = 1 + ADDRESS_BYTES + count;

        //Reallocate buf to have size = buf_size (+4 bytes)
        *buf = realloc(*buf, buf_size * sizeof(**buf));
        if (*buf == NULL) {
                perror("Realloc error:");
                exit(5);
        }
        memset(*buf, 0xAA, buf_size);       //initialized whole buffer to aa (to easily catch error) FIXME

        //Populate buffer to send
        (*buf)[0] = READ_DATA;
        //taking little endian into account
        (*buf)[1] = *((char*)&addr + 2);
        (*buf)[2] = *((char*)&addr + 1);
        (*buf)[3] = *(char*)&addr;

        ret = wiringPiSPIDataRW(SPI_CHANNEL, *buf, buf_size);

        if (bool_output) {
                printf("Data sequence of %d byte(s) at address %06x is:\n", count, addr);
                for (int i = 4; i < buf_size; i++) {
                        printf("%02x ", (*buf)[i]);
                        if (((i-4) % 8) == 7) printf(" ");
                        if (((i-4) % 16) == 15) printf("\n");
                }
                printf("\n\n");
        }

#ifdef VERBOSE
        printf("Read data return: %d\n", ret);
#endif
        return ret;
}
*/

int spi_read_data(int addr, unsigned char *buf, int count)
{
        int ret = 0;
        int i = 0 ;
        int tr_size = 0;

        while (count > 0) {
                //Populate buffer to send
                static_buffer[0] = READ_DATA;
                //taking little endian into account
                static_buffer[1] = *((char*)&addr + 2);
                static_buffer[2] = *((char*)&addr + 1);
                static_buffer[3] = *(char*)&addr;
                tr_size = count > MAX_BUFFER_SIZE ? (MAX_BUFFER_SIZE + 4) : (count + 4);
                ret = wiringPiSPIDataRW(SPI_CHANNEL, static_buffer, tr_size);
                memcpy(buf + i*MAX_BUFFER_SIZE, static_buffer + 4, tr_size - 4);
                count -= tr_size - 4;
                addr += tr_size - 4;
                i++;
        }

#ifdef VERBOSE
        printf("Read data return: %d\n", ret);
#endif
        return ret;
}

int spi_write_enable()
{
        int ret = 0;
        unsigned char buf = WRITE_ENABLE;
        ret = wiringPiSPIDataRW(SPI_CHANNEL, &buf, 1);
#ifdef VERBOSE
        printf("Write enable return: %d\n", ret);
#endif
        return ret;
}

int spi_write_disable()
{
        int ret = 0;
        unsigned char buf = WRITE_DISABLE;
        ret = wiringPiSPIDataRW(SPI_CHANNEL, &buf, 1);
#ifdef VERBOSE
        printf("Write disable return: %d\n", ret);
#endif
        return ret;
}

int spi_write_data(int addr, unsigned char **buf, int count, int bool_output)
{
        int ret = 0;
        //Calculate buffer size. 1 byte for command + 3 bytes for
        //address + count bytes for how many following bytes to write
        int buf_size = 1 + ADDRESS_BYTES + count;

        //Reallocate buf to have size = buf_size (+4 bytes)
        *buf = realloc(*buf, buf_size * sizeof(**buf));
        if (*buf == NULL) {
                perror("Realloc error:");
                exit(5);
        }
        memmove((*buf + 4), *buf, count);

        //Populate buffer to send
        (*buf)[0] = PAGE_PROGRAM;
        //taking little endian into account
        (*buf)[1] = *((char*)&addr + 2);
        (*buf)[2] = *((char*)&addr + 1);
        (*buf)[3] = *(char*)&addr;

        /*
        printf("Checking buffer of %d byte(s) at address %06x is:\n", count, addr);
        for (int i = 0; i < buf_size; i++) {
                printf("%02x ", (*buf)[i]);
                if (((i-4) % 8) == 7) printf(" ");
                if (((i-4) % 16) == 15) printf("\n");
        }
        printf("\n\n");
        */

        if (bool_output) {
                printf("Programming %d byte(s) at address %06x is:\n", count, addr);
                for (int i = 4; i < buf_size; i++) {
                        printf("%02x ", (*buf)[i]);
                        if (((i-4) % 8) == 7) printf(" ");
                        if (((i-4) % 16) == 15) printf("\n");
                }
                printf("\n\n");
        }
        spi_write_enable();
        ret = wiringPiSPIDataRW(SPI_CHANNEL, *buf, buf_size);

#ifdef VERBOSE
        printf("Write data return: %d\n", ret);
#endif
        return ret;
}

/*
 * This function might be obsoleted
 */
unsigned char *str_hex_converter(unsigned char *s)
{
        return s;
}

void dump_flash(const char *name)
{
        int ret = 0;
        FILE *dump_file = fopen(name, "w");
        if (dump_file == NULL){
                perror("Open dump_file error:");
                exit(1);
        }

        int transaction_count = 4096;
        int transaction_size = MAIN_FLASH_SIZE/transaction_count;       /* 2048 bytes */
        int buffer_size = 1 + ADDRESS_BYTES + transaction_size;         /* 2052 bytes */
        printf("Transaction size is: %d\n", transaction_size*sizeof(unsigned char));
        printf("Buffer size is: %d\n", buffer_size*sizeof(unsigned char));

        /* allocate dumping buffer */
        unsigned char *d_buff = calloc(buffer_size, sizeof(*d_buff));
        if (d_buff == NULL) {
                perror("d_buff malloc error:");
                exit(5);
        }

        printf("Dumping whole flash to file [%s] ...\n", name);

        int addr = 0;
        for (int i = 0; i < transaction_count; i++) {
                addr = i * transaction_size;

                //Populate buffer to send
                d_buff[0] = READ_DATA;
                //taking little endian into account
                d_buff[1] = *((char*)&addr + 2);
                d_buff[2] = *((char*)&addr + 1);
                d_buff[3] = *(char*)&addr;

                ret = wiringPiSPIDataRW(SPI_CHANNEL, d_buff, buffer_size);
                if (ret != buffer_size) {
                        printf("SPIDataRW failure!\n");
                        exit(1);
                }
                //spi_read_data(i*transaction_size, &d_buff, transaction_size, 0);
                // d_buff is guaranteed to be reallocated with size + 4 bytes
                fwrite(d_buff+4, sizeof(char), transaction_size, dump_file);
        }

        printf("Finish dumping!\n");
        fclose(dump_file);
        free(d_buff);
}
