#include "spinfs.h"
#include "spi_flash.h"
//#include <stdio.h>

/*
        root_inode.magic1 = ;
        root_inode.name = ;
        root_inode.inode_num = ;
        root_inode.uid = ;
        root_inode.gid = ;
        root_inode.mode = ;
        root_inode.flags = ;
        root_inode.ctime = ;
        root_inode.mtime = ;
        root_inode.parent_inode = ;
        root_inode.version = ;
        root_inode.magic2 = ;
        root_inode.data_size = ;
*/

/*
 * Global variables
 */
uint32_t head, tail;
uint32_t ht_slot;       /* determine position of newest head and tail in Sec Regs*/
uint32_t inode_table_size;  // does not count index 0, = array size - 1
struct inode_table_entry *inode_table;

#ifdef SIMULATED_FLASH
FILE* sim_main_file;
FILE* sim_head_file;
FILE* sim_tail_file;
#endif


void spinfs_init()
{
        inode_table = calloc(1, sizeof(*inode_table));

#ifdef SIMULATED_FLASH
        /*
         * Open simulated flash files
         * If file not found, create it and format to 0xFF
         */
        sim_main_file = fopen(SIMULATED_MAIN_FILE_PATH, "r+");
        if (sim_main_file == NULL) {
                perror("Sim Main file open error");
                sim_main_file = fopen(SIMULATED_MAIN_FILE_PATH, "w");
                sim_create_flash_file(SIMULATED_MAIN_FILE_PATH);
        }
        sim_head_file = fopen(SIMULATED_HEAD_FILE_PATH, "r+");
        if (sim_head_file == NULL) {
                perror("Sim head file open error");
                sim_head_file = fopen(SIMULATED_HEAD_FILE_PATH, "w");
                sim_create_flash_file(SIMULATED_HEAD_FILE_PATH);
        }
        sim_tail_file = fopen(SIMULATED_TAIL_FILE_PATH, "r+");
        if (sim_tail_file == NULL) {
                perror("Sim tail file open error");
                sim_tail_file = fopen(SIMULATED_TAIL_FILE_PATH, "w");
                sim_create_flash_file(SIMULATED_TAIL_FILE_PATH);
        }
#endif

        read_head_tail();
        print_head_tail_info();

        spinfs_scan_for_inode_table();
}

void spinfs_scan_for_inode_table() //TODO
{
#ifdef SIMULATED_FLASH

        int addr = head;
        int count = 0;
        struct spinfs_raw_inode *s = malloc(sizeof(*s));
        //TODO check head and tail wrap around
        while (addr < tail) {
                printf("Address: 0x%06x\n", addr);
                s = spinfs_read_inode(s, addr);
                print_node_info(s);
                spinfs_update_inode_table(s, addr);
                addr += sizeof(*s) + s->data_size;
                count++;
        }
        printf("Total count: %d\n", count);
        print_inode_table(inode_table);
        free(s);
#endif
}

void spinfs_deinit()
{
        print_inode_table(inode_table);
        free(inode_table);
#ifdef SIMULATED_FLASH
        fclose(sim_main_file);
        fclose(sim_head_file);
        fclose(sim_tail_file);
#endif
}

/*
 * ******************************************************
 * inode table functions
 * ******************************************************
 */

void spinfs_update_inode_table(struct spinfs_raw_inode *inode, uint32_t addr)
{
        printf("        Current biggest inode in inode table: %d\n", inode_table_size);

        if (inode->inode_num > inode_table_size) {
                printf("        New entry: %d, 0x%06x, %d\n", inode->inode_num, addr, inode->version);
                inode_table_size = inode->inode_num;
                // allocate extra memories for higher inodes
                inode_table = realloc(inode_table, (inode_table_size + 1) * sizeof(*inode_table));
        }
        else {
                printf("        Update old entry: %d, 0x%06x, %d\n", inode->inode_num, addr, inode->version);
        }
        //populate or update entry with current inode metadata
        inode_table[inode->inode_num].physical_addr = addr;
        inode_table[inode->inode_num].version = inode->version;

        printf("        New biggest inode in inode table: %d\n", inode_table_size);
}

/*
 * ******************************************************
 * head, tail functions
 * ******************************************************
 */

/*
 * Load head and tail values from Security Registers
 */
void read_head_tail()
{
#ifdef SIMULATED_FLASH
        /*
         * Get the most recent head and tail value based on the last valid
         * value in Security Registers
         */
        int sim_head_file_offset = 0;
        int sim_tail_file_offset = 0;
        for (sim_head_file_offset = SEC_REG_SIZE - sizeof(uint32_t);
                        sim_head_file_offset >= 0;
                        sim_head_file_offset -= sizeof(uint32_t)) {
                fseek(sim_head_file, sim_head_file_offset, SEEK_SET);
                if (fgetc(sim_head_file) != 0xFF) {
                        break;
                }
                if (sim_head_file_offset == 0) {
                        sim_head_file_offset = -1;
                        break;
                }
        }
        for (sim_tail_file_offset = SEC_REG_SIZE - sizeof(uint32_t);
                        sim_tail_file_offset >= 0;
                        sim_tail_file_offset -= sizeof(uint32_t)) {
                fseek(sim_tail_file, sim_tail_file_offset, SEEK_SET);
                if (fgetc(sim_tail_file) != 0xFF) {
                        break;
                }
                if (sim_tail_file_offset == 0) {
                        sim_tail_file_offset = -1;
                        break;
                }
        }

        printf("Security Register 1 offset 0x%x.\n", sim_head_file_offset);
        printf("Security Register 2 offset 0x%x.\n", sim_tail_file_offset);

        // Check to head and tail are written in pair and aligned to the same pair
        if (sim_head_file_offset == sim_tail_file_offset) {
                if (sim_head_file_offset == -1)
                        ht_slot = 0;
                else
                        ht_slot = sim_head_file_offset / sizeof(uint32_t) + 1;
        } else {
                printf("Head and tail have different slot!\n");
                exit(EXIT_FAILURE);
        }

        /*
         * Receive head and tail value based on ht_slot
         */
        if (ht_slot > 0) {
                fseek(sim_head_file, (ht_slot - 1) * sizeof(uint32_t), SEEK_SET);
                fseek(sim_tail_file, (ht_slot - 1) * sizeof(uint32_t), SEEK_SET);
                fread(&head, sizeof(uint32_t), 1, sim_head_file);
                fread(&tail, sizeof(uint32_t), 1, sim_tail_file);
        } else {
                head = 0;
                tail = 0;
        }
#endif
}

uint32_t get_ht_slot()
{
#ifdef SIMULATED_FLASH
#endif
        return ht_slot;
}
uint32_t get_head()
{
#ifdef SIMULATED_FLASH
#endif
        return head;
}
uint32_t get_tail()
{
#ifdef SIMULATED_FLASH
#endif
        return tail;
}

void print_head_tail_info()
{
        printf("Head and tail is at slot: %d\n", ht_slot);
        printf("Value of head: %d.\n", head);
        printf("Value of tail: %d.\n\n", tail);
}

void set_head_tail(uint32_t head_new, uint32_t tail_new) //TODO will be removed
{
        head = head_new;
        tail = tail_new;
        write_head_tail();
}

void write_head_tail()
{
        /*
         * Write new value to the simulated Security Registers
         * If ht_slot is 64 (i.e. Sec Reg is full), truncate the file
         */
        if (ht_slot == 64) {
                spinfs_erase_sec_reg_1_2();
        }

        ht_slot++;
#ifdef SIMULATED_FLASH
        fseek(sim_head_file, (ht_slot - 1) * sizeof(uint32_t), SEEK_SET);
        fwrite(&head, sizeof(uint32_t), 1, sim_head_file);
        fseek(sim_tail_file, (ht_slot - 1) * sizeof(uint32_t), SEEK_SET);
        fwrite(&tail, sizeof(uint32_t), 1, sim_tail_file);
#endif
}

#ifdef SIMULATED_FLASH
/*
 * Truncate the given file and write 0xFF to corresponding simlulated size
 */
void sim_create_flash_file(char *file)
{
        if (strcmp(file, SIMULATED_MAIN_FILE_PATH) == 0) {
                printf("Formatting Main Flash (taking long time)...\n");

                sim_main_file = freopen(NULL, "w", sim_main_file);
                if (sim_main_file == NULL) {
                        perror("Sim Main file open error");
                        exit(EXIT_FAILURE);
                }
                sim_main_file = freopen(NULL, "r+", sim_main_file);
                if (sim_main_file == NULL) {
                        perror("Sim Main file open error");
                        exit(EXIT_FAILURE);
                }

                for (int i = 0; i < MAIN_FLASH_SIZE; i++) {
                        fputc(0xFF, sim_main_file);
                }
        } else if (strcmp(file, SIMULATED_HEAD_FILE_PATH) == 0) {
                printf("Formatting Security Register 1.\n");

                sim_head_file = freopen(NULL, "w", sim_head_file);
                if (sim_head_file == NULL) {
                        perror("Sim head file open error");
                        exit(EXIT_FAILURE);
                }
                sim_head_file = freopen(NULL, "r+", sim_head_file);
                if (sim_head_file == NULL) {
                        perror("Sim head file open error");
                        exit(EXIT_FAILURE);
                }

                for (int i = 0; i < SEC_REG_SIZE; i++) {
                        fputc(0xFF, sim_head_file);
                }
        } else if (strcmp(file, SIMULATED_TAIL_FILE_PATH) == 0) {
                printf("Formatting Security Register 2.\n");

                sim_tail_file = freopen(NULL, "w", sim_tail_file);
                if (sim_tail_file == NULL) {
                        perror("Sim tail file open error");
                        exit(EXIT_FAILURE);
                }
                sim_tail_file = freopen(NULL, "r+", sim_tail_file);
                if (sim_tail_file == NULL) {
                        perror("Sim tail file open error");
                        exit(EXIT_FAILURE);
                }

                for (int i = 0; i < SEC_REG_SIZE; i++) {
                        fputc(0xFF, sim_tail_file);
                }
        }
}
#endif

/*
 * Erase both Security Register 1 and 2 storing head and tail value
 */
void spinfs_erase_sec_reg_1_2()
{
#ifdef SIMULATED_FLASH
        sim_create_flash_file(SIMULATED_HEAD_FILE_PATH);
        sim_create_flash_file(SIMULATED_TAIL_FILE_PATH);
#endif
        ht_slot = 0;
}

int32_t spinfs_format()
{
        /*
         * Erase main flash
         */
#ifdef SIMULATED_FLASH
        //taking long time so dont do it
        sim_create_flash_file(SIMULATED_MAIN_FILE_PATH);
#endif

        /*
         * Erase Security Registers 1 and 2, then reset head and tail in RAM
         */
        spinfs_erase_sec_reg_1_2();
        head = 0;
        tail = 0;

        print_head_tail_info();         // print info after erasing

        /*
         * Write root directory inode
         */
        struct spinfs_raw_inode *root_inode = malloc(sizeof(*root_inode));
        root_inode->magic1 = SPINFS_MAGIC1;
        strncpy(root_inode->name, "/", MAX_NAME_LEN);
        root_inode->inode_num = 1;
        root_inode->uid = getuid();
        root_inode->gid = getgid();
        root_inode->mode = S_IFDIR;
        root_inode->flags = 0;
        root_inode->ctime = time(NULL);
        root_inode->mtime = time(NULL);
        root_inode->parent_inode = 0;
        root_inode->version = 1;
        root_inode->magic2 = SPINFS_MAGIC2;
        root_inode->data_size = 0;
        spinfs_write_inode(root_inode);
        free(root_inode);

        //TODO clear inode_table
        inode_table = realloc(inode_table, sizeof(*inode_table));
        inode_table_size = 0;
        spinfs_scan_for_inode_table();

        return 0;
}

/*
 * Append new node to the end of the main flash, update tail and write new
 * head, tail values to a new slot
 */
void spinfs_write_inode(struct spinfs_raw_inode *inode)
{

#ifdef SIMULATED_FLASH
        fseek(sim_main_file, tail, SEEK_SET);
        fwrite(inode, 1, sizeof(*inode) + inode->data_size, sim_main_file);
#endif
        spinfs_update_inode_table(inode, tail);
        //TODO if tail > MAIN_FLASH_SIZE
        tail += sizeof(*inode) + inode->data_size;
        write_head_tail();
        print_head_tail_info();

        //update parent inode
        if (inode->parent_inode != 0) {
                spinfs_update_parent_inode(inode);
        }
}

void spinfs_update_parent_inode(struct spinfs_raw_inode *inode)
{
        struct spinfs_raw_inode *parent = spinfs_read_inode(NULL, inode_table[inode->parent_inode].physical_addr);
        print_node_info(parent);
        //TODO get dirent and update dirent, update version, write back

        free(parent);
}

struct spinfs_raw_inode *spinfs_read_inode(struct spinfs_raw_inode *inode, uint32_t addr)
{
#ifdef SIMULATED_FLASH
        //fwrite(inode, 1, sizeof(*inode) + inode->data_size, sim_main_file);
        //struct spinfs_raw_inode holder;
        inode = realloc(inode, sizeof(*inode));     // allocate initial size
        /*
         * Get inode stem (without data)
         */
        fseek(sim_main_file, addr, SEEK_SET);
        fread(inode, 1, sizeof(*inode), sim_main_file);
        if (inode->data_size > 0) {
                inode = realloc(inode, sizeof(*inode) + inode->data_size);     // allocate extra memory for data[]
                fread(inode->data, 1, inode->data_size, sim_main_file);                    // copy extra data to the allocated struct
        }
#endif

        return inode;
}










/*
 * Update head and tail by pair to SEC_REG 1 and 2
 * Need to identify current head_tail_addr
 */
int update_head_tail(uint32_t head_n, uint32_t tail_n)
{
        return 0;
}

void print_node_info(struct spinfs_raw_inode *ri)
{
        int node_size =  sizeof(*ri) + ri->data_size;
        /*
         * There might a bug here when printing %s with ri->name when ri->name
         * is 32-bit length and there is no NULL character
         * Solution: use precision %.32s
         */
        printf("Size of i-node %d, %.*s is: %d + %d = %d\n"
                , ri->inode_num, MAX_NAME_LEN, ri->name, sizeof(*ri), ri->data_size, node_size);
        print_buffer((unsigned char *)ri, node_size);
        printf("\n");
}

void print_inode_table(struct inode_table_entry *it)
{

        printf("    I-node    |    Address     |   Version    \n");
        for (int i = 1; i <= inode_table_size; i++) {
                printf("     %4d         0x%06x           %4d   \n", i, it[i].physical_addr, it[i].version);
        }
}

void ls_file(struct spinfs_raw_inode *inode)
{
        printf("Output:\n");
        printf("%.*s\n", MAX_NAME_LEN, inode->name);
}

void print_directory(struct spinfs_raw_inode *inode)
{
        // TODO check inode is a directory

        int dir_file_count = inode->data_size / sizeof(struct dir_entry);
        printf("Directory %.*s", MAX_NAME_LEN, inode->name);
        if (dir_file_count == 0) {
                printf(" is empty.\n");
                return;
        }
        printf(" has %d file(s).\n", dir_file_count);
        printf("    Entry index    |    File name     |   I-node number    \n");
        for (int i = 0; i < dir_file_count; i++) {
                printf("     %4d                   %.*s                %4d   \n",
                                i, MAX_NAME_LEN,
                                ((struct dir_entry *)inode->data)[i].name,
                                ((struct dir_entry*)inode->data)[i].inode_num);
        }
}

uint32_t find_file_in_dir(struct spinfs_raw_inode *inode, char *filename)
{
        // TODO check inode is a directory
        if (S_ISREG(inode->mode)) {
                printf("%.*s is a regular file.\n", MAX_NAME_LEN, inode->name);
                return 0;
        }
        else if (S_ISDIR(inode->mode)) {
                printf("%.*s is a directory.\n", MAX_NAME_LEN, inode->name);
        }

        int dir_file_count = inode->data_size / sizeof(struct dir_entry);
        for (int i = 0; i < dir_file_count; i++) {
                printf("Index %d, %.*s, target %s :", i, MAX_NAME_LEN, ((struct dir_entry *)inode->data)[i].name, filename);
                if (strncmp(filename, ((struct dir_entry *)inode->data)[i].name, MAX_NAME_LEN) == 0) {
                        printf("matched.\n");
                        printf("\n");
                        return ((struct dir_entry *)inode->data)[i].inode_num;
                } else {
                        printf("unmatched.\n");
                }
        }
        printf("\n");
        return 0;
}

void spinfs_scan_fs(uint32_t head, uint32_t tail)
{
}

/*
 * Return inode structure from a physical address
 */
void spinfs_get_inode_at_addr(struct spinfs_raw_inode *s, uint32_t addr)
{
        //struct spinfs_raw_inode holder;
        s = realloc(s, sizeof(*s));
}

/*
struct spinfs_raw_inode *createInode(unsigned char *d_buf, uint32_t d_size)
{
        struct spinfs_raw_inode *i = malloc(sizeof(*i) + d_size);         //need to free manually

        strncpy(i->name, "This_is_the_name_of_root_directory", 32);
        i->inode_num = 255;
        i->parent_inode_num = 0;
        i->data_size = 0;
        i->version = 1;
        //code to memcpy d_buf to data member
        if (d_buf != NULL) {
                memcpy(i->data, d_buf, d_size);
        }
        return i;
}
*/

/*
 * Copy a file to the end of flash
 */
/*
void spinfs_cp(FILE *f)
{

}
/**/

#if 0
int main()
{
        //spi_init();
        printf("Size of struct: %d\n", sizeof(struct test_str));
        struct test_str arr[2];
        printf("Addr of arr: %p\n", arr);
        printf("Addr of arr + 1: %p\n", arr + 1);
        printf("Addr of arr->m: %p\n", &(arr->m));
        printf("Addr of arr[0]: %p\n", &arr[0]);
        printf("Addr of arr[1]: %p\n", &arr[1]);
        printf("Addr of arr[0].m: %p\n", &(arr[0].m));
        struct test_str *p = malloc(2*(sizeof(*p) + sizeof(int)));
        printf("Addr of p: %p\n", p);
        printf("Addr of p + 1: %p\n", p + 1);
        printf("Size of object: %d\n", sizeof(*p) + sizeof(int));

        free(p);
        return 0;
}
#endif

#if 0
int main()
{
        head = 0x000000;
        tail = 0x000000;
        struct spinfs_raw_inode *root_inode = createInode(NULL, 0);
        //root_inode->name[0] = '/';
        strncpy(root_inode->name, "This_is_the_name_of_root_directory", 32);
        root_inode->inode_num = 255;
        root_inode->parent_inode_num = 0;
        root_inode->data_size = 0;
        root_inode->version = 1;

        printf("Current i-node size: %d bytes.\n", sizeof(*root_inode));
        print_buffer((unsigned char*)root_inode, sizeof(*root_inode));

        int fd_spi = spi_init();
        //spi_write_data(0x020000, (unsigned char*)root_inode, sizeof(*root_inode));
        spi_close(fd_spi);
        free(root_inode);
        return 0;
}
#endif

