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

char diskfile_path[PATH_MAX];
bitmap_t dataBlockMap = NULL;
bitmap_t inodeMap = NULL;
struct superblock* superblock = NULL;

// Declare your in-memory data structures here

/* 
 * Get available inode number from bitmap
 */
int get_avail_ino() {

	// Step 1: Read inode bitmap from disk
	
	// Step 2: Traverse inode bitmap to find an available slot

	// Step 3: Update inode bitmap and write to disk 
	for(int i = 0;i<MAX_INUM;i++)
	{
		if(get_bitmap(inodeMap,i)==0)
		{
			set_bitmap(inodeMap,i);
			bio_write(superblock->i_bitmap_blk, inodeMap);
			return i;
		}
	}
	return -1;
}

/* 
 * Get available data block number from bitmap
 */
int get_avail_blkno() {

	// Step 1: Read data block bitmap from disk
	
	// Step 2: Traverse data block bitmap to find an available slot

	// Step 3: Update data block bitmap and write to disk 

	for(int i = 0;i<MAX_INUM;i++)
	{
		if(get_bitmap(dataBlockMap,i)==0)
		{
			set_bitmap(dataBlockMap,i);
			bio_write(superblock->d_bitmap_blk, dataBlockMap);
			return i;
		}
	}
	return -1;
}

/* 
 * inode operations
 */
int readi(uint16_t ino, struct inode *inode) {

	// Step 1: Get the inode's on-disk block number
	int inodeStart = superblock->i_start_blk;
	int offset = ino/ (BLOCK_SIZE/sizeof(struct inode));
	int number = inodeStart + offset;
	// Step 2: Get offset of the inode in the inode on-disk block
	int i_offset = ino % (BLOCK_SIZE/sizeof(struct inode));
	i_offset *= sizeof(struct inode);
	// Step 3: Read the block from disk and then copy into inode structure
	void* buffer = calloc(1,BLOCK_SIZE);
	bio_read(number,buffer);
	memcpy(inode, buffer+offset,sizeof(struct inode));
	free(buffer);
	return 0;
}

int writei(uint16_t ino, struct inode *inode) {

	// Step 1: Get the block number where this inode resides on disk
	int inodeStart = superblock->i_start_blk;
	int offset = ino / (BLOCK_SIZE/sizeof(struct inode));
	int number = inodeStart + offset;
	// Step 2: Get the offset in the block where this inode resides on disk
	int i_offset = ino % (BLOCK_SIZE/sizeof(struct inode));
	i_offset *= sizeof(struct inode);
	// Step 3: Write inode to disk 
	void* buffer = calloc(1,BLOCK_SIZE);
	bio_write(number,buffer);
	memcpy(inode, buffer+offset, sizeof(struct inode));
	free(buffer);
	return 0;
}


/* 
 * directory operations
 */
int dir_find(uint16_t ino, const char *fname, size_t name_len, struct dirent *dirent) {

	// Step 1: Call readi() to get the inode using ino (inode number of current directory)
	struct inode* inode = calloc(1,sizeof(struct inode));
	readi(ino,inode);
	// Step 2: Get data block of current directory from inode
	for(int i = 0;i<16;i++)
	{
		int dataBlock = inode->direct_ptr[i];
		void* block = calloc(1,BLOCK_SIZE);
		bio_read(dataBlock, block);
	
		// Step 3: Read directory's data block and check each directory entry.
		//If the name matches, then copy directory entry to dirent structure
		int dirs = BLOCK_SIZE / sizeof(struct dirent);
		struct dirent* current = calloc(1,sizeof(struct dirent));

		for (int j = 0;j<dirs;j++)
		{
			memcpy(current, block + j * sizeof(struct dirent), sizeof(struct dirent));
			if(current->valid == 1 && strlen(current->name) == name_len)
			{
				if(strcmp(current->name,fname) == 0)
				{
					*dirent = *current;
					free(current);
					free(block);
					free(inode);
					return 1;
				}
			}
		}
		free(current);
		free(block);
	}
	free(inode);
	return 0;
}	

int dir_add(struct inode dir_inode, uint16_t f_ino, const char *fname, size_t name_len) {

	// Step 1: Read dir_inode's data block and check each directory entry of dir_inode
	
	// Step 2: Check if fname (directory name) is already used in other entries

	// Step 3: Add directory entry in dir_inode's data block and write to disk

	// Allocate a new data block for this directory if it does not exist

	// Update directory inode

	// Write directory entry
	for(int i = 0;i<16;i++)
	{
		int dataBlock = dir_inode.direct_ptr[i];
		if(dataBlock==0)
		{
			int dirsPerBlock = BLOCK_SIZE / sizeof(struct dirent);
			struct dirent* newDir = calloc(dirsPerBlock,sizeof(struct dirent));
			newDir->ino = f_ino;
			memcpy(newDir->name,fname,name_len);
			newDir->valid  = 1;
			int newDataBlock = get_avail_blkno();
			bio_write(newDataBlock,newDir);
			struct inode* dirInode = calloc(1,sizeof(struct inode));
			*dirInode = dir_inode;
			dirInode->direct_ptr[i] = newDataBlock;
			writei(dirInode->ino,dirInode);
			free(newDir);
			return 1;
		}

		void* data = calloc(1,BLOCK_SIZE);
		bio_read(dataBlock,data);

		int dirsPerBlock = BLOCK_SIZE / sizeof(struct dirent);
		struct dirent* current = calloc(dirsPerBlock,sizeof(struct dirent));

		for(int j=0;j<dirsPerBlock;j++)
		{
			memcpy(current,data + j * sizeof(struct dirent),sizeof(struct dirent));
			if(current->valid==0)
			{
				current->ino = f_ino;
				memcpy(current->name,fname,name_len);
				current->valid = 1;
				memcpy(data+j*sizeof(struct dirent),current,sizeof(struct dirent));
				bio_write(dataBlock,data);
				free(current);
				free(data);
				return 1;
			}
		}
		free(current);
		free(data);
	}
	return 0;
}

int dir_remove(struct inode dir_inode, const char *fname, size_t name_len) {

	// Step 1: Read dir_inode's data block and checks each directory entry of dir_inode
	
	// Step 2: Check if fname exist

	// Step 3: If exist, then remove it from dir_inode's data block and write to disk
	for(int i=0;i<16;i++)
	{
		int dataBlock = dir_inode.direct_ptr[i];
		void* data = calloc(1,BLOCK_SIZE);
		bio_read(dataBlock,data);

		int dirsPerBlock = BLOCK_SIZE / sizeof(struct dirent);
		struct dirent* current = calloc(dirsPerBlock,sizeof(struct dirent));
		for(int j=0;j<dirsPerBlock;j++)
		{
			memcpy(current, data + j * sizeof(struct dirent), sizeof(struct dirent));
			if(current->valid == 1 && strlen(current->name) == name_len)
			{
				if(strcmp(current->name,fname)==0)
				{
					current->valid=0;
					memcpy(data + j * sizeof(struct dirent), current,sizeof(struct dirent));
					bio_write(dataBlock,data);
					free(current);
					free(data);
					return 1;
				}
			}
		}
		free(current);
		free(data);
	}
	return 0;
}

/* 
 * namei operation
 */
int get_node_by_path(const char *path, uint16_t ino, struct inode *inode) {
	
	// Step 1: Resolve the path name, walk through path, and finally, find its inode.
	// Note: You could either implement it in a iterative way or recursive way
	if(strlen(path)==1)
	{
		readi(0,inode);
		return 0;
	}
	char* pathcopy = calloc(1,strlen(path));
	strcpy(pathcopy,path);
	char* pathName = strtok(pathcopy+1,"/");
	int inodeNum = ino;
	struct dirent* dirent = calloc(1,sizeof(struct dirent));
	do{
		int found = dir_find(inodeNum,pathName,strlen(pathName),dirent);
		if(found==0)
		{
			return -1;
		}
		inodeNum = dirent->ino;
		pathName = strtok(NULL,"/");
	}while(pathName !=NULL);

	readi(inodeNum,inode);
	free(pathcopy);
	free(dirent);
	
	return 0;
}

/* 
 * Make file system
 */
int tfs_mkfs() {

	// Call dev_init() to initialize (Create) Diskfile
	dev_init(diskfile_path);
	// write superblock information
	superblock = calloc(1,sizeof(struct superblock));
	superblock->magic_num = MAGIC_NUM;
	superblock->max_inum = MAX_INUM;
	superblock->max_dnum = MAX_DNUM;
	superblock->i_bitmap_blk = 1;
	superblock->d_bitmap_blk = 2;
	superblock->i_start_blk = 3;
	// initialize inode bitmap
	int numBytes = MAX_INUM / 8;
	inodeMap = calloc(1,numBytes);
	
	// initialize data block bitmap
	int inoPerBlock = BLOCK_SIZE / sizeof(struct inode);
	int numBlocks = (MAX_INUM / inoPerBlock) + 1;
	if (MAX_INUM % inoPerBlock)
		numBlocks--;
	superblock ->d_start_blk = (superblock->i_start_blk) + numBlocks+1;
	bio_write(0,superblock);
	// update bitmap information for root directory
	set_bitmap(inodeMap,0);
	for(int i = 0;i<superblock->d_start_blk;i++)
		set_bitmap(dataBlockMap, i);
	bio_write(superblock->i_bitmap_blk,inodeMap);
	bio_write(superblock->d_bitmap_blk,dataBlockMap);

	for(int i = 0;i<numBlocks;i++)
	{
		void* inodeBlock = calloc(1,BLOCK_SIZE);
		bio_write((superblock->i_start_blk)+i,inodeBlock);
		free(inodeBlock);
	}
	// update inode for root directory
	struct inode* root = calloc(1,sizeof(struct inode));
	root->ino = 0;
	root ->valid = 1;
	root ->size = 0;
	root ->type = 0;
	root ->link = 2;
	root ->direct_ptr[16] = 0;
	root ->indirect_ptr[8] = 0;
	struct stat* stat = calloc(1,sizeof(struct stat));
	stat -> st_ino = 0;
	stat -> st_mode = __S_IFDIR | 0755;
	stat -> st_nlink = 2;
	stat -> st_uid = getuid();
	stat -> st_gid = getgid();
	stat -> st_atime = time(NULL);
	stat -> st_mtime = time(NULL);

	root ->vstat = *stat;
	writei(0,root);
	free(root);
	free(stat);
	return 0;
}


/* 
 * FUSE file operations
 */
static void *tfs_init(struct fuse_conn_info *conn) {

	// Step 1a: If disk file is not found, call mkfs
	int open = dev_open(diskfile_path);
	if(open!=0)
	{
		tfs_mkfs();
		return NULL;
	}
	// Step 1b: If disk file is found, just initialize in-memory data structures
	// and read superblock from disk
	void* buff = calloc(1,BLOCK_SIZE);
	bio_read(0,buff);

	struct superblock* sblock = calloc(1,sizeof(struct superblock));
	memcpy(sblock,buff,sizeof(struct superblock));

	if(sblock->magic_num != MAGIC_NUM)
	{
		free(buff);
		free(sblock);
		tfs_mkfs();
		return NULL;
	}

	superblock = calloc(1,sizeof(struct superblock));
	superblock = sblock;
	free(sblock);

	bio_read(1,buff);
	inodeMap = calloc(1,MAX_INUM / 8);
	memcpy(inodeMap,buff,MAX_INUM / 8);
	bio_read(2,buff);
	dataBlockMap = calloc(1,MAX_DNUM / 8);
	memcpy(dataBlockMap, buff, MAX_DNUM / 8);

	free(buff);
	return NULL;
}

static void tfs_destroy(void *userdata) {

	// Step 1: De-allocate in-memory data structures
	free(superblock);
	free(inodeMap);
	free(dataBlockMap);
	// Step 2: Close diskfile
	dev_close();
}

static int tfs_getattr(const char *path, struct stat *stbuf) {

	// Step 1: call get_node_by_path() to get inode from path

	// Step 2: fill attribute of file into stbuf from inode
	struct inode* inode = calloc(1,sizeof(struct inode));
	int found = get_node_by_path(path,0,inode);
	if(found==-1)
	{
		free(inode);
		return -ENOENT;
	}
	stbuf ->st_ino = inode->vstat.st_ino;
	stbuf->st_mode   = __S_IFDIR | 0755;
	stbuf->st_nlink  = 2;
	stbuf->st_uid = inode->vstat.st_uid;
	stbuf->st_gid = inode->vstat.st_gid;
	stbuf -> st_size = inode ->vstat.st_size;
	stbuf ->st_atime = inode->vstat.st_atime;
	time(&stbuf->st_mtime);

	free(inode);
	return 0;
}

static int tfs_opendir(const char *path, struct fuse_file_info *fi) {

	// Step 1: Call get_node_by_path() to get inode from path
	struct inode* inode = calloc(1,sizeof(struct inode));
	int found = get_node_by_path(path,0,inode);
	// Step 2: If not find, return -1
	free(inode);
	if(found==-1)
		return -ENOENT;
	else
    	return 0;
}

static int tfs_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {

	// Step 1: Call get_node_by_path() to get inode from path

	// Step 2: Read directory entries from its data blocks, and copy them to filler
	struct inode* inode = calloc(1,sizeof(struct inode));
	int found = get_node_by_path(path,0,inode);
	if(found==-1)
	{
		free(inode);
		return -ENOENT;
	}
	for(int i = 0;i<16;i++)
	{
		int dataBlock = inode->direct_ptr[i];
		void* block = calloc(1,BLOCK_SIZE);
		bio_read(dataBlock,block);

		int dirsPerBlock = BLOCK_SIZE / sizeof(struct dirent);
		struct dirent* current = calloc(dirsPerBlock,sizeof(struct dirent));
		for(int j = 0;j<dirsPerBlock;j++)
		{
			memcpy(current,block + j * sizeof(struct dirent), sizeof(struct dirent));
			if(current ->valid == 1)
			{
				char* name = calloc(1,strlen(current->name));
				memcpy(name,current->name,strlen(current->name));
				filler(buffer,name,NULL,0);
				free(name);
			}
		}
		free(current);
		free(block);
	}
	free(inode);
	return 0;
}


static int tfs_mkdir(const char *path, mode_t mode) {

	// Step 1: Use dirname() and basename() to separate parent directory path and target directory name
	char* dirName = calloc(1,strlen(path));
	dirName = dirname(path);
	char* baseName = calloc(1,strlen(path));
	strncpy(baseName,path,strlen(path));
	baseName = basename(baseName);

	// Step 2: Call get_node_by_path() to get inode of parent directory
	struct inode* dirNode = calloc(1,sizeof(struct inode));
	if(strcmp(dirName,".")==1)
		readi(0, dirNode);
	else
		get_node_by_path(dirName,0,dirNode);
	struct dirent* newDir = NULL;
	int found = dir_find(dirNode ->ino, baseName, strlen(baseName), newDir);
	if(found==0)
	{
		free(dirNode);
		free(baseName);
		free(dirName);
		return -ENOENT;
	}
	// Step 3: Call get_avail_ino() to get an available inode number
	int newInode = get_avail_ino();
	// Step 4: Call dir_add() to add directory entry of target directory to parent directory
	dir_add(*dirNode,newInode,baseName,strlen(baseName));
	// Step 5: Update inode for target directory
	struct inode* inode = calloc(1,sizeof(struct inode));
	inode -> ino = newInode;
	inode -> valid= 1;
	inode -> size = 0;
	inode -> type = 0;
	inode -> link = 2;
	inode ->direct_ptr[16] = 0;
	inode -> indirect_ptr[8] = 0;

	struct stat* stat = calloc(1,sizeof(struct stat));
	stat -> st_ino = inode->ino;
	stat -> st_mode = __S_IFDIR | mode;
	stat -> st_nlink = 2;
	stat -> st_uid = getuid();
	stat -> st_gid = getgid();
	stat ->st_size = 0;
	stat ->st_atime = time(NULL);
	stat ->st_mtime = time(NULL);

	inode->vstat = *stat;
	// Step 6: Call writei() to write inode to disk
	writei(newInode,inode);
	
	free(dirName);
	free(baseName);
	free(stat);
	free(inode);
	free(dirNode);

	return 0;
}

static int tfs_rmdir(const char *path) {

	// Step 1: Use dirname() and basename() to separate parent directory path and target directory name
	char* dirName = calloc(1,strlen(path));
	dirName = dirname(path);
	char* baseName = calloc(1,strlen(path));
	strncpy(baseName,path,strlen(path));
	baseName = basename(baseName);
	// Step 2: Call get_node_by_path() to get inode of target directory
	struct inode* targetNode = calloc(1,sizeof(struct inode));
	int found = get_node_by_path(path,0,targetNode);
	if(found==-1)
	{
		free(targetNode);
		return -ENOENT;
	}
	// Step 3: Clear data block bitmap of target directory
	for(int i = 0;i<16;i++)
	{
		int blockNum = targetNode ->direct_ptr[i];
		unset_bitmap(dataBlockMap,blockNum);
	}
	bio_write(superblock->d_bitmap_blk,dataBlockMap);
	// Step 4: Clear inode bitmap and its data block
	unset_bitmap(inodeMap,targetNode->ino);
	bio_write(superblock->i_bitmap_blk,inodeMap);

	targetNode -> valid = 0;
	writei(targetNode -> ino, targetNode);
	free(targetNode);

	// Step 5: Call get_node_by_path() to get inode of parent directory
	struct inode* parentNode = calloc(1,sizeof(struct inode));
	get_node_by_path(dirName,0,parentNode);
	// Step 6: Call dir_remove() to remove directory entry of target directory in its parent directory
	dir_remove(*parentNode,baseName,strlen(baseName));
	
	free(dirName);
	free(baseName);
	free(parentNode);
	free(targetNode);
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
	dirName = dirname(path);
	char* baseName = calloc(1,strlen(path));
	strncpy(baseName,path,strlen(path));
	baseName = basename(baseName);
	// Step 2: Call get_node_by_path() to get inode of parent directory
	struct inode* dirNode = calloc(1,sizeof(struct inode));
	if(strcmp(dirName,".")==1)
		readi(0,dirNode);
	else
		get_node_by_path(dirName,0,dirNode);
	struct dirent* newDir = NULL;
	int found = dir_find(dirNode->ino,baseName,strlen(baseName),newDir);
	if(found==0)
	{
		free(dirName);
		free(baseName);
		free(dirNode);
		return -ENOENT;
	}
	// Step 3: Call get_avail_ino() to get an available inode number
	int inodeNum = get_avail_ino();
	// Step 4: Call dir_add() to add directory entry of target file to parent directory
	dir_add(*dirNode,inodeNum,baseName,strlen(baseName));
	// Step 5: Update inode for target file
	struct inode* newNode = calloc(1,sizeof(struct inode));

	newNode -> ino = inodeNum;
	newNode -> valid = 1;
	newNode -> size = 0;
	newNode -> type = 1;
	newNode -> link = 1;
	newNode -> direct_ptr[16] = 0;
	newNode -> indirect_ptr[8] = 0;

	struct stat* stat = calloc(1,sizeof(struct stat));
	stat -> st_ino = newNode->ino;
	stat -> st_mode = __S_IFDIR | mode;
	stat -> st_nlink = 2;
	stat -> st_uid = getuid();
	stat -> st_gid = getgid();
	stat ->st_size = 0;
	stat ->st_atime = time(NULL);
	stat ->st_mtime = time(NULL);

	newNode->vstat = *stat;
	// Step 6: Call writei() to write inode to disk
	writei(inodeNum,newNode);

	free(stat);
	free(dirNode);
	free(newNode);
	free(dirName);
	free(baseName);
	return 0;
}

static int tfs_open(const char *path, struct fuse_file_info *fi) {

	// Step 1: Call get_node_by_path() to get inode from path
	struct inode* inode = calloc(1,sizeof(struct inode));
	int found = get_node_by_path(path,0,inode);
	// Step 2: If not find, return -1
	free(inode);
	if(found==-1)
		return -ENOENT;
	else
		return 0;
}

static int tfs_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {

	// Step 1: You could call get_node_by_path() to get inode from path
	struct inode* inode = calloc(1,sizeof(struct inode));
	int found = get_node_by_path(path,0,inode);
	if(found == -1)
	{
		free(inode);
		return -ENOENT;
	}
	// Step 2: Based on size and offset, read its data blocks from disk
	int current = offset/BLOCK_SIZE;
	int blockOffset = offset % BLOCK_SIZE;

	int amountLeft = size;
	while(current < 16 && amountLeft > 0)
	{
		int blockNum = inode->direct_ptr[current];
		void* data = calloc(1,BLOCK_SIZE);
		bio_read(blockNum,data);

		// Step 3: copy the correct amount of data from offset to buffer
		int copyAmount;
		if(blockOffset != 0)
		{
			copyAmount = BLOCK_SIZE-blockOffset;
			if(copyAmount > copyAmount)
				copyAmount = copyAmount;
			memcpy(buffer + size - amountLeft,data + blockOffset,copyAmount);
			blockOffset=0;
		}
		else
		{
			if(amountLeft > BLOCK_SIZE)
				copyAmount = BLOCK_SIZE;
			else
				copyAmount = amountLeft;
			memcpy(buffer + size - amountLeft, data, copyAmount);
		}

		amountLeft -=copyAmount;
		current++;
		free(data);
	}
	free(inode);
	// Note: this function should return the amount of bytes you copied to buffer
	return size-amountLeft;
	
}

static int tfs_write(const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {
	// Step 1: You could call get_node_by_path() to get inode from path
	struct inode* inode = calloc(1,sizeof(struct inode));
	int found = get_node_by_path(path,0,inode);
	if(found==-1)
	{
		free(inode);
		return -ENOENT;
	}
	// Step 2: Based on size and offset, read its data blocks from disk
	int current = offset / BLOCK_SIZE;
	int blockOffset = offset % BLOCK_SIZE;

	int amountLeft = size;
	while (current < 16 && amountLeft >0)
	{
		int blockNum = inode->direct_ptr[current];
		if(dataBlockMap == 0)
		{
			blockNum = get_avail_blkno();
			inode -> direct_ptr[current] = blockNum;
		}
		void* data = calloc(1,BLOCK_SIZE);
		bio_read(blockNum, data);

		// Step 3: Write the correct amount of data from offset to disk
		int copyAmount;
		if(blockOffset != 0)
		{
			copyAmount = BLOCK_SIZE - blockOffset;
			if(copyAmount > amountLeft)
				copyAmount = amountLeft;
			memcpy(data + blockOffset, buffer + size - amountLeft,copyAmount);
			bio_write(blockNum,data);
			blockOffset = 0;
		}
		else
		{
			if(amountLeft > BLOCK_SIZE)
				copyAmount = BLOCK_SIZE;
			else
				copyAmount = amountLeft;
			memcpy(data,buffer+size-amountLeft,copyAmount);
			bio_write(blockNum,data);
		}
		amountLeft -=copyAmount;
		current++;
		free(data);
	}
	// Step 4: Update the inode info and write it to disk
	inode->size += size-amountLeft;
	(inode->vstat).st_size += size - amountLeft;

	writei(inode->ino,inode);
	free(inode);
	// Note: this function should return the amount of bytes you write to disk
	return size-amountLeft;
}

static int tfs_unlink(const char *path) {

	// Step 1: Use dirname() and basename() to separate parent directory path and target file name
	char* dirName = calloc(1,strlen(path));
	dirName = dirname(path);
	char* baseName = calloc(1,strlen(path));
	strcpy(baseName,path,strlen(path));
	baseName = basename(baseName);
	// Step 2: Call get_node_by_path() to get inode of target file
	struct inode* targetNode = calloc(1,sizeof(struct inode));
	int found = get_node_by_path(path,0,targetNode);
	if(found==-1)
	{
		free(targetNode);
		return -ENOENT;
	}
	// Step 3: Clear data block bitmap of target file
	for(int i = 0;i<16;i++)
	{
		int blockNum = targetNode->direct_ptr[i];
		unset_bitmap(dataBlockMap,blockNum);
	}

	bio_write(superblock->d_bitmap_blk,dataBlockMap);

	// Step 4: Clear inode bitmap and its data block
	unset_bitmap(inodeMap,targetNode->ino);
	bio_write(superblock->i_bitmap_blk,inodeMap);

	targetNode ->valid = 0;
	writei(targetNode->ino,targetNode);
	free(targetNode);
	// Step 5: Call get_node_by_path() to get inode of parent directory
	struct inode* parentNode = calloc(1,sizeof(struct inode));
	get_node_by_path(dirName,0,parentNode);
	// Step 6: Call dir_remove() to remove directory entry of target file in its parent directory
	dir_remove(*parentNode,baseName,strlen(baseName));
	free(dirName);
	free(baseName);
	free(parentNode);
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

