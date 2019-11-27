#include "spinfs.h"
#include "spi_flash.h"
#include <errno.h>

/*
 * Global variables
 */
int fd_spi;
uint32_t head, tail;
uint32_t ht_slot;       /* determine position of newest head and tail in Sec Regs*/
struct inode_table_entry *itable; /* pointer to dynamic i-node table */
uint32_t itable_size;  /* does not count index 0, = array size - 1*/

#ifdef SIMULATED_FLASH
FILE* sim_main_file;
FILE* sim_head_file;
FILE* sim_tail_file;
#endif

#ifdef SIMULATED_FLASH
/*
 * Truncate the given file and write 0xFF to corresponding simlulated size
 */
void spinfs_create_sim_flash(char *file)
{
        if (strcmp(file, SIMULATED_MAIN_FILE_PATH) == 0) {
                printf("Formatting Main Flash (taking long time with Valgrind)...\n");

                /* truncate the file */
                sim_main_file = freopen(NULL, "w", sim_main_file);
                if (sim_main_file == NULL) {
                        perror("Sim main file open error");
                        exit(EXIT_FAILURE);
                }
                sim_main_file = freopen(NULL, "r+", sim_main_file);
                if (sim_main_file == NULL) {
                        perror("Sim main file open error");
                        exit(EXIT_FAILURE);
                }

                /* populate with 0xFF to simulate flash storage */
                for (int i = 0; i < MAIN_FLASH_SIZE; i++) {
                        fputc(0xFF, sim_main_file);
                }
        } else if (strcmp(file, SIMULATED_HEAD_FILE_PATH) == 0) {
                printf("Formatting Security Register 1.\n");

                /* truncate the file */
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

                /* populate with 0xFF to simulate flash storage */
                for (int i = 0; i < SEC_REG_SIZE; i++) {
                        fputc(0xFF, sim_head_file);
                }
        } else if (strcmp(file, SIMULATED_TAIL_FILE_PATH) == 0) {
                printf("Formatting Security Register 2.\n");

                /* truncate the file */
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

                /* populate with 0xFF to simulate flash storage */
                for (int i = 0; i < SEC_REG_SIZE; i++) {
                        fputc(0xFF, sim_tail_file);
                }
        }
}
#endif

int spinfs_init(int mkfs)
{
        itable = calloc(1, sizeof(*itable)); /* initialize index 0 to 0 */
#ifdef SIMULATED_FLASH
        /*
         * Open simulated flash files
         * If file not found, create it and format to 0xFF
         */
        sim_main_file = fopen(SIMULATED_MAIN_FILE_PATH, "r+");
        if (sim_main_file == NULL) {
                perror("Sim Main file open error");
                sim_main_file = fopen(SIMULATED_MAIN_FILE_PATH, "w");
                spinfs_create_sim_flash(SIMULATED_MAIN_FILE_PATH);
        }
        sim_head_file = fopen(SIMULATED_HEAD_FILE_PATH, "r+");
        if (sim_head_file == NULL) {
                perror("Sim head file open error");
                sim_head_file = fopen(SIMULATED_HEAD_FILE_PATH, "w");
                spinfs_create_sim_flash(SIMULATED_HEAD_FILE_PATH);
        }
        sim_tail_file = fopen(SIMULATED_TAIL_FILE_PATH, "r+");
        if (sim_tail_file == NULL) {
                perror("Sim tail file open error");
                sim_tail_file = fopen(SIMULATED_TAIL_FILE_PATH, "w");
                spinfs_create_sim_flash(SIMULATED_TAIL_FILE_PATH);
        }
#else
        fd_spi = spi_init();
#endif

        /* Skip scanning when doing mkfs */
        if(!mkfs) {
                spinfs_read_ht_slot();
                if (ht_slot == 0)
                        return -1;
                spinfs_read_head_tail();
                print_head_tail_info(__func__);

                spinfs_scan_fs();
        }
        return 0;
}

int spinfs_deinit()
{
        print_itable_info(__func__);
        free(itable);
#ifdef SIMULATED_FLASH
        fclose(sim_main_file);
        fclose(sim_head_file);
        fclose(sim_tail_file);
#else
        close(fd_spi);
#endif
        return 0;
}

void spinfs_read_ht_slot()
{
        /*
         * Get the most recent head and tail value based on the last valid
         * value in Security Registers
         */
        int sim_head_file_offset = 0;
        int sim_tail_file_offset = 0;
        unsigned char buf;
        for (sim_head_file_offset = SEC_REG_SIZE - sizeof(uint32_t);
                        sim_head_file_offset >= 0;
                        sim_head_file_offset -= sizeof(uint32_t)) {
#ifdef SIMULATED_FLASH
                fseek(sim_head_file, sim_head_file_offset, SEEK_SET);
                buf = fgetc(sim_head_file);
#else
                spi_read_sec_reg(SEC_REG_1_START_ADDR + sim_head_file_offset,
                                &buf, 1);
#endif
                if (buf != 0xFF) {
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
#ifdef SIMULATED_FLASH
                fseek(sim_tail_file, sim_tail_file_offset, SEEK_SET);
                buf = fgetc(sim_tail_file);
#else
                spi_read_sec_reg(SEC_REG_2_START_ADDR + sim_tail_file_offset,
                                &buf, 1);
#endif
                if (buf != 0xFF) {
                        break;
                }
                if (sim_tail_file_offset == 0) {
                        sim_tail_file_offset = -1;
                        break;
                }
        }

        /* Check to head and tail are written in pair and aligned to the same pair */
        if (sim_head_file_offset == sim_tail_file_offset) {
                if (sim_head_file_offset == -1)
                        ht_slot = 0;
                else
                        ht_slot = sim_head_file_offset / sizeof(uint32_t) + 1;
        } else {
                printf("WARNING: Head and tail have different slot!\n");
                printf("Security Register 1 offset 0x%x.\n", sim_head_file_offset);
                printf("Security Register 2 offset 0x%x.\n", sim_tail_file_offset);
                ht_slot = 0;
        }
}

/*
 * Receive head and tail value based on ht_slot
 */
void spinfs_read_head_tail()
{
        if (ht_slot > 0) {
#ifdef SIMULATED_FLASH
                fseek(sim_head_file, (ht_slot - 1) * sizeof(uint32_t), SEEK_SET);
                fread(&head, sizeof(uint32_t), 1, sim_head_file);
                fseek(sim_tail_file, (ht_slot - 1) * sizeof(uint32_t), SEEK_SET);
                fread(&tail, sizeof(uint32_t), 1, sim_tail_file);
#else
                spi_read_sec_reg(SEC_REG_1_START_ADDR
                                + (ht_slot - 1) * sizeof(uint32_t),
                                (unsigned char *)&head, sizeof(uint32_t));
                spi_read_sec_reg(SEC_REG_2_START_ADDR
                                + (ht_slot - 1) * sizeof(uint32_t),
                                (unsigned char *)&tail, sizeof(uint32_t));
#endif
        } else {
                head = 0;
                tail = 0;
        }
}

/*
 * Write new head, tail value to the Security Registers
 */
void spinfs_write_head_tail()
{
        /*
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
#else
        spi_write_sec_reg(SEC_REG_1_START_ADDR
                        + (ht_slot - 1) * sizeof(uint32_t),
                        (unsigned char *)&head, sizeof(uint32_t));
        spi_write_sec_reg(SEC_REG_2_START_ADDR
                        + (ht_slot - 1) * sizeof(uint32_t),
                        (unsigned char *)&tail, sizeof(uint32_t));
#endif
}

/*
 * Scan the whole filesystem to populate inode_table
 * Start from head, end at tail
 * TODO handle case when head and tail wrap around
 */
void spinfs_scan_fs()
{
        //printf("Scanning filesystem for inode_table\n\n");
        int addr = head;
        int count = 0;
        struct spinfs_raw_inode *s = malloc(sizeof(*s));
        /* TODO check head and tail wrap around */
        while (addr < tail) {
                //printf("Address: 0x%06x\n", addr);
                s = spinfs_read_inode(s, addr);
                //print_inode_info(s, __func__);
                spinfs_update_itable(s, addr);
                addr += sizeof(*s) + s->data_size;
                count++;
        }
        printf("\nTotal i-node count after scanning: %d\n", count);
        print_itable_info(__func__);
        free(s);
}

/*
 * TODO count obsolete i-nodes
 * TODO count deleted i-nodes
 */
void spinfs_update_itable(struct spinfs_raw_inode *i, uint32_t addr)
{
        //printf("\nUpdate i-node table -----------------------\n");
        //printf("        Current biggest i in i table: %d\n", itable_size);

        if (i->inode_num > itable_size) {
                //printf("        New entry: %d, 0x%06x, %d\n", i->inode_num, addr, i->version);
                itable_size = i->inode_num;     /* itable size is the current biggest inode */
                // allocate extra memories for higher inodes
                itable = realloc(itable, (itable_size + 1) * sizeof(*itable));
                //populate or update entry with current i metadata
                itable[i->inode_num].physical_addr = addr;
                itable[i->inode_num].version = i->version;
        } else if (itable[i->inode_num].version < i->version) {
                //printf("        Update old entry: %d, 0x%06x, %d\n", i->inode_num, addr, i->version);
                //populate or update entry with current i metadata
                itable[i->inode_num].physical_addr = addr;
                itable[i->inode_num].version = i->version;
        } else {
                //printf("        Current i %d entry in i table is more recent.\n", i->inode_num);
        }

        //printf("        New biggest i in i table: %d\n", itable_size);
        //printf("End of Update i table------------------------------------------\n\n");
}

void spinfs_erase_itable()
{
        itable = realloc(itable, sizeof(*itable));
        itable_size = 0;
}

uint32_t spinfs_get_next_avail_inum()
{
        return itable_size + 1;
}

/*
 * Check if i-node has correct magic numbers
 * TODO check CRC checksum
 */
int spinfs_check_valid_inode(struct spinfs_raw_inode *i)
{
        if ((i->magic1 != SPINFS_MAGIC1) ||
                (i->magic2 != SPINFS_MAGIC2)) {
                printf("Wrong i-node magic number!\n");
                return 1;
        }
        return 0;
}

struct spinfs_raw_inode *spinfs_read_inode(struct spinfs_raw_inode *i,
        uint32_t addr)
{
        i = realloc(i, sizeof(*i));     // allocate initial size
        if (i == NULL) {
                perror("Allocation error");
                exit(EXIT_FAILURE);
        }
        /*
         * Get i-node stem (without data)
         */
#ifdef SIMULATED_FLASH
        fseek(sim_main_file, addr, SEEK_SET);
        fread(i, 1, sizeof(*i), sim_main_file);
#else
        spi_read_data(addr, (unsigned char*)i, sizeof(*i));
#endif
        if (i->data_size > 0) {
                /* allocate extra memory for data[] */
                i = realloc(i, sizeof(*i) + i->data_size);
                /* copy extra data to the allocated struct */
#ifdef SIMULATED_FLASH
                fread(i->data, 1, i->data_size, sim_main_file);
#else
                spi_read_data(addr + sizeof(*i), (unsigned char*)i->data,
                                i->data_size);
#endif
        }

        if (spinfs_check_valid_inode(i)) {
                printf("File system corrupted with i-node at %d\n", addr);
                printf("Aborting...\n");
                spinfs_deinit();
                // TODO dump flash in case of corruption
                exit(EXIT_FAILURE);
        }
        return i;
}

struct spinfs_raw_inode *spinfs_get_inode_from_inum(struct spinfs_raw_inode *i,
        uint32_t inum)
{
        if (inum > itable_size) {
                errno = EINVAL;
                return NULL;
        }
        uint32_t addr = itable[inum].physical_addr;
        i = spinfs_read_inode(i, addr);
        return i;
}

/*
 * Append new node to the end of the main flash, update tail and write new
 * head, tail values to a new slot
 */
void spinfs_write_inode(struct spinfs_raw_inode *i)
{
        print_inode_info(i, __func__);
        if (tail + sizeof(*i) + i->data_size > MAIN_FLASH_SIZE) {
                printf("Fatal: Size to write too large!\n");
                exit(EXIT_FAILURE);
        }
#ifdef SIMULATED_FLASH
        //TODO handle case when write over size of flash
        fseek(sim_main_file, tail, SEEK_SET);
        fwrite(i, 1, sizeof(*i) + i->data_size, sim_main_file);
#else
        spi_write_data(tail, (unsigned char *)i, sizeof(*i) + i->data_size);
#endif
        spinfs_update_itable(i, tail);
        //TODO if tail > MAIN_FLASH_SIZE
        tail += sizeof(*i) + i->data_size;
        spinfs_write_head_tail();
        print_head_tail_info(__func__);
}

void spinfs_erase_fs()
{
#ifdef SIMULATED_FLASH
        spinfs_create_sim_flash(SIMULATED_MAIN_FILE_PATH);
#else
        /* FIXME Testing purpose: only erase the first block to save time */
        //spi_erase_chip();
        spi_erase_block(0);
#endif
}

/*
 * Erase both Security Register 1 and 2 storing head and tail value
 */
void spinfs_erase_sec_reg_1_2()
{
#ifdef SIMULATED_FLASH
        spinfs_create_sim_flash(SIMULATED_HEAD_FILE_PATH);
        spinfs_create_sim_flash(SIMULATED_TAIL_FILE_PATH);
#else
        spi_erase_sec_reg(SEC_REG_1_START_ADDR);
        spi_erase_sec_reg(SEC_REG_2_START_ADDR);
#endif
        ht_slot = 0;
}

void spinfs_format()
{
        spinfs_erase_fs();
        /*
         * Erase Security Registers 1 and 2, then reset head and tail in RAM
         */
        spinfs_erase_sec_reg_1_2();
        head = 0;
        tail = 0;
        print_head_tail_info(__func__);         // print info after erasing
        spinfs_erase_itable();
}

/*
 * Return the i-node number if name is in dir inode, 0 if not or inode is not dir
 * Return 1 if is not a directory, this is accepted since this function will
 * never return inum 1, which is root directory
 */
uint32_t spinfs_is_name_in_dir(struct spinfs_raw_inode *s, char *name)
{
        if (!S_ISDIR(s->mode)) {
                errno = ENOTDIR;
                return 1;       /* see comment */
        }
        int dirent_count = s->data_size / sizeof(struct dir_entry);
        /* loop through all dir entries and compare with name */
        for (int i = 0; i < dirent_count; i++) {
                if (strncmp(name,
                        ((struct dir_entry *)s->data)[i].name,
                        MAX_NAME_LEN) == 0) {
                        // file appears in dir table already ensures
                        // that file is not yet deleted
                        errno = EEXIST;
                        return ((struct dir_entry *)s->data)[i].inode_num;
                }
        }
        return 0;
}

/*
 * Return the i-node number if path is valid, 0 if not
 * TODO with current dir entry struct only 1 instance of name can be presented
 * in a dir table, even they are different types (i.e. REG and DIR cannot have
 * the same name).
 */
uint32_t spinfs_check_valid_path(char *path)
{
        uint32_t inum = 1;
        if ((strlen(path) <= 0) || (path[0] != '/')) {
                printf("SPINFS path has to be absolute!\n");
                errno = EINVAL;
                return 0;
        }

        struct spinfs_raw_inode *s = NULL;
        char *token = strtok(path, "/");
        if (token == NULL) {
                return 1;
        }
        while (token != NULL) {
                /*
                 * Check if token is in current inum
                 */
                s = spinfs_read_inode(s, itable[inum].physical_addr);
                /* inum now reflects the token i-node */
                inum = spinfs_is_name_in_dir(s, token);
                if (inum == 0) {   /* token is not found */
                        errno = ENOENT;
                        break;
                }

                /* get the next token in path */
                token = strtok(NULL, "/");
        }
        free(s);
        return inum;
}

int spinfs_get_dirent_index(struct dir_entry *t, int size, uint32_t inum)
{
        for (int i = 0; i < size; i++) {
                if (t[i].inode_num == inum) return i;
        }
        return -1;
}

void print_head_tail_info(const char *caller)
{
        int addr = (ht_slot - 1) * sizeof(uint32_t);
        printf("\nDebugging head, tail --------------------\n");
        printf("Caller: %s\n", caller);
        printf("Head and tail is at slot: %d, addr: 0x%x\n", ht_slot, addr);
        printf("Value of head: 0x%x, %d.\n", head, head);
        printf("Value of tail: 0x%x, %d.\n", tail, tail);
}

void print_itable_info(const char *caller)
{
        printf("\nDebugging i-node table ------------------\n");
        printf("Caller: %s\n", caller);
        printf("    I-node    |    Address     |   Version    \n");
        for (int i = 1; i <= itable_size; i++) {
                printf("     %4d         0x%06x           %4d   \n", i, itable[i].physical_addr, itable[i].version);
        }
}

void print_inode_info(struct spinfs_raw_inode *i, const char *caller)
{
        printf("\nDebugging i-node structure --------------\n");
        printf("Caller: %s\n", caller);
        int node_size =  sizeof(*i) + i->data_size;
        printf("Size of i-node %d, %.*s is: %d + %d = %d\n"
                , i->inode_num, MAX_NAME_LEN, i->name, sizeof(*i), i->data_size, node_size);
        printf("Raw metadata:\n");
        print_buffer((unsigned char *)i, sizeof(*i));
        /* Print details */
        printf("Magic 1             : %*.*s\n", MAX_NAME_LEN, 4, (char *)&i->magic1);
        printf("Name                : %*.*s\n", MAX_NAME_LEN, MAX_NAME_LEN, i->name);
        printf("I-node number       : %*d\n", MAX_NAME_LEN, i->inode_num);
        printf("Mode                : %*s\n", MAX_NAME_LEN,S_ISDIR(i->mode) ? "Directory" : "Regular File");
        printf("UID                 : %*d\n", MAX_NAME_LEN, i->uid);
        printf("GID                 : %*d\n", MAX_NAME_LEN, i->gid);
        printf("Creation time       : %*s", MAX_NAME_LEN, ctime(&(i->ctime)));
        printf("Modification time   : %*s", MAX_NAME_LEN, ctime(&(i->mtime)));
        printf("Flags               : %*s\n", MAX_NAME_LEN, F_ISDEL(i->flags) ? "DELETED" : "0");
        printf("Parent i-node number: %*d\n", MAX_NAME_LEN, i->parent_inode);
        printf("Version             : %*d\n", MAX_NAME_LEN, i->version);
        printf("Data size           : %*d\n", MAX_NAME_LEN, i->data_size);
        printf("Magic 2             : %*.*s\n", MAX_NAME_LEN, 4, (char *)&i->magic2);

        if (i->data_size > 0) {
                printf("\nRaw data segment:\n");
                print_buffer((unsigned char *)(i->data), i->data_size);
        }
}
