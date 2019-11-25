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
                *s = realloc(*s, sizeof(**s) + (*s)->data_size);     // allocate extra memory for data[]
                //fseek(f, addr, SEEK_SET);
                //fread(*s, 1, sizeof(**s) + (*s)->data_size, f);                    // copy correct size structure
                fread((*s)->data, 1, (*s)->data_size, f);                    // copy extra data to the allocated struct
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

        printf("\n---------------------------------------------------------------\n");
        printf("Executing ls tool:\n");
        printf("\n---------------------------------------------------------------\n");

        char path[] = "/dir1/a";
        uint32_t target_inode_num = 1;
        printf("Address of path pointer: %p.\n", path);
        if (path[0] != '/') {
                printf("Path needs to be absolute!\n");
                exit(EXIT_FAILURE);
        }
        printf("\npath: %s\n", path);
        int pathlen = strlen(path);
        printf("pathlen: %d\n\n", pathlen);
        /*
         * Parse the path to get the target i-node
         */
        char *token;
        token = strtok(path, "/");
        //printf("Address of token pointer: %p.\n", token);
        if (token == NULL) {
                // target i-node is root directory
                target_inode_num = 1;
        } else {
                // get current directory name from path
                while (token != NULL) {
                        /*
                        printf("\npath: %s\n", path);
                        fwrite(path, 1, pathlen, stdout);
                        printf("\n");
                        printf("Address of token pointer: %p.\n", token);
                        printf("-%s-\n", token);
                        */
                        get_inode_at_addr(&ri, fp, itable[target_inode_num].physical_addr);
                        target_inode_num = find_file_in_dir(ri, token);
                        if (target_inode_num == 0) break;

                        token = strtok(NULL, "/");
                }
        }


        /*
         * List content of the i-node afer path processing
         * TODO if i-node is a directory, list content; otherwise repeat its name
         */
        if (target_inode_num == 0) {
                printf("File/Directory not found.\n");
        } else {
                get_inode_at_addr(&ri, fp, itable[target_inode_num].physical_addr);
                if (S_ISREG(ri->mode)) {        // target is a regular file
                        ls_file(ri);
                } else if (S_ISDIR(ri->mode)) {
                        print_directory(ri);
                }
        }

        free(ri);
        free(itable);
        fclose(fp);
        return 0;
}
