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
	char ppath[60], *pptr, *ptr;
	int32_t cwd, inum, entry_block[0];
	inode node, dir_node;
	dir_block dir_entry;

	cwd = -1;
	strcpy(ppath, path);
	printf("%s\n",ppath);

	ptr = strtok(ppath, "/");
	if(ptr != NULL) {
		cwd = spb.root_directory;
		while (1){
			printf("%s\n", ptr);
			inode_read(&dir_node, cwd);
			pptr = ptr;
	  		ptr = strtok(NULL, "/");
			if(ptr== NULL)
				break;
			cwd = search_dir(&dir_node, pptr);
		}
	}
	inum = new_inode();
	memset((void *)&node, -1, sizeof(inode));

	metadata_init(&node.attr, mode|S_IFDIR, 4096, inum);
	if(search_bitmap(entry_block, 1) < 0){
		free_inode(inum); return -1;
	}
	node.direct_ptr[0] = entry_block[0];

	if(cwd == -1)
		spb.root_directory = inum;

	else
		update_dir(&dir_node, cwd, pptr);
	memset((void *)&dir_entry, -1, sizeof(dir_block));
	strcpy(dir_entry.entry[0].name, ".");
	dir_entry.entry[0].inode_num = inum;
	strcpy(dir_entry.entry[1].name, "..");
	dir_entry.entry[1].inode_num = cwd;
	data_write((void *)&dir_entry, node.direct_ptr[0]);
	inode_write(&node, inum);
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
