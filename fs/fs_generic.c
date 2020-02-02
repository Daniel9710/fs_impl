#define _LARGEFILE64_SOURCE
#define _XOPEN_SOURCE 500

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
	inode node;
	char ppath[56];
	int cwd = inode_trace(path, &node, ppath);
	(void) fi;
	memset(stbuf, 0, sizeof(struct stat));
	if(cwd == -1)
		cwd = spb.root_directory;
	else {
		cwd = search_dir(&node, ppath);
		if(cwd == -1)
			return -ENOENT;
	}
	inode_read(&node, cwd);
	stbuf->st_ino = cwd;
	stbuf->st_mode = node.attr.mode;
	stbuf->st_nlink = node.attr.nlink;
	stbuf->st_uid = node.attr.uid;
	stbuf->st_gid = node.attr.gid;
	stbuf->st_size = node.attr.size;
	stbuf->st_blksize = PAGESIZE;
	stbuf->st_atime = node.attr.atime;
	stbuf->st_mtime = node.attr.mtime;
	stbuf->st_ctime = node.attr.ctime;

    return 0;
}

int fs_utimens (const char *path, const struct timespec ts[2], struct fuse_file_info *fi) {

	return 0;
}

int fs_chmod (const char *path, mode_t mode, struct fuse_file_info *fi) {
	inode node;
	char ppath[56];
	int cwd = inode_trace(path, &node, ppath);
	(void) fi;
	if(cwd == -1)
		cwd = spb.root_directory;
	else {
		cwd = search_dir(&node, ppath);
		if(cwd == -1)
			return -ENOENT;
	}
	inode_read(&node, cwd);
	node.attr.mode = mode;
	node.attr.atime = node.attr.ctime = time(NULL);
	inode_write(&node, cwd);
    return 0;
}

int fs_chown (const char *path, uid_t uid, gid_t gid, struct fuse_file_info *fi) {
	inode node;
	char ppath[56];
	int cwd = inode_trace(path, &node, ppath);
	(void) fi;
	if(cwd == -1)
		cwd = spb.root_directory;
	else {
		cwd = search_dir(&node, ppath);
		if(cwd == -1)
			return -ENOENT;
	}
	inode_read(&node, cwd);
	node.attr.uid = uid;
	node.attr.gid = gid;
	node.attr.atime = node.attr.ctime = time(NULL);
	inode_write(&node, cwd);
    return 0;
}

int fs_rename (const char *oldpath, const char *newpath, unsigned int flags) {
	inode oldnode, newnode, dir_oldnode, dir_newnode;
	char oldppath[56], newppath[56];
	int oldcwd, newcwd, oldinum, newinum;

	if(flags)
		return -EINVAL;

	if((oldcwd = inode_trace(oldpath, &dir_oldnode, oldppath)) == -1)
		return -EACCES;
	oldinum = search_dir(&dir_oldnode, oldppath);

	if((newcwd = inode_trace(newpath, &dir_newnode, newppath)) == -1)
		return -EACCES;
	else if(newcwd == -2)
		return -ENOENT;
	else if((newinum = search_dir(&dir_newnode, newppath)) != -1) {
		inode_read(&oldnode, oldinum);
		inode_read(&newnode, newinum);

		if(newnode.attr.mode & S_IFDIR) {
			if(!(oldnode.attr.mode & S_IFDIR))
				return -EISDIR;
			if(is_dir_empty(&newnode) == -1)
				return -ENOTEMPTY;
		}
		else {
			if(oldnode.attr.mode & S_IFDIR)
				return -ENOTDIR;
		}
		remove_file(&newnode);
		free_inode(newinum);
	}
	else
		inode_read(&oldnode, oldinum);

	if(dir_oldnode.attr.ino == dir_newnode.attr.ino) {
			rename_dir(&dir_newnode, oldinum, newppath);
	}
	else {
		delete_dir(&dir_oldnode, oldinum);
		update_dir(&dir_newnode, oldinum, newppath, oldnode.attr.mode & 0770000);
		if(oldnode.attr.mode & S_IFDIR) {
			dir_oldnode.attr.nlink--;
			dir_newnode.attr.nlink++;
		}
		dir_oldnode.attr.mtime = dir_oldnode.attr.ctime = time(NULL);
		inode_write(&dir_oldnode, oldcwd);
	}
	dir_newnode.attr.mtime = dir_newnode.attr.ctime = time(NULL);
	inode_write(&dir_newnode, newcwd);

    return 0;
}

int fs_access (const char *path, int mask) {
	inode node;
	char ppath[56];
	int cwd = inode_trace(path, &node, ppath);
	if(cwd == -1)
		cwd = spb.root_directory;
	else {
		cwd = search_dir(&node, ppath);
		if(cwd == -1)
			return -ENOENT;
	}

	return 0;
}

int fs_symlink (const char *from, const char *to) {
	inode fromnode, tonode, dir_fromnode, dir_tonode;
	char fromppath[56], toppath[56];
	int fromcwd, tocwd, frominum, toinum;
	if(strlen(from) > PAGESIZE - sizeof(struct metadata))
		return -ENAMETOOLONG;
	if((fromcwd = inode_trace(from, &dir_fromnode, fromppath)) == -1)
		frominum = spb.root_directory;
	else
		frominum = search_dir(&dir_fromnode, fromppath);
	inode_read(&fromnode, frominum);

	tocwd = inode_trace(to, &dir_tonode, toppath);
	toinum = new_inode();
	memset(&tonode, -1, sizeof(inode));
	metadata_init(&tonode.attr, (fromnode.attr.mode & ~(0770000))|S_IFLNK, strlen(from), toinum);
	memcpy(&(tonode.direct_ptr), from, strlen(from));

	update_dir(&dir_tonode, toinum, toppath, S_IFLNK);
	dir_tonode.attr.nlink++;
	dir_tonode.attr.mtime = dir_tonode.attr.ctime = time(NULL);
	inode_write(&dir_tonode, tocwd);
	inode_write(&tonode, toinum);

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
	spb.fp = open("a", O_RDWR | O_CREAT | O_TRUNC | O_LARGEFILE, 0644);
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
  	spb.cur_bit_bn = 76;
	bitmap_read(spb.cur_bit, spb.cur_bit_bn);
}

void free_list_init() {
  	free_list ll;
  	int i, node_num = ENTRYPERPAGE - 2, i_num = spb.free_inode - 1, d_blk = spb.total_d_blocks - 1;
  	ll.next = -1;
  	while(i_num > 1023) {
    		for(i = node_num; i >= 0; --i)
      			ll.free_node[i] = i_num--;
   	 	bitmap_update(d_blk, VALID);
   	 	data_write((void *)&ll, d_blk);
   	 	ll.next = d_blk--;
  	}
  	for(i = node_num; i >= 0 && i_num > 0; --i)
    		ll.free_node[i] = i_num--;
	spb.list_now = i + 1;
	for(;i >= 0;--i)
		ll.free_node[i] = -1;
  	bitmap_update(d_blk, VALID);
  	data_write((void *)&ll, d_blk);
  	spb.list_first = d_blk;
}
void super_write() {
  	pwrite(spb.fp, &spb, PAGESIZE, SUPER_INIT_BN);
}

void super_read() {
  	pread(spb.fp, &spb, PAGESIZE, SUPER_INIT_BN);
}

void bitmap_read(d_bitmap *bitmap, uint32_t block_num) {
  	pread(spb.fp, bitmap, PAGESIZE, (block_num + D_BITMAP_INIT_BN) * PAGESIZE);
}

void bitmap_write(d_bitmap *bitmap, uint32_t block_num) {
  	pwrite(spb.fp, bitmap, PAGESIZE, (block_num + D_BITMAP_INIT_BN) * PAGESIZE);
}

void bitmap_update(uint32_t data_block_num, uint32_t type) {
  	uint32_t block_num = GET_BLOCKNUM(data_block_num , (PAGESIZE * 8));
  	uint32_t bit_idx = GET_BITIDX(data_block_num, (PAGESIZE * 8));
	if(block_num != spb.cur_bit_bn) {
    		bitmap_write(spb.cur_bit, spb.cur_bit_bn);
 	   	bitmap_read(spb.cur_bit, block_num);
 	   	spb.cur_bit_bn = block_num;
  	}
	if(type == VALID) {
 	   	BIT_SET(spb.cur_bit->bitset[bit_idx / 8], (bit_idx % 8));
 	   	spb.free_d_block--;
  	}
  	else {
  	  	BIT_CLEAR(spb.cur_bit->bitset[bit_idx / 8], (bit_idx % 8));
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
  	uint32_t block_num = GET_BLOCKNUM(inode_block_num, INODEPERPAGE);
  	uint32_t bit_idx = GET_BITIDX(inode_block_num, INODEPERPAGE);
  	i_block blk;
  	pread(spb.fp, (char*)&blk, PAGESIZE, (INODE_INIT_BN + block_num) * PAGESIZE);
  	*node = blk.i[bit_idx];
}

void inode_write(inode *node, uint32_t inode_block_num) {
  	uint32_t block_num = GET_BLOCKNUM(inode_block_num, INODEPERPAGE);
  	uint32_t bit_idx = GET_BITIDX(inode_block_num, INODEPERPAGE);
  	i_block blk;
  	pread(spb.fp, &blk, PAGESIZE, (INODE_INIT_BN + block_num) * PAGESIZE);
  	blk.i[bit_idx] = *node;
  	pwrite(spb.fp, &blk, PAGESIZE, (INODE_INIT_BN + block_num) * PAGESIZE);
}

int search_bitmap(int *arr, int num)
{
    d_bitmap bitmap;
    int a, b, c, bound = 0, i = 0;
	int **new_arr = (int **)malloc(sizeof(int *) * num);
	for (a = 0; a < num; a++)
		new_arr[a] = (int *)malloc(sizeof(int) * 3);
    	bitmap_write(spb.cur_bit, spb.cur_bit_bn);
	for (c = 0; c < D_BITMAP_NUM; c++) {
		bitmap_read(&bitmap, c);
		for (a = 0; a < PAGESIZE; a++) {
			for (b = 0; b < 8; b++) {
				if (bound >= spb.total_d_blocks){
          				printf("ERROR : NO SUFFICIENT BLOCK\n");
					return -1;
				}
				if (BIT_CHECK(bitmap.bitset[a], b) == 0) {
					new_arr[i][0] = c;
					new_arr[i][1] = a;
					new_arr[i][2] = b;
					arr[i] = bound;
					if (++i >= num) {
            					for (i = num; i > 0; i--)
                					bitmap_update(bound, VALID);
						return 0;
					}
				}
				bound++;
			}
		}
	}
	printf("ERROR : NO SUFFICIENT BLOCK\n");
  	return -1;
}

int new_inode(){
  	int blk_num = spb.list_first;
  	free_list ll;
 	if(spb.free_inode == 0)
	    	return -1;
  	data_read((void *)&ll, blk_num);
  	if(spb.list_now == ENTRYPERPAGE - 1) {
    		bitmap_update(blk_num, INVALID);
   	 	spb.list_first = ll.next;
   	 	spb.list_now = 0;
   	 	data_read((void *)&ll, blk_num);
  	}
	spb.free_inode--;
  	return ll.free_node[spb.list_now++];
}

void free_inode(int block_num){
  	int arr[1];
  	free_list *ll = (free_list *)calloc(1, sizeof(free_list));
  	if(spb.list_now == 0) {
    		search_bitmap(arr, 1);
	    	ll->next = spb.list_first;
		spb.list_first = arr[0];
	    	spb.list_now = ENTRYPERPAGE - 2;
    		ll->free_node[spb.list_now] = block_num;
  	}
  	else {
  	  	data_read((void *)ll, spb.list_first);
    		ll->free_node[--spb.list_now] = block_num;
  	}
	spb.free_inode++;
  	data_write((void *)ll, spb.list_first);
}
int inode_trace(const char *path, inode *node, char *file) {
	char ppath[100], *pptr = NULL, *ptr;
	int32_t cwd = -1,i;
	inode dir_node;

	strcpy(ppath, path);
	if((ptr = strtok(ppath,"/")) != NULL) {
		cwd = spb.root_directory;
		while (1){
			inode_read(&dir_node, cwd);
			pptr = ptr;
	  		if((ptr = strtok(NULL, "/")) == NULL)
				break;
			if((cwd = search_dir(&dir_node, pptr)) == -1)
				return -2;
		}

		for(i = 0; *(pptr + i) != '\0'; i++)
			*(file + i) = *(pptr + i);
		*(file + i) = '\0';
		*node = dir_node;
	}
	return cwd;
}

void metadata_init(struct metadata *meta, mode_t mode, size_t size, uint64_t ino) {
	time_t t = time(NULL);
	struct fuse_context *fs_cxt = fuse_get_context();

	meta->mode = mode;
	meta->nlink = 2;
	meta->uid = fs_cxt->uid;
	meta->gid = fs_cxt->gid;
	meta->size = size;
	meta->atime = t;
	meta->ctime = t;
	meta->mtime = t;
	meta->ino = ino;
}
void cur_bit_test() {
	bitmap_write(spb.cur_bit, spb.cur_bit_bn);
	bitmap_read(spb.cur_bit, spb.cur_bit_bn);
	printf("-------------------\n");
	for(int i = 0; i < 64; i++){
			for(int j = 0; j < 16; j++){
				printf("%x ",*(uint32_t *)&spb.cur_bit->bitset[(i * 16 + j) * 4]);
			}
		 printf("\n");
	}
}
