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

#define BIT_CHECK(block, idx) (block & (1 << idx))
#define BIT_SET(block, idx) block |= (1 << idx)
#define BIT_CLEAR(block, idx) block &= ~(1 << idx)
#define GET_BLOCKNUM(block_num, unit) block_num / unit
#define GET_BITIDX(block_num, unit) block_num % unit

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
  /*
  int p;
  for(int i = 0; i < 64; i++){
    for(int j = 0; j < 4; j++) {
      p = 0;
      for(int k = 0; k < 8; k++) {
        p |= spb.cur_bit->bitset[i * 16 + j *4] & (1 << k);
      }
      printf("%d",p);
    }
    printf("\n");
  }

  bitmap_read(spb.cur_bit, 0);
  bitmap_update(3, VALID);
*/
  int arr[3];
  printf("result : %d\n", search_bitmap(arr, 3));
  printf("%d %d %d\n", arr[0], arr[1], arr[2]);
	fs_mkdir("a/b/c/d/e/f/g", 0755);

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
  spb.total_d_blocks = (DEVSIZE / PAGESIZE) - DATA_INIT_BN;
  bitmap_init();
  free_list_init();

  super_write();
}
void bitmap_init() {
	struct d_bitmap *bit = (struct d_bitmap *)calloc(1, sizeof(struct d_bitmap));
	int i;

	for (i = D_BITMAP_INIT_BN; i < INODE_INIT_BN; i++)
    bitmap_write(bit, i);

  spb.free_d_block = (DEVSIZE / PAGESIZE) - DATA_INIT_BN;
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
void super_write() {
  pwrite(spb.fp, (char *)&spb, PAGESIZE, SUPER_INIT_BN);
}

void super_read() {
  pread(spb.fp, (char *)&spb, PAGESIZE, SUPER_INIT_BN);
}

void bitmap_read(d_bitmap *bitmap, uint32_t block_num) {
  pread(spb.fp, (char *)bitmap,  PAGESIZE, (block_num + D_BITMAP_INIT_BN) * PAGESIZE);
}

void bitmap_write(d_bitmap *bitmap, uint32_t block_num) {
  pwrite(spb.fp, (char *)bitmap, PAGESIZE, (block_num + D_BITMAP_INIT_BN) * PAGESIZE);
}

void bitmap_update(uint32_t data_block_num, uint8_t type) {
  uint32_t block_num = GET_BLOCKNUM(data_block_num , (PAGESIZE * 8));
  uint32_t bit_idx = GET_BITIDX(data_block_num, (PAGESIZE * 8));
  if(block_num != spb.cur_bit_bn) {
    bitmap_write(spb.cur_bit, spb.cur_bit_bn);
    bitmap_read(spb.cur_bit, block_num);
    spb.cur_bit_bn = block_num;
  }
  if(type == VALID) {
    BIT_SET(spb.cur_bit->bitset[bit_idx / 8], (1 << (bit_idx % 8)));
    spb.free_d_block--;
  }
  else {
    BIT_CLEAR(spb.cur_bit->bitset[bit_idx / 8], (1 << (bit_idx % 8)));
    spb.free_d_block++;
  }
}

void data_read(void *data, uint32_t block_num) {
  pread(spb.fp, (char *)data, PAGESIZE, (block_num + DATA_INIT_BN) * PAGESIZE);
}

void data_write(void *data, uint32_t block_num) {
  pwrite(spb.fp, (char *)data, PAGESIZE, (block_num + DATA_INIT_BN) * PAGESIZE);
}

void inode_read(inode *node, int32_t inode_block_num) {
  uint32_t block_num = GET_BLOCKNUM(inode_block_num, (PAGESIZE / sizeof(inode)));
  uint32_t bit_idx = GET_BITIDX(inode_block_num, (PAGESIZE / sizeof(inode)));
  i_block blk;
  pread(spb.fp, (char*)&blk, PAGESIZE, (INODE_INIT_BN + block_num) * PAGESIZE);
  *node = blk.i[bit_idx];
}

void inode_write(inode *node, uint32_t inode_block_num) {
  uint32_t block_num = GET_BLOCKNUM(inode_block_num, (PAGESIZE / sizeof(inode)));
  uint32_t bit_idx = GET_BITIDX(inode_block_num, (PAGESIZE / sizeof(inode)));
  i_block blk;
  pread(spb.fp, (char*)&blk, PAGESIZE, (INODE_INIT_BN + block_num) * PAGESIZE);
  blk.i[bit_idx] = *node;
  pwrite(spb.fp, (char*)&blk, PAGESIZE, (INODE_INIT_BN + block_num) * PAGESIZE);
}

int search_bitmap(int *arr, int num)
{
	d_bitmap *bitmap = (d_bitmap *)malloc(sizeof(d_bitmap));
	int a, b, c, bound = 0, i = 0;
	int **new_arr = (int **)malloc(sizeof(int *) * num);
	for (a = 0; a < num; a++)
		new_arr[a] = (int *)malloc(sizeof(int) * 3);
  bitmap_write(spb.cur_bit, spb.cur_bit_bn);
	for (c = 0; c < D_BITMAP_NUM; c++) {
		bitmap_read(bitmap, c);
		for (a = 0; a < PAGESIZE; a++) {
			for (b = 0; b < 8; b++) {
				if (bound >= spb.total_d_blocks){
					return -2;
				}
				if (BIT_CHECK(bitmap->bitset[a], b) == 0) {
					new_arr[i][0] = c;
					new_arr[i][1] = a;
					new_arr[i][2] = b;
					arr[i] = bound;
					if (++i >= num) {
            for (i = num - 1; i > 0; i--)
              bitmap_update(new_arr[i][0] * PAGESIZE + new_arr[i][1] * 8 + new_arr[i][2], VALID);
						return 0;
					}
				}
				bound++;
			}
		}
	}
	return -1;
}
