#include "spinfs.h"
#include "spi_flash.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>           // stat
#include <libgen.h>             // basename

void print_usage()
{
        fprintf(stderr, "####### Copy files to and from flash, /spinfs/ #######\n");
        fprintf(stderr, "Format: spinfs_cp src dest\n");
        fprintf(stderr, "Append /spinfs/ to tell the file is from flash\n");
        fprintf(stderr, "\n");
}


int main(int argc, char *argv[])
{
        if (argc != 2) {
                printf("Too many or too less arguments\n\n");
                print_usage();
                exit(EXIT_FAILURE);
        }
        print_usage();

        /*
         * Find source file size
         */
        struct stat src_st;
        if (stat(argv[1], &src_st) == -1) {
                perror("Stat file error");
                exit(EXIT_FAILURE);
        }
        if (!S_ISREG(src_st.st_mode)) {
                printf("Source is not a regular file.\n");
                exit(EXIT_FAILURE);
        }
        printf("File %s has size: %ld\n\n", argv[1], src_st.st_size);

        /*
         * Read content of file to buffer
         */
        FILE *src_file = fopen(argv[1], "r");
        if (src_file == NULL) {
                perror("File open error");
                exit(1);
        }

        spinfs_init();
        //spinfs_format();
        //spinfs_erase_sec_reg_1_2();

        //TODO check existence of new file name in target directory, if yes
        //modify the file with the same inode, if not write a new inode with
        //new inode_num
        //TODO get parent_inode based on the path of the destination

        char *dest = "/";
        uint32_t target_inode_num = 1;  //start traversing from root directory
        if (dest[0] != '/') {
                printf("Path needs to be absolute!\n");
                exit(EXIT_FAILURE);
        }
        printf("\nDestination path: %s\n", dest);


        char *src_basename = basename(argv[1]);


        struct spinfs_raw_inode *parent = malloc(sizeof(*parent));
        parent = spinfs_read_inode(parent, spinfs_get_inode_table_entry(target_inode_num).physical_addr);
        struct dir_entry *parent_dir_table = (struct dir_entry *)parent->data;
        int parent_dir_table_size = parent->data_size / sizeof(struct dir_entry);
        /*
         * Check existing file in parent directory
         */
        int dest_not_exist_flag = 1;
        int i = 0;
        for (i = 0; i < parent_dir_table_size; i++) {
                printf("i: %d\n", i);
                if (strncmp(src_basename, parent_dir_table[i].name, MAX_NAME_LEN) == 0) {
                        printf("Name matches existing file.\n");
                        //TODO handle case when matches existing file is a directory
                        dest_not_exist_flag = 0;
                        break;
                }
        }

        struct spinfs_raw_inode *new_inode = malloc(sizeof(*new_inode));

        /*
         * Destination file is not existed
         */
        if (dest_not_exist_flag) {

                new_inode = realloc(new_inode, sizeof(*new_inode) + src_st.st_size);

                new_inode->magic1 = SPINFS_MAGIC1;
                strncpy(new_inode->name, src_basename, MAX_NAME_LEN);
                new_inode->inode_num = get_inode_table_size() + 1;
                new_inode->uid = getuid();
                new_inode->gid = getgid();
                new_inode->mode = S_IFREG;
                new_inode->flags = 0;
                new_inode->ctime = time(NULL);
                new_inode->mtime = time(NULL);
                new_inode->parent_inode = parent->inode_num;
                new_inode->version = 1;
                new_inode->data_size = src_st.st_size;
                new_inode->magic2 = SPINFS_MAGIC2;
                if (new_inode->data_size > 0) {
                        fseek(src_file, 0, SEEK_SET);
                        fread(new_inode->data, 1, new_inode->data_size, src_file);
                }


                /*
                 * Adding new directory entry
                 */
                printf("Adding new directory entry in %.*s directory.\n", MAX_NAME_LEN, parent->name);
                parent_dir_table_size++;
                // Allocate extra memory for 1 more entry
                parent->data_size += sizeof(struct dir_entry);
                parent = realloc(parent, sizeof(*parent) + parent->data_size);

                // Assign new value for the new entry
                parent_dir_table = (struct dir_entry *)parent->data;
                strncpy(parent_dir_table[parent_dir_table_size - 1].name, new_inode->name, MAX_NAME_LEN);
                parent_dir_table[parent_dir_table_size - 1].inode_num = new_inode->inode_num;

        } else {
                printf("Destination file exists, modifying...\n");

                int dest_inode_num = parent_dir_table[i].inode_num;
                printf("inode: %d\n", dest_inode_num);
                new_inode = spinfs_read_inode(new_inode, spinfs_get_inode_table_entry(dest_inode_num).physical_addr);
                //print_node_info(new_inode);

                // Existing dest file must be regular
                if (!S_ISREG(new_inode->mode)) {
                        printf("Existing destination is not a regular file.\n");
                        exit(EXIT_FAILURE);
                }
                new_inode->uid = getuid();
                new_inode->gid = getgid();
                new_inode->mtime = time(NULL);
                new_inode->parent_inode = parent->inode_num;
                ++new_inode->version;
                new_inode->data_size = src_st.st_size;
                if (new_inode->data_size > 0) {
                        new_inode = realloc(new_inode, sizeof(*new_inode) + new_inode->data_size);
                        fread(new_inode->data, 1, new_inode->data_size, src_file);
                }

        }

        spinfs_write_inode(new_inode);

        // Update parent inode and write to flash
        parent->mtime = time(NULL);
        ++parent->version;
        printf("\n");
        print_node_info(parent);
        print_directory(parent);
        spinfs_write_inode(parent);




#if 0

        printf("\nUpdating parent inode------------------\n");
        struct spinfs_raw_inode *parent = spinfs_read_inode(NULL, inode_table[inode->parent_inode].physical_addr);
        print_node_info(parent);
        print_directory(parent);


        //get dirent and update dirent, update version, write back
        struct dir_entry *parent_dir_table = (struct dir_entry *)parent->data;
        int parent_dir_table_size = parent->data_size / sizeof(struct dir_entry);
        // check child file is already present
        // TODO if file delete, remove entry from dir table, check inode mode DELETED flag
        int dest_not_exist_flag = 1;
        for (int i = 0; i < parent_dir_table_size; i++) {
                printf("i: %d\n", i);
                if (strncmp(inode->name, parent_dir_table[i].name, MAX_NAME_LEN) == 0) {
                        printf("Name matches existing file.\n");
                        dest_not_exist_flag = 0;
                        break;
                }
        }
        // Adding new directory entry
        if (dest_not_exist_flag == 1) {
                printf("Adding new directory entry in %.32s directory.\n", parent->name);
                parent_dir_table_size++;
                parent->data_size += sizeof(struct dir_entry);
                parent = realloc(parent, sizeof(*parent) + parent->data_size);

                parent_dir_table = (struct dir_entry *)parent->data;
                strncpy(parent_dir_table[parent_dir_table_size - 1].name, inode->name, MAX_NAME_LEN);
                parent_dir_table[parent_dir_table_size - 1].inode_num = inode->inode_num;
        }

        //Update parent inode metadata
        parent->mtime = time(NULL);
        parent->version++;
        printf("\n");
        print_node_info(parent);
        print_directory(parent);

        //Write new parent inode structure to flash
#ifdef SIMULATED_FLASH
        fseek(sim_main_file, tail, SEEK_SET);
        fwrite(parent, 1, sizeof(*parent) + parent->data_size, sim_main_file);
#endif

        //Update inode_table, tail, head accordingly
        spinfs_update_inode_table(parent, tail);
        //TODO if tail > MAIN_FLASH_SIZE
        tail += sizeof(*inode) + parent->data_size;
        write_head_tail();
        print_head_tail_info();

        free(parent);
        printf("End of Updating parent inode------------------\n\n");
#if 1
        struct spinfs_raw_inode *new_inode =
                malloc(sizeof(*new_inode) + src_st.st_size);
        new_inode->magic1 = SPINFS_MAGIC1;
        strncpy(new_inode->name, basename(argv[1]), MAX_NAME_LEN);
        new_inode->inode_num = 2;
        new_inode->uid = getuid();
        new_inode->gid = getgid();
        new_inode->mode = S_IFREG;
        new_inode->flags = 0;
        new_inode->ctime = time(NULL);
        new_inode->mtime = time(NULL);
        new_inode->parent_inode = 1;
        new_inode->version = 2;
        new_inode->magic2 = SPINFS_MAGIC2;
        new_inode->data_size = src_st.st_size;
        if (new_inode->data_size > 0)
                //memcpy(new_inode->data, data, data_size);
                fread(new_inode->data, 1, new_inode->data_size, src_file);
        spinfs_write_inode(new_inode);

        free(new_inode);
#endif
#endif






        free(new_inode);
        free(parent);
        fclose(src_file);
        spinfs_deinit();
        return 0;
}
