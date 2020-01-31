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
  struct superblock super;
	spb.fp = open("a", O_RDWR | O_CREAT | O_LARGEFILE, 0644);
	printf("hello %d \n", spb.fp);
  spb.root_directory = ROOT_DIR;
  spb.total_block_size = DEVSIZE;
  spb.d_bitmap_init_bn = D_BITMAP_INIT_BN;
  spb.inode_init_bn = INODE_INIT_BN;
  spb.list_first = 0;
  spb.free_inode = (DATA_INIT_BN - INODE_INIT_BN) * (PAGESIZE / sizeof(struct inode));
  spb.free_d_block = DEVSIZE - DATA_INIT_BN;
  spb.cur_bit = NULL;

  printf("%d\n", sizeof(struct superblock));
  write(spb.fp, (char *)&spb, PAGESIZE);
  pread(spb.fp, (char *)&super, PAGESIZE, 0);
  printf("hello %d \n", super.fp);
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

void bitmap_init() {
	struct d_bitmap *bit = (struct d_bitmap *)calloc(1, sizeof(struct d_bitmap));
	int i;
	lseek(spb.fp, D_BITMAP_INIT_BN * PAGESIZE, SEEK_SET);

	for (i = D_BITMAP_INIT_BN; i < INODE_INIT_BN; i++)
		write(spb.fp, (char *)bit, PAGESIZE);

	spb.cur_bit = bit;
}
/*
void free_list_init() {
	spb.list_first =
}
*/
