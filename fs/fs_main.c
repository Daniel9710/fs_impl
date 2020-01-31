#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fuse3/fuse.h>
#include <fcntl.h>
#include "fs_main.h"
#include "metadata.h"

struct superblock spb;

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
  uint32_t block_num = data_block_num / (PAGESIZE * 8);
  uint32_t bit_idx = data_block_num % (PAGESIZE * 8);
  if(block_num != spb.cur_bit_bn) {
    bitmap_write(spb.cur_bit, spb.cur_bit_bn);
    bitmap_read(spb.cur_bit, block_num);
    spb.cur_bit_bn = block_num;
  }
  if(type == VALID)
    spb.cur_bit->bitset[bit_idx / 8] |= (1 << (bit_idx % 8));
  else
    spb.cur_bit->bitset[bit_idx / 8] &= ~(1 << (bit_idx % 8));
}

void data_read(void *data, uint32_t block_num) {
  pread(spb.fp, (char *)data, PAGESIZE, (block_num + DATA_INIT_BN) * PAGESIZE);
}
void data_write(void *data, uint32_t block_num) {
  pwrite(spb.fp, (char *)data, PAGESIZE, (block_num + DATA_INIT_BN) * PAGESIZE);
}
void inode_read(inode *node, int32_t inode_block_num) {
  uint32_t block_num = data_block_num / (PAGESIZE / sizeof(inode));
  uint32_t bit_idx = data_block_num % (PAGESIZE / sizeof(inode));
  i_block blk;
  pread(spb.fp, (char*)&blk, PAGESIZE, (INODE_INIT_BN + block_num) * PAGESIZE);
  *node = blk[bit_idx];
}
void inode_write(inode *node, uint32_t inode_block_num) {
  uint32_t block_num = data_block_num / (PAGESIZE / sizeof(inode));
  uint32_t bit_idx = data_block_num % (PAGESIZE / sizeof(inode));
  i_block blk;
  pread(spb.fp, (char*)&blk, PAGESIZE, (INODE_INIT_BN + block_num) * PAGESIZE);
  blk[bit_idx] = *blk;
  pwrite(spb.fp, (char*)&blk, PAGESIZE, (INODE_INIT_BN + block_num) * PAGESIZE);
}
