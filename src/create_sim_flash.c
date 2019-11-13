#include "spinfs.h"
#include "spi_flash.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//realloc_struct_based_on_data_size
//dir_entry_size

void populate_raw_inode(
                struct spinfs_raw_inode **ri_dp,
                char *name,
                uint32_t inode_num,
                uint32_t parent_inode,
                uint32_t version,
                uint32_t data_size,
                char *data)
{
        // realloc based on data_size
        *ri_dp = realloc(*ri_dp, sizeof(**ri_dp) + data_size);
        //populate the node based on parameters
        (*ri_dp)->magic1 = SPIN_FS_MAGIC1;
        strncpy((*ri_dp)->name, name, 32);
        (*ri_dp)->inode_num = inode_num;
        (*ri_dp)->parent_inode = parent_inode;
        (*ri_dp)->version = version;
        (*ri_dp)->data_size = data_size;
        (*ri_dp)->magic2 = SPIN_FS_MAGIC2;
        if (data_size > 0)
                memcpy((*ri_dp)->data, data, data_size);
}

void update_dir_table(struct dir_entry **dt, uint32_t *size, char *name,
                uint32_t inode_num)
{
        printf("        New directory size: %d + %d = %d\n", *size,
                        sizeof(**dt), *size + sizeof(**dt));
        printf("        New entry: %s, %d\n", name, inode_num);

        *dt = realloc(*dt, *size + sizeof(**dt));
        int dt_index = *size / sizeof(**dt);
        printf("        Index of new entry: %d\n", dt_index);
        strncpy((*dt)[dt_index].name, name, 32);
        (*dt)[dt_index].inode_num = inode_num;

        *size += sizeof(**dt);
        //print_buffer((unsigned char*)*dt, *size);
}

int main(int argc, char *argv[])
{
        FILE *fp;
        int count = 0;
        int current_inode_num = 0;
        char *current_name = NULL;
//int current_node_size = 0;

        // raw_inode pointer
        struct spinfs_raw_inode *ri = NULL;

        struct dir_entry *root_p = NULL;
        //struct dir_entry *root_p = malloc(sizeof(*root_p));
        uint32_t root_p_size = 0;
        /*
        struct dir_entry *dir1_p = NULL;
        uint32_t dir1_p_size = 0;
        struct dir_entry *dir2_p = NULL;
        uint32_t dir2_p_size = 0;
        */

        fp = fopen("sim_flash.bin", "w");

        /*
         * new "/" node
         */
        current_name = "/";
        populate_raw_inode(&ri, current_name, ++current_inode_num, 0, 1,
                        root_p_size, (char *)root_p);
        // print info then write to flash
        print_node_info(ri);
        fwrite(ri, 1, sizeof(*ri) + ri->data_size, fp);
        count++;
        ///////////////////////////////////////////////////////////////

        /*
         * new "foo" node
         */
        current_name = "foo";
        populate_raw_inode(&ri, current_name, ++current_inode_num, 1, 1,
                        strlen(current_name), current_name);
        // print info then write to flash
        print_node_info(ri);
        fwrite(ri, 1, sizeof(*ri) + ri->data_size, fp);
        count++;

        /*
         * update "/" node
         */
        update_dir_table(&root_p, &root_p_size, ri->name, ri->inode_num);
        current_name = "/";
        populate_raw_inode(&ri, current_name, 1, 0, 2,
                        root_p_size, (char *)root_p);
        // print info then write to flash
        print_node_info(ri);
        fwrite(ri, 1, sizeof(*ri) + ri->data_size, fp);
        count++;
        ///////////////////////////////////////////////////////////////

#endif



        printf("Total count: %d\n", count);

        if (ri != NULL) free(ri);
        if (root_p != NULL) free(root_p);
        fclose(fp);
        return 0;
}
