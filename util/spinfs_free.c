#include "spinfs.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

void print_usage()
{
        fprintf(stderr, "####### Garbage Collector trying to claim all dirty space and erase the flash #######\n");
        fprintf(stderr, "Format: spinfs_free\n");
        fprintf(stderr, "\n");
}

#define TITLE_TEXT_WIDTH        32

/* TODO layout is not consistent if fieldWidth is odd */
void centerText(char *text, int fieldWidth) {
        int padlen = (fieldWidth - strlen(text)) / 2;
        printf("%*s%s%*s", padlen, "", text, padlen, "");
}

void centerTitleText(char *text, int fieldWidth) {
        printf("\n#####################");
        centerText(text, fieldWidth);
        printf("#####################\n\n");
}

int garbage_collector()
{
        spinfs_free_first_sector();
        return 0;
}

int main(int argc, char *argv[])
{
        if (argc != 1) {
                printf("Too many arguments\n\n");
                print_usage();
                exit(EXIT_FAILURE);
        }
        print_usage();

        spinfs_init(0);
        if (garbage_collector() == -1) {
                perror("FREE error");
                exit(EXIT_FAILURE);
        }
        spinfs_deinit();

        return 0;
}
