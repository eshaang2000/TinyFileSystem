/*
 *  Copyright (C) 2021 CS416 Rutgers CS
 *	Tiny File System
 *	File:	tfs.c
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

// Declare your in-memory data structures here

/* 
 * Get available inode number from bitmap
 */
int get_avail_ino() {

	// Step 1: Read inode bitmap from disk
	
	// Step 2: Traverse inode bitmap to find an available slot

	// Step 3: Update inode bitmap and write to disk 
  
	return 0;
}

/* 
 * Get available data block number from bitmap
 */
int get_avail_blkno() {

	// Step 1: Read data block bitmap from disk
	
	// Step 2: Traverse data block bitmap to find an available slot

	// Step 3: Update data block bitmap and write to disk 

	return 0;
}

/* 
 * inode operations
 */
int readi(uint16_t ino, struct inode *inode) {

  // Step 1: Get the inode's on-disk block number
  uint16_t i_block_num = ino / (BLOCK_SIZE / sizeof(struct inode));//Finds i_block
  // Step 2: Get offset of the inode in the inode on-disk block
  uint16_t inode_num = ino % (BLOCK_SIZE / sizeof(struct inode));//get inode within i_block
  // Step 3: Read the block from disk and then copy into inode structure
  void *buf = malloc(BLOCK_SIZE);
  bio_read(SuperBlock->i_start_blk + block_num, buf);//read i_block from disk into buf
  *inode = *((struct inode*) buf + inode_num);//sets *inode to inode in buf
  free(buf);
  return 0;
}

int writei(uint16_t ino, struct inode *inode) {

  // Step 1: Get the block number where this inode resides on disk
  uint32_t i_block_num = ino / (BLOCK_SIZE / sizeof(struct inode));//Finds i_block
  // Step 2: Get the offset in the block where this inode resides on disk
  uint16_t inode_num = ino % (BLOCK_SIZE / sizeof(struct inode));//get inode within i_block
  // Step 3: Write inode to disk 
  i_block_num += superBlock->i_start_blk;
  void* buf = malloc(BLOCK_SIZE);
  bio_read(i_block_num, buf);//Read the whole iblock
  struct inode *ptr = (struct inode) buf;
  ptr += inode_num;
  *ptr = *inode;  
  bio_write(i_block_num, buf);
  return 0;
}


/* 
 * directory operations
 */
int dir_find(uint16_t ino, const char *fname, size_t name_len, struct dirent *dirent) {

  // Step 1: Call readi() to get the inode using ino (inode number of current directory)

  // Step 2: Get data block of current directory from inode
  //directory inode type = 1, else = 0
 
  // Step 3: Read directory's data block and check each directory entry.
  //If the name matches, then copy directory entry to dirent structure

  struct dirent *de;
  struct inode i_node;
  void *buffer;
  int i, j;

  readi(ino, &i_node);
  
  for(i = 0; i < 16; i++){
    if(i_node->direct_ptr[i] == -1){//Not sure if need this
      continue;
    }
    bio_read(superBlock->d_start_blk + i_node->direct_ptr[i], buffer);
    de = (struct dirent) buffer;
    for(j = 0; j < BLOCK_SIZE / sizeof(struct dirent); j += sizeof(struct dirent)){
      if(!strcmp(fname, de->name)){
	//Found a match, copy to *dirent
        *dirent = *de;
	return 1;
      }
      de++;
    }
  }
  return 0;
}

int dir_add(struct inode dir_inode, uint16_t f_ino, const char *fname, size_t name_len) {

  // Step 1: Read dir_inode's data block and check each directory entry of dir_inode
	
  // Step 2: Check if fname (directory name) is already used in other entries
  
  // Step 3: Add directory entry in dir_inode's data block and write to disk
  
  // Allocate a new data block for this directory if it does not exist

  // Update directory inode

  // Write directory entry
  void *buffer;
  int i, j, blk_num_i;//index of direct_ptr in inode where the valid dirent is in
  struct dirent *save, *de, *new = malloc(sizeof(struct dirent));//save will point to the first valid dirent in the direct_ptr in inode
  new->ino = f_ino;
  new->name = fname;
  new->len = name_len;
  new->valid = 1;
  
  for(i = 0; i < 16; i++){
    bio_read(superBlock->d_start_blk + dir_inode->direct_ptr[i], buffer);
    de = (struct dirent) buffer;
    for(j = 0; j < BLOCK_SIZE / sizeof(struct dirent); j += sizeof(struct dirent)){
      if(!strcmp(fname, de->name)){
	//Found a match, can't add
	return -1;
      }
      if(!(de->valid)){
	blk_num_i = dir_inode->direct_ptr[i];
	save = de;
      }
      de++;
    }
  }
  
  *save = *new;
  bio_write(superBlock->d_start_blk + dir_inode->direct_ptr[blk_num_i], buffer);//writes updated data block to disk
  /*End of Step 3*/
  struct inode temp;
  readi(new->ino, &temp);
  temp->direct_ptr[0] = get_avail_blkno();
  writei(temp->ino, &temp);
  return 0;
}

int dir_remove(struct inode dir_inode, const char *fname, size_t name_len) {

  // Step 1: Read dir_inode's data block and checks each directory entry of dir_inode
  
  // Step 2: Check if fname exist

  // Step 3: If exist, then remove it from dir_inode's data block and write to disk
  
  struct dirent *de;
  void *buffer;
  int i, j;
  for(i = 0; i < 16; i++){
    bio_read(superBlock->d_start_blk + dir_inode->direct_ptr[i], buffer);
    de = (struct dirent) buffer;
    for(j = 0; j < BLOCK_SIZE / sizeof(struct dirent); j += sizeof(struct dirent)){
      if(!strcmp(fname, de->name)){
	//Found a match
	de->valid = 0;
	bio_write(superBlock->d_start_blk + dir_inode->direct_ptr[i], buffer);
	return 1;
      }
      de++;
    }
  }	

  return 0;
}

/* 
 * namei operation
 */
int get_node_by_path(const char *path, uint16_t ino, struct inode *inode) {
	
	// Step 1: Resolve the path name, walk through path, and finally, find its inode.
	// Note: You could either implement it in a iterative way or recursive way
  if(){

  }
  const char s[2] = "/";
  
	return 0;
}

/* 
 * Make file system
 */
int tfs_mkfs() {

	// Call dev_init() to initialize (Create) Diskfile
	dev_init(diskfile_path);
	// write superblock information

	// initialize inode bitmap

	// initialize data block bitmap

	// update bitmap information for root directory

	// update inode for root directory
	// printf("Eshaan is the best\n");

	return 0;
}


/* 
 * FUSE file operations
 */
static void *tfs_init() {
//struct fuse_conn_info *conn
	// Step 1a: If disk file is not found, call mkfs
  // Step 1b: If disk file is found, just initialize in-memory data structures
  // and read superblock from disk
  	tfs_mkfs();
	printf("Eshaan is the best\n");
	return NULL;
}

static void tfs_destroy(void *userdata) {

	// Step 1: De-allocate in-memory data structures

	// Step 2: Close diskfile

}

static int tfs_getattr(const char *path, struct stat *stbuf) {

	// Step 1: call get_node_by_path() to get inode from path

	// Step 2: fill attribute of file into stbuf from inode

		stbuf->st_mode   = S_IFDIR | 0755;
		stbuf->st_nlink  = 2;
		time(&stbuf->st_mtime);

	return 0;
}

static int tfs_opendir(const char *path, struct fuse_file_info *fi) {

	// Step 1: Call get_node_by_path() to get inode from path

	// Step 2: If not find, return -1

    return 0;
}

static int tfs_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {

	// Step 1: Call get_node_by_path() to get inode from path

	// Step 2: Read directory entries from its data blocks, and copy them to filler

	return 0;
}


static int tfs_mkdir(const char *path, mode_t mode) {

	// Step 1: Use dirname() and basename() to separate parent directory path and target directory name

	// Step 2: Call get_node_by_path() to get inode of parent directory

	// Step 3: Call get_avail_ino() to get an available inode number

	// Step 4: Call dir_add() to add directory entry of target directory to parent directory

	// Step 5: Update inode for target directory

	// Step 6: Call writei() to write inode to disk
	
	printf("Eshaan\n");
	return 0;
}

static int tfs_rmdir(const char *path) {

	// Step 1: Use dirname() and basename() to separate parent directory path and target directory name

	// Step 2: Call get_node_by_path() to get inode of target directory

	// Step 3: Clear data block bitmap of target directory

	// Step 4: Clear inode bitmap and its data block

	// Step 5: Call get_node_by_path() to get inode of parent directory

	// Step 6: Call dir_remove() to remove directory entry of target directory in its parent directory

	return 0;
}

static int tfs_releasedir(const char *path, struct fuse_file_info *fi) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
    return 0;
}

static int tfs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {

	// Step 1: Use dirname() and basename() to separate parent directory path and target file name

	// Step 2: Call get_node_by_path() to get inode of parent directory

	// Step 3: Call get_avail_ino() to get an available inode number

	// Step 4: Call dir_add() to add directory entry of target file to parent directory

	// Step 5: Update inode for target file

	// Step 6: Call writei() to write inode to disk

	return 0;
}

static int tfs_open(const char *path, struct fuse_file_info *fi) {

	// Step 1: Call get_node_by_path() to get inode from path

	// Step 2: If not find, return -1

	return 0;
}

static int tfs_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {

	// Step 1: You could call get_node_by_path() to get inode from path

	// Step 2: Based on size and offset, read its data blocks from disk

	// Step 3: copy the correct amount of data from offset to buffer

	// Note: this function should return the amount of bytes you copied to buffer
	return 0;
}

static int tfs_write(const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {
	// Step 1: You could call get_node_by_path() to get inode from path

	// Step 2: Based on size and offset, read its data blocks from disk

	// Step 3: Write the correct amount of data from offset to disk

	// Step 4: Update the inode info and write it to disk

	// Note: this function should return the amount of bytes you write to disk
	return size;
}

static int tfs_unlink(const char *path) {

	// Step 1: Use dirname() and basename() to separate parent directory path and target file name

	// Step 2: Call get_node_by_path() to get inode of target file

	// Step 3: Clear data block bitmap of target file

	// Step 4: Clear inode bitmap and its data block

	// Step 5: Call get_node_by_path() to get inode of parent directory

	// Step 6: Call dir_remove() to remove directory entry of target file in its parent directory

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
	printf("This is how you start\n");
	getcwd(diskfile_path, PATH_MAX);
	strcat(diskfile_path, "/DISKFILE");

	fuse_stat = fuse_main(argc, argv, &tfs_ope, NULL);

	return fuse_stat;
}

