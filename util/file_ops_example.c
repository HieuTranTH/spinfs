#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_usage()
{
        fprintf(stderr, "####### Copy 2 files byte by byte #######\n");
        fprintf(stderr, "Usage: %s [file]\n", "file_ops_example");
        fprintf(stderr, "### If no file name is specified, default file \"flash_dump.bin\" will be used ###\n");
        fprintf(stderr, "\n");
}

int main(int argc, char *argv[])
{
        print_usage();

        FILE *fp, *fp_w;
        int d;
        unsigned char c;
        int count;

        if (argc > 1)
                fp = fopen(argv[1], "r+");
        else
                fp = fopen("flash_dump.bin", "r+");
        if (fp == NULL) {
                perror("File open error");
                exit(1);
        }

        fp_w = fopen("fp_w.txt", "w");

#if 1
        /*
         * Test seek at offset 0
         * This copies byte by byte
         */
        do {
                d = fgetc(fp);
                if (d == EOF)
                        break;
                printf("%x|", d);
                fputc(d, fp_w);
                count++;
        } while (1);
        printf("\n");
        printf("count: %d\n", count);
#elif 0
        /*
         * Test seek at offset 2, and print offset after each byte read
         * This copies byte by byte
         */
        fseek(fp, 2, SEEK_SET);
                printf("current offset: %ld\n", ftell(fp));
        do {
                if (fread(&c, 1, 1, fp) == 0) {
                        perror("EOF error");
                        printf("feof return: %d\n", feof(fp));
                        printf("ferror return: %d\n", ferror(fp));
                        break;
                }
                printf("%x\n", c);
                printf("current offset: %ld\n", ftell(fp));
                fputc(c, fp_w);
                count++;
        } while (1);
        printf("\n");
        printf("count: %d\n", count);
#else
        /*
         * this only copies integers
         */
        do {
                if (fscanf(fp, "%d", &d) == EOF) {
                        perror("EOF error");
                        printf("ferror return: %d\n", ferror(fp));
                        break;
                }
                printf("%x|", d);
                fprintf(fp_w, "%d", d);
                count++;
        } while (1);
        printf("\n");
        printf("count: %d\n", count);
#endif

        fclose(fp);
        fclose(fp_w);
        return 0;
}
