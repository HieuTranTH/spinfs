#include "spinfs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_usage()
{
        fprintf(stderr, "####### Report i-node counts, free and used space #######\n");
        fprintf(stderr, "Format: spinfs_report_space\n");
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

int report_space()
{
        centerTitleText("REPORTING SPACE USAGE", TITLE_TEXT_WIDTH);

        print_head_tail_info(__func__);
        spinfs_report_space();

        centerTitleText("DONE REPORTING SPACE USAGE", TITLE_TEXT_WIDTH);
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
        if (report_space() == -1) {
                perror("REPORT SPACE error");
                exit(EXIT_FAILURE);
        }
        spinfs_deinit();

        return 0;
}
