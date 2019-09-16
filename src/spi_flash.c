#include "spi_flash.h"
#include <time.h>

/*
 * Global variables
 */

int spi_init()
{
        int fd;

        printf("Initializing...\n");
        fd = wiringPiSPISetupMode(SPI_CHANNEL, SPI_SPEED, SPI_MODE);
        sleep(1);

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

int spi_read_data(int addr, unsigned char **buf, int count, int bool_output)
{
        int ret = 0;
        //Calculate buffer size. 1 byte for command + 3 bytes for
        //address + count bytes for how many following bytes to read
        int buf_size = 1 + ADDRESS_BYTES + count;

        *buf = (unsigned char*)realloc(*buf, buf_size * sizeof(**buf));

        if (*buf == NULL) {
                perror("Realloc error:");
                exit(5);
        }
        memset(*buf, 0xAA, buf_size);       //initialized whole buffer to aa (to easily catch error)

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

int spi_write_data(int addr, unsigned char **write_buf, int count, int bool_output)
{
        int ret = 0;
        //Calculate buffer size. 1 byte for command + 3 bytes for
        //address + count bytes for how many following bytes to write
        int buf_size = 1 + ADDRESS_BYTES + count;
        unsigned char *buf = (unsigned char*)malloc(buf_size * sizeof(unsigned char));
        if (*buf == NULL) {
                perror("Realloc error:");
                exit(5);
        }

        //Populate buffer to send
        buf[0] = PAGE_PROGRAM;
        //taking little endian into account
        buf[1] = *((char*)&addr + 2);
        buf[2] = *((char*)&addr + 1);
        buf[3] = *(char*)&addr;
        memcpy(buf + 4, write_buf, count);

        if (bool_output) {
                printf("Programming %d byte(s) at address %06x is:\n", count, addr);
                for (int i = 4; i < buf_size; i++)
                        printf("%02x ", buf[i]);
                printf("\n\n");
        }
        spi_write_enable();
        ret = wiringPiSPIDataRW(SPI_CHANNEL, buf, buf_size);

        free(buf);

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
        FILE *dump_file = fopen(name, "w");
        if (dump_file == NULL){
                perror("Open dump_file error:");
                exit(1);
        }

        int transaction_count = 4096;
        int transaction_size = MAIN_FLASH_SIZE/transaction_count;
        printf("Transaction size is: %d\n", transaction_size*sizeof(unsigned char));

        unsigned char *d_buff = malloc(sizeof(*d_buff));
        if (d_buff == NULL) {
                perror("d_buff malloc error:");
                exit(5);
        }

        printf("Dumping whole flash...\n");

        for (int i = 0; i < transaction_count; i++){
                spi_read_data(i*transaction_size, &d_buff, transaction_size, 0);
                // d_buff is guaranteed to be reallocated with size + 4 bytes
                fwrite(d_buff+4, sizeof(char), transaction_size, dump_file);
        }

        printf("Finish dumping!\n");
        fclose(dump_file);
        free(d_buff);
}
