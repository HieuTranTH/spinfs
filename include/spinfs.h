#include <stdint.h>
//#include <time.h> /* time_t */

#define SPIN_FS_MAGIC1 0x5350494e   /* SPIN */
#define SPIN_FS_MAGIC2 0x46537631   /* FSv1 */

struct spinfs_raw_inode {
    uint32_t magic1;
    char name[32];
    uint32_t inode_num;
    /*
     * Will be implemented later
     */
    /*
    int32_t uid;
    int32_t gid;
    int32_t mode;
    int32_t flags;
    time_t ctime;
    time_t mtime;
    */
    uint32_t parent_inode;
    uint32_t version;
    uint32_t data_size;
    uint32_t magic2;
    /*
     * Remember padding if needed
     */
    char data[];
};

struct dir_entry {
    char name[32];
    uint32_t inode_num;
};

extern uint32_t head, tail; // might not need to be declared here

int update_head_tail(uint32_t head_n, uint32_t tail_n);

void print_node_info(struct spinfs_raw_inode *ri);

/*
int32_t spinfs_init(void);
int32_t spinfs_uninit(void);
int32_t spinfs_sync(void);
int32_t spinfs_mount(const char *volume);
int32_t spinfs_unmount(const char *volume);
int32_t spinfs_format(const char *volume);
int32_t spinfs_format();

struct spinfs_raw_inode *createInode(unsigned char *d_buf, uint32_t d_size);
*/

#if 0
struct test_str {
    int i;
    char a;
    int m[];
};
#endif