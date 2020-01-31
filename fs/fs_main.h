#ifndef __FS_MAIN__
#define __FS_MAIN__

#define FUSE_USE_VERSION 31
#include "global_types.h"
#include "metadata.h"
#ifdef MONITOR
#include "monitor.h"
//extern struct monitor *global_monitor;
#endif

#define K 1024
#define M 1024 * K
#define G 1024 * M
#define T 1024L * G

#define SUPER_INIT_BN 0
#define D_BITMAP_INIT_BN 1
#define INODE_INIT_BN 76
#define DATA_INIT_BN (INODE_INIT_BN + 100 * K)
#define DIRECT_PTR 40
#define INDIRECT_PTR 6
#define D_INDIRECT_PTR 2
#define ROOT_DIR 0

#define INVALID 0
#define VALID 1


typedef struct superblock {
	FILE *fp;
	uint32_t root_directory;
	uint32_t total_block_size;
	uint32_t d_bitmap_init_bn;
	uint32_t inode_init_bn;
	uint32_t list_first;
	uint32_t free_inode;
	uint32_t free_d_block;
	uint32_t cur_bit_bn;
	struct d_bitmap *cur_bit;
	char reserve[PAGESIZE - 52];
}superblock;

typedef struct inode {

	struct metadata attr;
	uint32_t direct_ptr[DIRECT_PTR];
	uint32_t indirect_ptr[INDIRECT_PTR];
	uint32_t d_indirect_ptr[D_INDIRECT_PTR];

}inode;

typedef struct i_block {
	struct inode i[PAGESIZE / sizeof(struct inode)];
}i_block;

typedef struct d_bitmap {
	char bitset[PAGESIZE];
}d_bitmap;
typedef struct free_list {
	int32_t free_node[(PAGESIZE / 4) - 1];
	int32_t next;
}free_list;


#endif
