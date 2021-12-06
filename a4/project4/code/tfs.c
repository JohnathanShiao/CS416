/*
 *  Copyright (C) 2019 CS416 Spring 2019
 *	
 *	Tiny File System
 *
 *	File:	tfs.c
 *  Author: Yujie REN
 *	Date:	April 2019
 *
 */

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/time.h>
#include <libgen.h>
#include <limits.h>

#include "block.h"
#include "tfs.h"

#define INODE_SIZE sizeof(struct inode)
#define NUM_INODES BLOCK_SIZE/INODE_SIZE
#define NUM_IBLKS MAX_INUM/NUM_INODES

#define DIRENT_SIZE sizeof(struct dirent)
#define NUM_DIRENTS BLOCK_SIZE/DIRENT_SIZE
#define ROOT "/"
#define CUR_DIR "."
#define PAR_DIR ".."

char diskfile_path[PATH_MAX];

// Declare your in-memory data structures here

struct superblock* superblock;

static void getNames(const char* path, char* dirName, char* baseName)
{
	char* temp = calloc(1,strlen(path));
	memcpy(temp,path,strlen(path));
	memcpy(dirName,dirname(temp),strlen(dirname(temp)));
	memcpy(basename,__xpg_basename(temp),strlen(__xpg_basename(temp)));
}
/* 
 * Get available inode number from bitmap
 */
int get_avail_ino() {

	// Step 1: Read inode bitmap from disk
	char i_bitmap_string[BLOCK_SIZE];
	bitmap_t i_bitmap = (bitmap_t)i_bitmap_string;
	bio_read(superblock->i_bitmap_blk,i_bitmap);
	// Step 2: Traverse inode bitmap to find an available slot
	int i_num = 2;
	while(get_bitmap(i_bitmap,i_num))
		i_num++;
	// Step 3: Update inode bitmap and write to disk 
	set_bitmap(i_bitmap,i_num);
	bio_write(superblock->i_bitmap_blk,i_bitmap);
	return i_num;
}

/* 
 * Get available data block number from bitmap
 */
int get_avail_blkno() {

	// Step 1: Read data block bitmap from disk
	char d_bitmap_string[BLOCK_SIZE];
	bitmap_t d_bitmap = (bitmap_t)d_bitmap_string;
	bio_read(superblock->d_bitmap_blk,d_bitmap);
	// Step 2: Traverse data block bitmap to find an available slot
	int d_num = 1;
	while(get_bitmap(d_bitmap,d_num))
		d_num++;
	// Step 3: Update data block bitmap and write to disk 
	set_bitmap(d_bitmap,d_num);
	bio_write(superblock->d_bitmap_blk,d_bitmap);
	return d_num;
}

/* 
 * inode operations
 */
int readi(uint16_t ino, struct inode *inode) {

	// Step 1: Get the inode's on-disk block number
	struct inode* temp_blk = calloc(1,BLOCK_SIZE);
	uint32_t blk_num = (ino*INODE_SIZE)/BLOCK_SIZE;
	// Step 2: Get offset of the inode in the inode on-disk block
	uint32_t offset = (blk_num*BLOCK_SIZE) + superblock->i_start_blk;
	int int_offset = ino - (NUM_INODES*blk_num);
	// Step 3: Read the block from disk and then copy into inode structure
	bio_read(offset,temp_blk);
	if(!temp_blk[int_offset].valid)
		return -1;
	memcpy(inode,&temp_blk[int_offset],INODE_SIZE);
	return 0;
}

int writei(uint16_t ino, struct inode *inode) {

	// Step 1: Get the block number where this inode resides on disk
	struct inode* temp_blk = calloc(1,BLOCK_SIZE);
	uint32_t blk_num = (ino*INODE_SIZE)/BLOCK_SIZE;
	// Step 2: Get the offset in the block where this inode resides on disk
	uint32_t offset = (blk_num*BLOCK_SIZE) + superblock->i_start_blk;
	int int_offset = ino-(NUM_INODES*blk_num);
	// Step 3: Write inode to disk 
	bio_read(offset,temp_blk);
	memcpy(&temp_blk[int_offset],inode,INODE_SIZE);
	bio_write(offset,temp_blk);
	return 0;
}


/* 
 * directory operations
 */
int dir_find(uint16_t ino, const char *fname, size_t name_len, struct dirent *dirent) {
	struct dirent* curr_dirent = calloc(1,NUM_DIRENTS);
	struct inode* curr_inode;
	// Step 1: Call readi() to get the inode using ino (inode number of current directory)
	if(readi(ino,&curr_inode)<0)
		return -1;

	if(!curr_inode->valid)
		return -1;

	// Step 2: Get data block of current directory from inode
	for(int i = 0;i<16;i++)
	{
		if(curr_inode->direct_ptr[i] != -1)
		{
			// Step 3: Read directory's data block and check each directory entry.
			//If the name matches, then copy directory entry to dirent structure
			bio_read(curr_inode->direct_ptr[i],curr_dirent);
			for(int j = 0;j<NUM_DIRENTS;j++)
			{
				if(strcmp(curr_dirent[j].name,fname)==0)
				{
					memcpy(dirent,&curr_dirent[j],DIRENT_SIZE);
					return 0;
				}
			}
		}
	}
	return -1;
}

int dir_add(struct inode dir_inode, uint16_t f_ino, const char *fname, size_t name_len) {

	// Step 1: Read dir_inode's data block and check each directory entry of dir_inode
	struct dirent entry;
	// Step 2: Check if fname (directory name) is already used in other entries
	if(dir_find(dir_inode.ino,fname,name_len,&entry)==0)
		return -1;
	
	// Step 3: Add directory entry in dir_inode's data block and write to disk
	for(int i = 0;i<16;i++)
	{
		if(dir_inode.direct_ptr[i] == -1)
		{
			dir_inode.direct_ptr[i] = get_avail_blkno()*BLOCK_SIZE + superblock->d_start_blk;
			dir_inode.size += DIRENT_SIZE;
			struct dirent* temp = calloc(1,BLOCK_SIZE);
			bio_read(dir_inode.direct_ptr[i],temp);
			temp[0].ino = f_ino;
			temp[0].valid = 1;
			memcpy(temp[0].name,fname,name_len);
			bio_write(dir_inode.direct_ptr[i],temp);
			writei(dir_inode.ino,&dir_inode);
			return 0;
		}

		struct dirent* entries = calloc(1,BLOCK_SIZE);
		bio_read(dir_inode.direct_ptr[i],entries);
		for(int j = 0;j<NUM_DIRENTS;j++)
		{
			if(entries[j].valid == 0)
			{
				dir_inode.size+=DIRENT_SIZE;
				entries[j].ino=f_ino;
				entries[j].valid=1;
				memcpy(entries[j].name,fname,name_len);
				bio_write(dir_inode.direct_ptr[i],entries);
				return 0;
			}
		}
	}
	// Allocate a new data block for this directory if it does not exist

	// Update directory inode

	// Write directory entry

	return -1;
}

int dir_remove(struct inode dir_inode, const char *fname, size_t name_len) {

	// Step 1: Read dir_inode's data block and checks each directory entry of dir_inode
	struct dirent entry;
	// Step 2: Check if fname exist
	if(dir_find(dir_inode.ino,fname,name_len,&entry)<0)
		return -1;
	
	// Step 3: If exist, then remove it from dir_inode's data block and write to disk
	struct dirent* entries = calloc(1,BLOCK_SIZE);
	bio_read(dir_inode.direct_ptr[entry.ino],entries);
	for(int i = 0;i<NUM_DIRENTS;i++)
	{
		if(entries[i].valid == 1)
		{
			if(strcmp(entries[i].name,fname)==0)
			{
				memset(&entries[i],0,DIRENT_SIZE);
				dir_inode.size -=DIRENT_SIZE;
				for(int j = 0;j<NUM_DIRENTS;j++)
				{
					if(entries[j].valid==1)
					{
						bio_write(dir_inode.direct_ptr[entry.ino],entries);
						return 0;
					}
				}
				char d_bitmap[BLOCK_SIZE];
				int blk_num = (dir_inode.direct_ptr[entry.ino] - superblock->d_start_blk)/BLOCK_SIZE;
				bio_read(superblock->d_bitmap_blk,(bitmap_t)d_bitmap);
				unset_bitmap((bitmap_t)d_bitmap,blk_num);
				bio_write(superblock->d_bitmap_blk,(bitmap_t)d_bitmap);

				dir_inode.direct_ptr[entry.ino] = -1;
				writei(dir_inode.ino,&dir_inode);
				return 0;
			}
		}
	}
	return -1;
}

/* 
 * namei operation
 */
int get_node_by_path(const char *path, uint16_t ino, struct inode *inode) {
	
	// Step 1: Resolve the path name, walk through path, and finally, find its inode.
	// Note: You could either implement it in a iterative way or recursive way
	struct dirent curr_dir;
	int path_size = strlen(path)+1;
	char* temp_path = calloc(1,path_size);
	memcpy(temp_path,path,path_size);
	if(strcmp(path,"/")==0)
	{
		if(readi(ino,inode)<0)
			return -1;
		return 0;
	}

	char* fname = strtok(temp_path,"/");
	if(dir_find(ino,fname,strlen(fname),&curr_dir)<0)
		return -1;
	else
	{
		if(readi(curr_dir.ino,inode)<0)
			return -1;
		return 0;
	}

	while(fname!=NULL)
	{
		fname = strtok(NULL,"/");
		if(fname==NULL)
			return 0;
		if(dir_find(inode->ino,fname,strlen(fname),&curr_dir)<0)
			return -1;
		else
		{
			if(readi(curr_dir.ino,inode)<0)
				return -1;
		}
	}
	return -1;
}

/* 
 * Make file system
 */
int tfs_mkfs() {

	// Call dev_init() to initialize (Create) Diskfile
	dev_init(diskfile_path);
	// write superblock information
	superblock = calloc(1,sizeof(superblock));
	superblock->magic_num = MAGIC_NUM;
	superblock->max_inum = MAX_INUM;
	superblock->max_dnum = MAX_DNUM;
	superblock->i_bitmap_blk = BLOCK_SIZE;
	superblock->d_bitmap_blk = superblock->i_bitmap_blk + BLOCK_SIZE;
	superblock->i_start_blk = superblock->d_bitmap_blk + BLOCK_SIZE;
	superblock->d_start_blk= superblock->i_start_blk + (MAX_INUM*INODE_SIZE);
	bio_write(0,superblock);
	// initialize inode bitmap
	char i_bitmap_string[BLOCK_SIZE];
	bitmap_t i_bitmap = (bitmap_t)i_bitmap_string;
	memset(i_bitmap_string,0,BLOCK_SIZE);
	set_bitmap(i_bitmap,2);
	// initialize data block bitmap
	char d_bitmap_string[BLOCK_SIZE];
	bitmap_t d_bitmap = (bitmap_t)d_bitmap_string;
	memset(d_bitmap_string,0,BLOCK_SIZE);

	struct inode* root = calloc(1,INODE_SIZE);
	root->ino = 2;
	root->valid = 1;
	root->type = __S_IFDIR;
	root->size = 0;
	memset(root->direct_ptr,-1,sizeof(root->direct_ptr));
	// update bitmap information for root directory
	writei(2,root);
	// update inode for root directory
	bio_write(superblock->i_bitmap_blk,i_bitmap);
	return 0;
}


/* 
 * FUSE file operations
 */
static void *tfs_init(struct fuse_conn_info *conn) {

	// Step 1a: If disk file is not found, call mkfs
	if(dev_open(diskfile_path)<0)
		tfs_mkfs();
	else
	{
		// Step 1b: If disk file is found, just initialize in-memory data structures
		// and read superblock from disk
		superblock=calloc(1,sizeof(superblock));
		bio_read(0,superblock);
	}
	return NULL;
}

static void tfs_destroy(void *userdata) {

	// Step 1: De-allocate in-memory data structures
	free(superblock);
	superblock=NULL;
	// Step 2: Close diskfile
	dev_close();
}

static int tfs_getattr(const char *path, struct stat *stbuf) {

	// Step 1: call get_node_by_path() to get inode from path
	struct inode temp;
	memset(stbuf,0,sizeof(struct stat));

	if(get_node_by_path(path,2,&temp)<0)
		return -ENOENT;
	// Step 2: fill attribute of file into stbuf from inode
	stbuf->st_gid=getgid();
	stbuf->st_uid=getuid();
	if(temp.type == __S_IFDIR)
	{
		stbuf->st_mode = __S_IFDIR | 0755;
		stbuf->st_nlink=2;
	}
	else
	{
		stbuf->st_mode = __S_IFREG | 0644;
		stbuf->st_nlink = 1;
	}
	time(&stbuf->st_mtime);

	return 0;
}

static int tfs_opendir(const char *path, struct fuse_file_info *fi) {

	// Step 1: Call get_node_by_path() to get inode from path
	struct inode temp;
	if(get_node_by_path(path,2,&temp)<0)
		return -ENOENT;
	// Step 2: If not find, return -1
    return 0;
}

static int tfs_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {

	// Step 1: Call get_node_by_path() to get inode from path
	struct inode temp;
	struct dirent entries[NUM_DIRENTS];
	if(get_node_by_path(path,2,&temp)<0)
		return -ENOENT;
	// Step 2: Read directory entries from its data blocks, and copy them to filler
	filler(buffer,CUR_DIR,NULL,0);
	filler(buffer,PAR_DIR,NULL,0);
	for(int i = 0;i<16;i++)
	{
		if(temp.direct_ptr[i] != -1)
		{
			bio_read(temp.direct_ptr[i],entries);
			for(int j = 0;j< NUM_DIRENTS;j++)
			{
				if(entries[j].valid)
				{
					if(filler(buffer,entries[j].name,NULL,0)!=0)
						return -ENOMEM;
				}
			}
		}
	}
	return 0;
}


static int tfs_mkdir(const char *path, mode_t mode) {

	// Step 1: Use dirname() and basename() to separate parent directory path and target directory name
	char* dirName = calloc(1,strlen(path));
	char* baseName = calloc(1,strlen(path));
	getNames(path,dirName,baseName);
	// Step 2: Call get_node_by_path() to get inode of parent directory
	struct inode parent_dir;
	if(get_node_by_path(dirName,2,&parent_dir)!=0)
		return -1;
	// Step 3: Call get_avail_ino() to get an available inode number
	uint16_t ino = get_avail_ino();
	// Step 4: Call dir_add() to add directory entry of target directory to parent directory
	if(dir_add(&parent_dir,ino,baseName,strlen(baseName))!=0)
		return -1;
	// Step 5: Update inode for target directory
	struct inode* temp = calloc(1,INODE_SIZE);
	temp->ino = ino;
	temp->valid=1;
	temp->type = __S_IFDIR;
	temp->size=0;
	memset(temp->direct_ptr,-1,sizeof(temp->direct_ptr));
	// Step 6: Call writei() to write inode to disk
	if(writei(ino,&temp)!=0)
		return -1;

	return 0;
}

static int tfs_rmdir(const char *path) {

	// Step 1: Use dirname() and basename() to separate parent directory path and target directory name
	char* dirName = calloc(1,strlen(path));
	char* baseName = calloc(1,strlen(path));
	getNames(path,dirName,baseName);
	// Step 2: Call get_node_by_path() to get inode of target directory
	struct inode parent_dir;
	if(get_node_by_path(dirName,2,&parent_dir)!=0)
		return -1;
	struct dirent temp;
	if(dir_find(parent_dir.ino,baseName,strlen(baseName),&temp)!=0)
		return -1;
	
	struct inode temp_inode;
	if(readi(temp.ino,&temp_inode)!=0)
		return -1;
	for(int i = 0; i<sizeof(temp_inode.direct_ptr)/sizeof(int);i++)
	{
		if(temp_inode.direct_ptr[i] != -1)
			return -1;
	}
	// Step 3: Clear data block bitmap of target directory
	// Step 4: Clear inode bitmap and its data block
	unsigned char bitmap[BLOCK_SIZE]; 
	bio_read(superblock->i_bitmap_blk, bitmap); 
	unset_bitmap(bitmap, temp.ino); 
	bio_write(superblock->i_bitmap_blk, bitmap); 
	// Step 5: Call get_node_by_path() to get inode of parent directory

	// Step 6: Call dir_remove() to remove directory entry of target directory in its parent directory
	dir_remove(parent_dir,baseName,strlen(baseName));
	return 0;
}

static int tfs_releasedir(const char *path, struct fuse_file_info *fi) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
    return 0;
}

static int tfs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {

	// Step 1: Use dirname() and basename() to separate parent directory path and target file name
	char* dirName = calloc(1,strlen(path));
	char* baseName = calloc(1,strlen(path));
	getNames(path,dirName,baseName);
	// Step 2: Call get_node_by_path() to get inode of parent directory
	struct inode parent_dir;
	if(get_node_by_path(dirName,2,&parent_dir)!=0)
		return -1;
	// Step 3: Call get_avail_ino() to get an available inode number
	uint16_t temp_ino = get_avail_ino();
	// Step 4: Call dir_add() to add directory entry of target file to parent directory
	if(dir_add(parent_dir,temp_ino,baseName,strlen(baseName))!=0)
		return -1;
	// Step 5: Update inode for target file
	struct inode* temp = calloc(1,INODE_SIZE);
	temp->ino = temp_ino;
	temp->valid=1;
	temp->type = __S_IFDIR;
	temp->size=0;
	memset(temp->direct_ptr,-1,sizeof(temp->direct_ptr));
	// Step 6: Call writei() to write inode to disk
	if(writei(temp_ino,&temp)!=0)
		return -1;
	return 0;
}

static int tfs_open(const char *path, struct fuse_file_info *fi) {

	// Step 1: Call get_node_by_path() to get inode from path
	struct inode temp_inode;
	if(get_node_by_path(path,2,&temp_inode)!=0)
		return -1;
	// Step 2: If not find, return -1
	return 0;
}

static int tfs_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {

	// Step 1: You could call get_node_by_path() to get inode from path
	struct inode temp_inode;
	if(get_node_by_path(path,2,&temp_inode)!=0)
		return -1;
	// Step 2: Based on size and offset, read its data blocks from disk
	char* read_buf = calloc(1,BLOCK_SIZE);
	char* cpy_buf = calloc(1,size);
	int start = offset/BLOCK_SIZE;
	offset -=(start*BLOCK_SIZE);
	int amount = 0;
	// Step 3: copy the correct amount of data from offset to buffer
	for(int i = start;i<sizeof(temp_inode.direct_ptr)/sizeof(int) && size !=0;i++)
	{
		if(temp_inode.direct_ptr[i] == -1)
			break;
		if(bio_read(temp_inode.direct_ptr[i],read_buf),0)
			return amount;
		if(BLOCK_SIZE-offset >=size)
		{
			memcpy(cpy_buf + amount,read_buf+offset,size);
			offset = 0;
			amount+=size;
			size-=size;
		}
		else
		{
			memcpy(cpy_buf+amount, read_buf+offset,BLOCK_SIZE-offset);
			amount+=(BLOCK_SIZE - offset);
			offset = 0;
			size -= (BLOCK_SIZE - offset);
		}
	}
	memcpy(buffer,cpy_buf,amount);
	free(cpy_buf);
	// Note: this function should return the amount of bytes you copied to buffer
	return amount;
}

static int tfs_write(const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {
	// Step 1: You could call get_node_by_path() to get inode from path
	struct inode temp_inode;
	if(get_node_by_path(path,2,&temp_inode)!=0)
		return -1;
	// Step 2: Based on size and offset, read its data blocks from disk
	char* read_buf = calloc(1,BLOCK_SIZE);
	int start = offset/BLOCK_SIZE;
	offset -= (start * BLOCK_SIZE);
	// Step 3: Write the correct amount of data from offset to disk
	int amount=0;
	for(int i = 0;i<sizeof(temp_inode.direct_ptr)/sizeof(int) && size != 0;i++)
	{
		if(temp_inode.direct_ptr[i] == -1)
		{
			if(size==0)
				break;
			temp_inode.direct_ptr[i] = get_avail_ino()*BLOCK_SIZE + superblock->d_start_blk;
		}
		if(bio_read(temp_inode.direct_ptr[i],read_buf)<0)
			return amount;
		if(BLOCK_SIZE - offset >= size)
		{
			memcpy(read_buf+offset, buffer + amount,size);
			if(bio_write(temp_inode.direct_ptr[i],read_buf)<0)
				return amount;
			amount +=size;
			size -= size;
			offset = 0;
		}
		else
		{
			memcpy(read_buf+offset, buffer+offset, BLOCK_SIZE - offset);
			if(bio_write(temp_inode.direct_ptr[i], read_buf)<0)
				return amount;
			amount += (BLOCK_SIZE-offset);
			size -= (BLOCK_SIZE - offset);
			offset = 0;
		}
	}
	// Step 4: Update the inode info and write it to disk
	temp_inode.size+=amount;
	if(writei(temp_inode.ino, &temp_inode)<0)
		return -1;
	// Note: this function should return the amount of bytes you write to disk
	return amount;
}

static int tfs_unlink(const char *path) {

	// Step 1: Use dirname() and basename() to separate parent directory path and target file name
	char* dirName = calloc(1,strlen(path));
	char* baseName = calloc(1,strlen(path));
	getNames(path,dirName,baseName);
	// Step 2: Call get_node_by_path() to get inode of target file
	struct inode parent_node;
	if(get_node_by_path(dirName,2,&parent_node)!=0)
		return -1;
	struct dirent temp_dir;
	if(dir_find(parent_node.ino,baseName,strlen(baseName),&temp_dir)!=0)
		return -1;	
	// Step 3: Clear data block bitmap of target file
	unsigned char d_bitmap[BLOCK_SIZE];
	if(bio_read(superblock->d_bitmap_blk,d_bitmap)<0)
		return -1;
	// Step 4: Clear inode bitmap and its data block
	unsigned char bitmap[BLOCK_SIZE]; 
	bio_read(superblock->i_bitmap_blk, bitmap); 
	unset_bitmap(bitmap, temp_dir.ino); 
	bio_write(superblock->i_bitmap_blk, bitmap);

	struct inode temp_inode;
	if(readi(temp_dir.ino,&temp_inode)<0)
		return -1;
	for(int i =0;i<sizeof(temp_inode.direct_ptr)/sizeof(int);i++)
	{
		if(temp_inode.direct_ptr[i] != -1)
			unset_bitmap(d_bitmap,(temp_inode.direct_ptr[i] - superblock->d_start_blk)/BLOCK_SIZE);
	}
	if(bio_write(superblock->d_bitmap_blk,d_bitmap)<0)
		return -1; 
	// Step 5: Call get_node_by_path() to get inode of parent directory
	// Step 6: Call dir_remove() to remove directory entry of target file in its parent directory
	if(dir_remove(parent_node,baseName,strlen(baseName))<0)
		return -1;
	return 0;
}

static int tfs_truncate(const char *path, off_t size) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
    return 0;
}

static int tfs_release(const char *path, struct fuse_file_info *fi) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
	return 0;
}

static int tfs_flush(const char * path, struct fuse_file_info * fi) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
    return 0;
}

static int tfs_utimens(const char *path, const struct timespec tv[2]) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
    return 0;
}


static struct fuse_operations tfs_ope = {
	.init		= tfs_init,
	.destroy	= tfs_destroy,

	.getattr	= tfs_getattr,
	.readdir	= tfs_readdir,
	.opendir	= tfs_opendir,
	.releasedir	= tfs_releasedir,
	.mkdir		= tfs_mkdir,
	.rmdir		= tfs_rmdir,

	.create		= tfs_create,
	.open		= tfs_open,
	.read 		= tfs_read,
	.write		= tfs_write,
	.unlink		= tfs_unlink,

	.truncate   = tfs_truncate,
	.flush      = tfs_flush,
	.utimens    = tfs_utimens,
	.release	= tfs_release
};


int main(int argc, char *argv[]) {
	int fuse_stat;

	getcwd(diskfile_path, PATH_MAX);
	strcat(diskfile_path, "/DISKFILE");

	fuse_stat = fuse_main(argc, argv, &tfs_ope, NULL);

	return fuse_stat;
}

