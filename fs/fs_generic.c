#define _LARGEFILE64_SOURCE

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include "fs_generic.h"
#include "metadata.h"
#include "fs_dir.h"
#include "fs_file.h"

struct monitor *global_monitor;
struct superblock spb;


int fs_getattr (const char *path, struct stat *stbuf, struct fuse_file_info *fi) {

    return 0;
}

int fs_utimens (const char *path, const struct timespec ts[2], struct fuse_file_info *fi) {

	return 0;

}

int fs_chmod (const char *path, mode_t mode, struct fuse_file_info *fi) {

    return 0;
}

int fs_chown (const char *path, uid_t uid, gid_t gid, struct fuse_file_info *fi) {

    return 0;
}

int fs_rename (const char *oldpath, const char *newpath, unsigned int flags) {

    return 0;
}

int fs_access (const char *path, int mask) {

	return 0;
}

int fs_symlink (const char *from, const char *to) {

	return 0;
}

int fs_readlink (const char *path, char *buf, size_t size) {

	return 0;
}


void *fs_init (struct fuse_conn_info *conn, struct fuse_config *cfg) {
	cfg->use_ino = 1;
	cfg->kernel_cache = 1;
	cfg->negative_timeout = 120;
	cfg->attr_timeout = 240;
	cfg->entry_timeout = 240;
	conn->want |= FUSE_CAP_WRITEBACK_CACHE;

#ifdef MONITOR
	monitor_init(&global_monitor);
#endif

  super_init();
  for(int i = 0; i < 256; i++)
    for(int j = 0; j < 4; j++)
      printf("%d", *(int *)&spb.cur_bit->bitset[i * 16 + j * 4]);
    printf("\n");

  pread(spb.fp, (char *)spb.cur_bit, PAGESIZE, (D_BITMAP_INIT_BN + 3) * PAGESIZE);


	fs_mkdir("/", 0755);

	return NULL;
}

void fs_destroy (void *private_data) {
#ifdef MONITOR
	monitor_print(global_monitor);
	monitor_destroy(global_monitor);
#endif

	return;
}
void super_init() {
  spb.fp = open("a", O_RDWR | O_CREAT | O_LARGEFILE, 0644);
  spb.root_directory = ROOT_DIR;
  spb.total_block_size = DEVSIZE;
  spb.d_bitmap_init_bn = D_BITMAP_INIT_BN;
  spb.inode_init_bn = INODE_INIT_BN;
  spb.free_inode = (DATA_INIT_BN - INODE_INIT_BN) * (PAGESIZE / sizeof(struct inode));
  spb.free_d_block = DEVSIZE - DATA_INIT_BN;
  bitmap_init();
  free_list_init();

  super_update();
}

void super_update() {
  pwrite(spb.fp, (char *)&spb, PAGESIZE, SUPER_INIT_BN);
}

void super_read() {
  pread(spb.fp, (char *)&spb, PAGESIZE, SUPER_INIT_BN);
}

void bitmap_init() {
	struct d_bitmap *bit = (struct d_bitmap *)calloc(1, sizeof(struct d_bitmap));
	int i;

	for (i = D_BITMAP_INIT_BN; i < INODE_INIT_BN; i++)
    bitmap_write(bit, i);

	spb.cur_bit = bit;
  spb.cur_bit_bn = 0;
}

void free_list_init() {
  free_list ll;
  int i, node_num = PAGESIZE / sizeof(uint32_t) - 2, i_num = spb.free_inode - 2, d_blk = 0;
  ll.next = -1;
  while(i_num > 1023) {
    for(i = node_num; i >= 0; --i)
      ll.free_node[i] = i_num--;
    bitmap_update(d_blk, VALID);
    data_write((void *)&ll, d_blk);
    ll.next = d_blk++;
  }
  for(i = node_num; i >= 0 && i_num >= 0; --i)
    ll.free_node[i] = i_num--;
  bitmap_update(d_blk, VALID);
  data_write((void *)&ll, d_blk);
  spb.list_first = d_blk;
}

void bitmap_read(d_bitmap *bitmap, uint32_t block_num) {
  pread(spb.fp, (char *)bitmap,  PAGESIZE, (block_num + D_BITMAP_INIT_BN) * PAGESIZE);
}

void bitmap_write(d_bitmap *bitmap, uint32_t block_num) {
  pwrite(spb.fp, (char *)bitmap, PAGESIZE, (block_num + D_BITMAP_INIT_BN) * PAGESIZE);
}

void bitmap_update(uint32_t data_block_num, char type) {
  uint32_t block_num = data_block_num / (PAGESIZE * sizeof(char));
  uint32_t bit_idx = data_block_num % (PAGESIZE * sizeof(char));
  if(block_num != spb.cur_bit_bn) {
    bitmap_write(spb.cur_bit, spb.cur_bit_bn);
    bitmap_read(spb.cur_bit, block_num);
  }
  if(type == VALID)
    spb.cur_bit->bitset[bit_idx / (PAGESIZE / sizeof(char))] |= (1 << (bit_idx % sizeof(char)));
  else
    spb.cur_bit->bitset[bit_idx / (PAGESIZE / sizeof(char))] &= ~(1 << (bit_idx % sizeof(char)));
}

void data_read(void *data, uint32_t block_num) {
  pread(spb.fp, (char *)data, PAGESIZE, (block_num + DATA_INIT_BN) * PAGESIZE);
}

void data_write(void *data, uint32_t block_num) {
  pwrite(spb.fp, (char *)data, PAGESIZE, (block_num + DATA_INIT_BN) * PAGESIZE);
}


/*
void free_list_init() {
	spb.list_first =
}
*/
