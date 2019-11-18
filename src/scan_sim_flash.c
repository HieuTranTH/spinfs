#include "spinfs.h"
#include "spi_flash.h"  //print_buffer()
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
//#include <string.h>

/*
 * Return inode structure from a physical address
 * TODO not use double pointer, can just use pointer and return it back
 */
void get_inode_at_addr(struct spinfs_raw_inode **s, FILE *f, uint32_t addr)
{
        //struct spinfs_raw_inode holder;
        *s = realloc(*s, sizeof(**s));     // allocate initial size
        /*
         * Get inode stem (without data)
         */
        fseek(f, addr, SEEK_SET);
        fread(*s, 1, sizeof(**s), f);
        if ((*s)->data_size > 0) {
                *s = realloc(*s, sizeof(**s) + (*s)->data_size);     // allocate correct size
                fseek(f, addr, SEEK_SET);
                fread(*s, 1, sizeof(**s) + (*s)->data_size, f);                    // copy correct size structure
        }
}

/*
 * Update inode table
 * TODO not use double pointer, can just use pointer and return it back
 */
void update_inode_table(struct inode_table_entry **it, uint32_t *max_inode,
                uint32_t inode_num, uint32_t addr, uint32_t version)
{
        printf("        Current biggest inode in inode table: %d\n", *max_inode);

        if (inode_num > *max_inode) {
                printf("        New entry: %d, 0x%06x, %d\n", inode_num, addr, version);
                *max_inode = inode_num;
                // allocate extra memories for higher inodes
                *it = realloc(*it, (*max_inode + 1) * sizeof(**it));
        }
        else {
                printf("        Update old entry: %d, 0x%06x, %d\n", inode_num, addr, version);
        }
        //populate or update entry with current inode metadata
        (*it)[inode_num].physical_addr = addr;
        (*it)[inode_num].version = version;

        printf("        New biggest inode in inode table: %d\n", *max_inode);
}

int main(int argc, char *argv[])
{
        int count = 0;
        int addr = 0;

        /*
         *raw_inode pointer
         */
        struct spinfs_raw_inode *ri = NULL;

        struct inode_table_entry *itable = calloc(1, sizeof(*itable));
        extern uint32_t inode_table_size;  // = array size - 1

        char sim_flash[] = "sim_flash.bin";
        /*
         * Find source file size
         */
        struct stat info;
        if (stat(sim_flash, &info) == -1) {
                perror("Stat file error");
                exit(EXIT_FAILURE);
        }
        printf("File \"%s\" has size: %ld\n\n", sim_flash, info.st_size);

        FILE *fp = fopen(sim_flash, "r");
        if (fp == NULL) {
                perror("File open error");
                exit(EXIT_FAILURE);
        }

        while (addr < info.st_size) {
                printf("Address: 0x%06x\n", addr);
                get_inode_at_addr(&ri, fp, addr);
                print_node_info(ri);
                update_inode_table(&itable, &inode_table_size, ri->inode_num, addr, ri->version);
                printf("\n");

                addr += sizeof(*ri) + ri->data_size;
                count++;
        }

        printf("Total count: %d\n", count);

        printf("\n");
        print_buffer((unsigned char *)itable, (inode_table_size + 1) * sizeof(*itable));


        printf("\n");
        print_inode_table(itable);


        free(ri);
        free(itable);
        fclose(fp);
        return 0;
}
