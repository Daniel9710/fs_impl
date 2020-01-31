#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <assert.h>
#include "fs_dir.h"
#include "metadata.h"
#include <stdlib.h>
#include "fs_generic.h"

extern struct superblock spb;

int fs_opendir (const char *path, struct fuse_file_info *fi) {
	fi->keep_cache = 1;

	return 0;
}

int fs_mkdir (const char *path, mode_t mode) {
	char ppath[60];
	strcpy(ppath, path);
	uint32_t parent, child, tmp;
	parent = child = tmp = spb.root_directory;
	char *ptr = strtok(ppath, "/");


	while (ptr != NULL){
		printf("%s\n", ptr);
	  ptr = strtok(NULL, "/");
	}
	return 0;
}

int fs_readdir (const char *path, void *buf, fuse_fill_dir_t filler, off_t off, struct fuse_file_info *fi, enum fuse_readdir_flags flags) {

	filler(buf, ".", NULL, 0, (enum fuse_fill_dir_flags)0);
	filler(buf, "..", NULL, 0, (enum fuse_fill_dir_flags)0);

	return 0;
}

int fs_rmdir (const char *path) {

	return 0;
}

int fs_releasedir (const char *path, struct fuse_file_info *fi) {
	return 0;
}

int fs_fsyncdir (const char *path, int datasync, struct fuse_file_info *fi) {
	return 0;
}
