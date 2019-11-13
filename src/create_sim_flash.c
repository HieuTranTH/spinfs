#include "spinfs.h"
#include "spi_flash.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//realloc_struct_based_on_data_size
//dir_entry_size

void populate_raw_inode(
                struct spinfs_raw_inode *ri,
                char *name,
                uint32_t inode_num,
                uint32_t parent_inode,
                uint32_t version,
                uint32_t data_size,
                char *data)
{
        // realloc based on data_size
        ri = realloc(ri, sizeof(*ri) + data_size);
        //populate the node based on parameters
        ri->magic1 = SPIN_FS_MAGIC1;
        strncpy(ri->name, name, 32);
        ri->inode_num = inode_num;
        ri->parent_inode = parent_inode;
        ri->version = version;
        ri->data_size = data_size;
        ri->magic2 = SPIN_FS_MAGIC2;
        if (data_size > 0)
                memcpy(ri->data, data, data_size);
}

void update_dir_table(struct dir_entry *dir_table_p, uint32_t *current_size, char *name, uint32_t inode_num)
{
        printf("Dir size to be updated: %d + %d = %d\n", *current_size,
                        sizeof(*dir_table_p), *current_size
                        + sizeof(*dir_table_p));
        printf("Other parameter: %s, %d\n", name, inode_num);
        dir_table_p = realloc(dir_table_p, *current_size + sizeof(*dir_table_p));
        strncpy(dir_table_p[*current_size].name, name, 32);
        dir_table_p[*current_size].inode_num = inode_num;

        *current_size += sizeof(*dir_table_p);

        printf("Content in pointer: %p\n", dir_table_p);
        print_buffer((unsigned char*)dir_table_p, *current_size);
}

int main(int argc, char *argv[])
{
        FILE *fp;
        int count = 0;
        int current_inode_num = 0;
        char *current_name = NULL;
//int current_node_size = 0;
        struct dir_entry *root_dir_p = NULL;
        //struct dir_entry *root_dir_p = malloc(sizeof(*root_dir_p));
        uint32_t root_dir_p_size = 0;
        /*
        struct dir_entry *dir1_p = NULL;
        uint32_t dir1_p_size = 0;
        struct dir_entry *dir2_p = NULL;
        uint32_t dir2_p_size = 0;
        */

        fp = fopen("sim_flash.bin", "w");

        // raw_inode pointer
        struct spinfs_raw_inode *ri = malloc(sizeof(*ri));

        /*
         * new "/" node
         */
        current_name = "/";
        populate_raw_inode(ri, current_name, ++current_inode_num, 0, 1,
                        root_dir_p_size, (char *)root_dir_p);
        // print info then write to flash
        print_node_info(ri);
        fwrite(ri, 1, sizeof(*ri) + ri->data_size, fp);
        count++;
        ///////////////////////////////////////////////////////////////

        /*
         * new "foo" node
         */
        current_name = "foo";
        populate_raw_inode(ri, current_name, ++current_inode_num, 1, 1,
                        strlen(current_name), current_name);
        // print info then write to flash
        print_node_info(ri);
        fwrite(ri, 1, sizeof(*ri) + ri->data_size, fp);
        count++;

        /*
         * update "/" node
         */
        printf("Content in pointer: %p\n", root_dir_p);
        //update_dir_table(root_dir_p, &root_dir_p_size, ri->name, ri->inode_num);
        printf("New root_dir_p_size: %d\n", root_dir_p_size);

        printf("Content in pointer: %p\n", root_dir_p);
        print_buffer((unsigned char*)root_dir_p, root_dir_p_size);
        current_name = "/";
        populate_raw_inode(ri, current_name, 1, 0, 2,
                        root_dir_p_size, (char *)root_dir_p);
        // print info then write to flash
        print_node_info(ri);
        fwrite(ri, 1, sizeof(*ri) + ri->data_size, fp);
        count++;
        ///////////////////////////////////////////////////////////////




        printf("Total count: %d\n", count);

        free(ri);
        fclose(fp);
        return 0;
}
