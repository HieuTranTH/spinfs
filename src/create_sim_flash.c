#include "spinfs.h"
#include "spi_flash.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//realloc_struct_based_on_data_size
//dir_entry_size

/*
 * TODO not use double pointer, can just use pointer and return it back
 */
void populate_raw_inode(
                struct spinfs_raw_inode **ri_dp,
                char *name,
                uint32_t inode_num,
                mode_t mode,
                uid_t uid,
                gid_t gid,
                int write_ctime,
                uint32_t parent_inode,
                uint32_t version,
                uint32_t data_size,
                char *data)
{
        // realloc based on data_size
        *ri_dp = realloc(*ri_dp, sizeof(**ri_dp) + data_size);
        //populate the node based on parameters
        (*ri_dp)->magic1 = SPIN_FS_MAGIC1;
        strncpy((*ri_dp)->name, name, MAX_NAME_LEN);
        (*ri_dp)->inode_num = inode_num;
        (*ri_dp)->mode = mode;
        (*ri_dp)->uid = uid;
        (*ri_dp)->gid = gid;
        if (write_ctime) (*ri_dp)->ctime = time(NULL);
        (*ri_dp)->parent_inode = parent_inode;
        (*ri_dp)->version = version;
        (*ri_dp)->data_size = data_size;
        (*ri_dp)->magic2 = SPIN_FS_MAGIC2;
        if (data_size > 0)
                memcpy((*ri_dp)->data, data, data_size);
}

/*
 * size is in bytes, not the size of array
 * TODO not use double pointer, can just use pointer and return it back
 */
void update_dir_table(struct dir_entry **dt, uint32_t *size, char *name,
                uint32_t inode_num)
{
        printf("        New directory size: %d + %d = %d\n", *size,
                        sizeof(**dt), *size + sizeof(**dt));
        printf("        New entry: %.*s, %d\n", MAX_NAME_LEN, name, inode_num);

        *dt = realloc(*dt, *size + sizeof(**dt));
        int dt_index = *size / sizeof(**dt);
        printf("        Index of new entry: %d\n", dt_index);
        strncpy((*dt)[dt_index].name, name, MAX_NAME_LEN);
        (*dt)[dt_index].inode_num = inode_num;

        *size += sizeof(**dt);
        //print_buffer((unsigned char*)*dt, *size);
}

int main(int argc, char *argv[])
{
        char sim_flash[] = "sim_flash.bin";
        FILE *fp = fopen(sim_flash, "w");
        if (fp == NULL) {
                perror("File open error");
                exit(EXIT_FAILURE);
        }

        int count = 0;
        int current_inode_num = 0;
        char *current_name = NULL;
//int current_node_size = 0;

        /*
         *raw_inode pointer
         */
        struct spinfs_raw_inode *ri = NULL;

        struct dir_entry *root_p = NULL;
        uint32_t root_p_size = 0;
        struct dir_entry *dir1_p = NULL;
        uint32_t dir1_p_size = 0;
        struct dir_entry *dir2_p = NULL;
        uint32_t dir2_p_size = 0;

        uid_t current_uid = getuid();
        gid_t current_gid = getgid();
        printf("Current UID: %d, current GID: %d\n", current_uid, current_gid);

        printf("Current time from Epoch: %ld, in hex 0x%x\n", time(NULL), time(NULL));

        /*
         * new "/" node
         */
        current_name = "/";
        populate_raw_inode(&ri, current_name, ++current_inode_num, S_IFDIR,
                        current_uid, current_gid, WRITE_CTIME, 0, 1,
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
        populate_raw_inode(&ri, current_name, ++current_inode_num, S_IFREG,
                        current_uid, current_gid, WRITE_CTIME, 1, 1,
                        strlen(current_name), current_name);
        // print info then write to flash
        print_node_info(ri);
        fwrite(ri, 1, sizeof(*ri) + ri->data_size, fp);
        count++;

        /*
         * update "/" node
         * TODO ri->ctime is still have old value from previous inode, so
         * NOT_WRITE_CTIME is not doing anything
         */
        update_dir_table(&root_p, &root_p_size, ri->name, ri->inode_num);
        current_name = "/";
        populate_raw_inode(&ri, current_name, 1, S_IFDIR, current_uid,
                        current_gid, NOT_WRITE_CTIME, 0, 2, root_p_size, (char *)root_p);
        // print info then write to flash
        print_node_info(ri);
        fwrite(ri, 1, sizeof(*ri) + ri->data_size, fp);
        count++;
        ///////////////////////////////////////////////////////////////

        /*
         * new "bar" node
         */
        current_name = "bar";
        populate_raw_inode(&ri, current_name, ++current_inode_num, S_IFREG, current_uid, current_gid, WRITE_CTIME, 1, 1,
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
        populate_raw_inode(&ri, current_name, 1, S_IFDIR, current_uid, current_gid, NOT_WRITE_CTIME, 0, 3,
                        root_p_size, (char *)root_p);
        // print info then write to flash
        print_node_info(ri);
        fwrite(ri, 1, sizeof(*ri) + ri->data_size, fp);
        count++;
        ///////////////////////////////////////////////////////////////

        /*
         * new "dir1" node
         */
        current_name = "dir1";
        populate_raw_inode(&ri, current_name, ++current_inode_num, S_IFREG, current_uid, current_gid, WRITE_CTIME, 1, 1,
                        dir1_p_size, (char *)dir1_p);
        // print info then write to flash
        print_node_info(ri);
        fwrite(ri, 1, sizeof(*ri) + ri->data_size, fp);
        count++;

        /*
         * update "/" node
         */
        update_dir_table(&root_p, &root_p_size, ri->name, ri->inode_num);
        current_name = "/";
        populate_raw_inode(&ri, current_name, 1, S_IFDIR, current_uid, current_gid, NOT_WRITE_CTIME, 0, 4,
                        root_p_size, (char *)root_p);
        // print info then write to flash
        print_node_info(ri);
        fwrite(ri, 1, sizeof(*ri) + ri->data_size, fp);
        count++;
        ///////////////////////////////////////////////////////////////

        /*
         * new "dir2" node
         */
        current_name = "dir2";
        populate_raw_inode(&ri, current_name, ++current_inode_num, S_IFDIR, current_uid, current_gid, WRITE_CTIME, 4, 1,
                        dir2_p_size, (char *)dir2_p);
        // print info then write to flash
        print_node_info(ri);
        fwrite(ri, 1, sizeof(*ri) + ri->data_size, fp);
        count++;

        /*
         * update "dir1" node
         */
        update_dir_table(&dir1_p, &dir1_p_size, ri->name, ri->inode_num);
        current_name = "dir1";
        populate_raw_inode(&ri, current_name, 4, S_IFDIR, current_uid, current_gid, NOT_WRITE_CTIME, 1, 2,
                        dir1_p_size, (char *)dir1_p);
        // print info then write to flash
        print_node_info(ri);
        fwrite(ri, 1, sizeof(*ri) + ri->data_size, fp);
        count++;
        ///////////////////////////////////////////////////////////////

        /*
         * new "a" node
         */
        current_name = "a";
        populate_raw_inode(&ri, current_name, ++current_inode_num, S_IFREG, current_uid, current_gid, WRITE_CTIME, 4, 1,
                        strlen(current_name), current_name);
        // print info then write to flash
        print_node_info(ri);
        fwrite(ri, 1, sizeof(*ri) + ri->data_size, fp);
        count++;

        /*
         * update "dir1" node
         */
        update_dir_table(&dir1_p, &dir1_p_size, ri->name, ri->inode_num);
        current_name = "dir1";
        populate_raw_inode(&ri, current_name, 4, S_IFDIR, current_uid, current_gid, NOT_WRITE_CTIME, 1, 3,
                        dir1_p_size, (char *)dir1_p);
        // print info then write to flash
        print_node_info(ri);
        fwrite(ri, 1, sizeof(*ri) + ri->data_size, fp);
        count++;
        ///////////////////////////////////////////////////////////////

        /*
         * new "b" node
         */
        current_name = "b";
        populate_raw_inode(&ri, current_name, ++current_inode_num, S_IFREG, current_uid, current_gid, WRITE_CTIME, 4, 1,
                        strlen(current_name), current_name);
        // print info then write to flash
        print_node_info(ri);
        fwrite(ri, 1, sizeof(*ri) + ri->data_size, fp);
        count++;

        /*
         * update "dir1" node
         */
        update_dir_table(&dir1_p, &dir1_p_size, ri->name, ri->inode_num);
        current_name = "dir1";
        populate_raw_inode(&ri, current_name, 4, S_IFDIR, current_uid, current_gid, NOT_WRITE_CTIME, 1, 4,
                        dir1_p_size, (char *)dir1_p);
        // print info then write to flash
        print_node_info(ri);
        fwrite(ri, 1, sizeof(*ri) + ri->data_size, fp);
        count++;
        ///////////////////////////////////////////////////////////////

        /*
         * new "c" node
         */
        current_name = "c";
        populate_raw_inode(&ri, current_name, ++current_inode_num, S_IFREG, current_uid, current_gid, WRITE_CTIME, 4, 1,
                        strlen(current_name), current_name);
        // print info then write to flash
        print_node_info(ri);
        fwrite(ri, 1, sizeof(*ri) + ri->data_size, fp);
        count++;

        /*
         * update "dir2" node
         */
        update_dir_table(&dir2_p, &dir2_p_size, ri->name, ri->inode_num);
        current_name = "dir2";
        populate_raw_inode(&ri, current_name, 5, S_IFDIR, current_uid, current_gid, NOT_WRITE_CTIME, 4, 2,
                        dir2_p_size, (char *)dir2_p);
        // print info then write to flash
        print_node_info(ri);
        fwrite(ri, 1, sizeof(*ri) + ri->data_size, fp);
        count++;
        ///////////////////////////////////////////////////////////////



        printf("Total count: %d\n", count);

        free(ri);
        free(root_p);
        free(dir1_p);
        free(dir2_p);
        fclose(fp);
        return 0;
}
