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
	char ppath[60];
	inode dir_node;

	if(inode_trace(path, &dir_node, ppath) == -1)
		fi->fh = spb.root_directory;
	else
		fi->fh = search_dir(&dir_node, ppath);
	return 0;
}

int fs_mkdir (const char *path, mode_t mode) {
	char ppath[56];
	int32_t cwd, inum, entry_block[0];
	inode node, dir_node;
	dir_block dir_entry;

	cwd = inode_trace(path, &dir_node, ppath);
	inum = new_inode();
	memset(&node, -1, sizeof(inode));

	metadata_init(&node.attr, mode|S_IFDIR, 4096, inum);
	if(search_bitmap(entry_block, 1) < 0){
		free_inode(inum); return -1;
	}
	node.direct_ptr[0] = entry_block[0];

	if(cwd == -1)
		cwd = spb.root_directory = inum;

	else {
		update_dir(&dir_node, inum, ppath,S_IFDIR);
		dir_node.attr.nlink++;
		dir_node.attr.mtime = dir_node.attr.ctime = time(NULL);
		inode_write(&dir_node, cwd);
	}
	memset(&dir_entry, -1, sizeof(dir_block));
	dir_entry.entry[0].type = S_IFDIR;
	dir_entry.entry[0].inode_num = inum;
	strcpy(dir_entry.entry[0].name, ".");
	dir_entry.entry[1].type = S_IFDIR;
	dir_entry.entry[1].inode_num = cwd;
	strcpy(dir_entry.entry[1].name, "..");
	data_write((void *)&dir_entry, node.direct_ptr[0]);
	inode_write(&node, inum);
	return 0;
}

int fs_readdir (const char *path, void *buf, fuse_fill_dir_t filler, off_t off, struct fuse_file_info *fi, enum fuse_readdir_flags flags) {

	(void) off;
	(void) flags;
	inode node;
	int inum = fi->fh, i, j;
	dir_block dir;
	struct stat st;
	memset(&st, 0, sizeof(struct stat));
	inode_read(&node, inum);
	for(i = 0; i < DIRECT_PTR; i++) {
		if(node.direct_ptr[i] != -1) {
			data_read((void *)&dir, node.direct_ptr[i]);
			for(j = 0; j < DIRPERPAGE; j++) {
				if(dir.entry[j].inode_num != -1) {
					st.st_ino = dir.entry[j].inode_num;
					st.st_mode = dir.entry[j].type;
					filler(buf, dir.entry[j].name, &st, 0, (enum fuse_fill_dir_flags)0);
				}
			}
		}
		else
			break;
	}
	return 0;
}

int fs_rmdir (const char *path) {
	char ppath[60];
	int32_t cwd, inum, i;
	inode node, dir_node;

	if((cwd = inode_trace(path, &dir_node, ppath)) == -1)
		return -EFAULT;

	inum = search_dir(&dir_node, ppath);
	inode_read(&node, inum);
	if(is_dir_empty(&node) == -1)
		return -ENOTEMPTY;

	delete_dir(&dir_node, inum);
	dir_node.attr.nlink--;
	dir_node.attr.mtime = dir_node.attr.ctime = time(NULL);
	inode_write(&dir_node, cwd);

	for(i = 0; i < DIRECT_PTR && node.direct_ptr[i] != -1; i++)
		bitmap_update(node.direct_ptr[i], INVALID);
	free_inode(inum);
	return 0;
}

int fs_releasedir (const char *path, struct fuse_file_info *fi) {
	(void) fi;
	return 0;
}

int fs_fsyncdir (const char *path, int datasync, struct fuse_file_info *fi) {
	return 0;
}

int search_dir(inode *node, const char *ptr) {
	int i, j;
    	dir_block dir;
    	for(i = 0; i < DIRECT_PTR; i++){
        	if(node->direct_ptr[i] != -1) {
          	  	data_read((void *)&dir, node->direct_ptr[i]);
		        for(j = 0; j < DIRPERPAGE; j++) {
					if(dir.entry[j].inode_num != -1) {
        	      	 	 	if(strcmp(dir.entry[j].name, ptr) == 0)
                    				return dir.entry[j].inode_num;
					}
          		}
        	}
			else
				return -1;
    	}
    	return -1;
}

int update_dir(inode *node, int inum, const char *ptr, int type) {
    int i, j, arr[3];
    dir_block dir;
    for(i = 0; i < DIRECT_PTR; i++){
        if(node->direct_ptr[i] != -1) {
            data_read((void *)&dir, node->direct_ptr[i]);
            for(j = 0; j < DIRPERPAGE; j++) {
                if(dir.entry[j].inode_num == -1) {
			update_direntry(&dir, inum, ptr, j, node->direct_ptr[i], type);
                    	return 0;
                }
            }
        }
        else {
            if(search_bitmap(arr, 1) < 0)
                return -1;
            node->direct_ptr[i] = arr[0];
            memset((void *)&dir, -1, sizeof(dir_block));
            update_direntry(&dir, inum, ptr, 0, arr[0], type);
            return 0;
        }
    }
    return -1;
}
int rename_dir(inode *node, int inum, char *ptr) {
	int i, j;
    dir_block dir;
    for(i = 0; i < DIRECT_PTR; i++){
        if(node->direct_ptr[i] != -1) {
            data_read((void *)&dir, node->direct_ptr[i]);
            for(j = 0; j < DIRPERPAGE; j++) {
                if(dir.entry[j].inode_num == inum) {
					strcpy(dir.entry[j].name, ptr);
					data_write((void *)&dir, node->direct_ptr[i]);
                    return 0;
                }
            }
        }
        else
			return -1;
    }
    return -1;
}
void update_direntry(dir_block *dir, int inum, const char *ptr, int idx, int blk, int type) {
    strcpy(dir->entry[idx].name, ptr);
    dir->entry[idx].inode_num = inum;
    dir->entry[idx].type = type;
    data_write((void *)dir, blk);
}

int delete_dir(inode *node, int inum){
	int i, j;
	dir_block dir;
	for(i = 0; i < DIRECT_PTR; i++){
        if(node->direct_ptr[i] != -1) {
            data_read((void *)&dir, node->direct_ptr[i]);
            for(j = 0; j < DIRPERPAGE; j++) {
                if(dir.entry[j].inode_num == inum) {
			update_direntry(&dir, -1, "--", j, node->direct_ptr[i], 0);
			return 0;
                }
            }
        }
		else
			return -1;
    }
	return -1;
}
int is_dir_empty(inode *node) {
	int i, j =2;
	dir_block dir;
	for(i = 0; i < DIRECT_PTR; i++){
		if(node->direct_ptr[i] != -1) {
			data_read((void *)&dir, node->direct_ptr[i]);
			for(; j < DIRPERPAGE; j++) {
				if(dir.entry[j].inode_num > -1)
					return -1;
			}
			j = 0;
		}
		else
			return 0;
	}
	return 0;
}
