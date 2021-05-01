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
struct superblock* superBlock;//Represnts the inmemory superblock
bitmap_t inode_bitmap;
bitmap_t data_bitmap;
int superBlockblock = 0; //the superblock starts at 0
struct inode*  inode_mem; //represent the inmemory inode
struct superblock* s;
// Declare your in-memory data structures here

/* 
 * Get available inode number from bitmap
 */
int get_avail_ino() {
	printf("Getting into the get_avail_ino method\n");

	// Step 1: Read inode bitmap from disk
	/* 
	1. You need to know where the inode bitmap is and then call bioread 
	2. To do this first get the super block - done
	3. Then get the inode bitmap block
	4. traverse the inode
	5. return the i*/
	int a;

	// a = bio_read(superBlockblock, superBlock);
	// if(a<0){
	// 	perror("Problem reading into the supernode");
	// }
	// a = bio_read(superBlock1->i_bitmap_blk, inode_bitmap); //this the problem
	// // free(data_bitmap);
	// if(a<0){
	// 	printf("Problem reading into the inode");
	// 	perror("Problem reading into the inode");
	// }
	
	// Step 2: Traverse inode bitmap to find an available slot
	/* 
	1. This is pretty easy */

	int i; 

	for(i=0; i<superBlock->max_inum; i++){
		if(get_bitmap(inode_bitmap, i)==0){
			set_bitmap(inode_bitmap, i);
			break;
		}
	}
	if(i == superBlock->max_inum){
		perror("No available inodes");
		return -1;
	}
	// Step 3: Update inode bitmap and write to disk 
	// a = bio_write(superBlock->i_bitmap_blk, inode_bitmap);
	// if(a<0){
	// 	printf("Problem wriring into the inode");
	// 	perror("Problem wriitn into the inode");
	// }
	return i;
}

/* 
 * Get available data block number from bitmap
 */
int get_avail_blkno() {

	// Step 1: Read data block bitmap from disk
	printf("Getting into the get_avail_blkno method\n");

	// Step 1: Read inode bitmap from disk
	/* 
	1. You need to know where the inode bitmap is and then call bioread 
	2. To do this first get the super block - done
	3. Then get the inode bitmap block
	4. traverse the inode
	5. return the i*/
	int a;
	// a = bio_read(superBlockblock, superBlock);
	// if(a<0){
	// 	perror("Problem reading into the supernode");
	// }
	// a = bio_read(superBlock->d_bitmap_blk, data_bitmap); //this the problem
	// // free(data_bitmap);
	// if(a<0){
	// 	printf("Problem reading into the data");
	// 	perror("Problem reading into the data");
	// }
	
	// Step 2: Traverse inode bitmap to find an available slot
	/* 
	1. This is pretty easy */

	int i; 

	for(i=0; i<superBlock->max_dnum; i++){
		if(get_bitmap(data_bitmap, i)==0){
			// inode_bitmap1[i] = 1;
			set_bitmap(data_bitmap, i);
			break;
		}
	}
	if(i == superBlock->max_dnum){
		perror("No available data blocks");
		return -1;
	}
	// Step 3: Update inode bitmap and write to disk 
	// a = bio_write(superBlock1->d_bitmap_blk, data_bitmap1);
	// if(a<0){
	// 	printf("Problem wriring into the data");
	// 	perror("Problem wriitn into the data");
	// }
	// free(data_bitmap1);
	// free(superBlock);
	return i;
	// Step 2: Traverse data block bitmap to find an available slot

	// Step 3: Update data block bitmap and write to disk 
}

/* 
 * inode operations
 */
int readi(uint16_t ino, struct inode *inode) {

  // Step 1: Get the inode's on-disk block number
  /* 
  a. divide by total block nums to get the numeber of blcoks
  b. Then modulo it to find which one  */
  if(ino >= MAX_INUM){
	  perror("Invalid ino number\n");
	  return -1;
  }
  int n = MAX_INUM * sizeof(struct inode) / BLOCK_SIZE; //total no of blocks
  int inodesInEach = MAX_INUM/n;
  int blockNo = ino/inodesInEach;
  printf("The block number is %d\n", blockNo);
  ino = ino%inodesInEach;
  // Step 2: Get offset of the inode in the inode on-disk block

//   int offset = ino%sizeof(struct inode);
  int offset = ino;
  printf("The offset is %d\n", offset);

  // Step 3: Read the block from disk and then copy into inode structure
  return 0;
}

int writei(uint16_t ino, struct inode *inode) {

	// Step 1: Get the block number where this inode resides on disk
	if(ino >= MAX_INUM){
	  perror("Invalid ino number\n");
	  return -1;
  	}
  	int n = MAX_INUM * sizeof(struct inode) / BLOCK_SIZE; //total no of blocks
  	int inodesInEach = MAX_INUM/n;
  	int blockNo = ino/inodesInEach;
  	printf("The block number is %d\n", blockNo);
  	ino = ino%inodesInEach;
	// Step 2: Get the offset in the block where this inode resides on disk
	int offset = ino;
  	printf("The offset is %d\n", offset);
	// Step 3: Write inode to disk 

	return 0;
}


/* 
 * directory operations
 */
int dir_find(uint16_t ino, const char *fname, size_t name_len, struct dirent *dirent) {

  // Step 1: Call readi() to get the inode using ino (inode number of current directory)

  // Step 2: Get data block of current directory from inode

  // Step 3: Read directory's data block and check each directory entry.
  //If the name matches, then copy directory entry to dirent structure

	return 0;
}

int dir_add(struct inode dir_inode, uint16_t f_ino, const char *fname, size_t name_len) {

	// Step 1: Read dir_inode's data block and check each directory entry of dir_inode
	
	// Step 2: Check if fname (directory name) is already used in other entries

	// Step 3: Add directory entry in dir_inode's data block and write to disk

	// Allocate a new data block for this directory if it does not exist

	// Update directory inode

	// Write directory entry

	return 0;
}

int dir_remove(struct inode dir_inode, const char *fname, size_t name_len) {

	// Step 1: Read dir_inode's data block and checks each directory entry of dir_inode
	
	// Step 2: Check if fname exist

	// Step 3: If exist, then remove it from dir_inode's data block and write to disk

	return 0;
}

/* 
 * namei operation
 */
int get_node_by_path(const char *path, uint16_t ino, struct inode *inode) {
	
	// Step 1: Resolve the path name, walk through path, and finally, find its inode.
	// Note: You could either implement it in a iterative way or recursive way

	return 0;
}

/* 
 * Make file system
 */
int tfs_mkfs() {

	// Call dev_init() to initialize (Create) Diskfile
	dev_init(diskfile_path); // this initialzes our "disk" - should create a diskfile
	dev_open(diskfile_path); // opens the disk though we didn't need to do this
	void* buffer; // buffer to put stuff in
	// write superblock information

	/* 
	1. space for the superblock is there on the diskfile
	2. Set the memory as that
	3. Put all the stuff in that would need to be copied */

	superBlock = malloc(sizeof(struct superblock)); 
	superBlock->magic_num = MAGIC_NUM;
	superBlock->max_inum = MAX_INUM;
	superBlock->max_dnum = MAX_DNUM;
	superBlock->i_bitmap_blk = superBlockblock+1;
	superBlock->d_bitmap_blk = superBlock->i_bitmap_blk + 1;
	superBlock->i_start_blk = superBlock->d_bitmap_blk + 1;
	//we need to calculate how many blocks we need for inodes
	/* The size of one inode is 256 Bytes and there are 1024 no of inodes. Ofc this can easily be changed. So going to do the calculation now */
	int n = MAX_INUM * sizeof(struct inode) / BLOCK_SIZE;
	superBlock->d_start_blk = superBlock->d_bitmap_blk+n;
	printf("The number of inode blocks are %d\n", n);
	//superblock infomration done

	int a;
	// a = bio_write(0, superBlock); //0th block to the superblock
	// if(a<=0){
	// 	perror("No bytes were written\n");
	// 	return -1;
	// }

	//write to the disk compelete

	// initialize inode bitmap
	inode_bitmap = malloc((superBlock->max_inum)*sizeof(char));//have to memset
	memset(inode_bitmap, 0, superBlock->max_inum);


	// a = bio_write(superBlock->i_bitmap_blk, inode_bitmap);
	// if(a<=0){
	// 	perror("No bytes were read\n");
	// 	return -1;
	// }
	// initialize data block bitmap
	data_bitmap = malloc((superBlock->max_dnum)*sizeof(char));//have to memset
	memset(data_bitmap, 0, superBlock->max_dnum);

	// a = bio_write(superBlock->d_bitmap_blk, data_bitmap);
	// if(a<=0){
	// 	perror("No bytes were read\n");
	// 	return -1;
	// }


	// update bitmap information for root directory
	/* inode for the root directory. The first inode is for the root directory. 
	Use the set inode function to do this. make that
	1. Make an inode. Initialize all attributes. Write to the disk drive. The zeroth entry in the table would be the roor directot */

	int nextAvail = get_avail_ino();
	if(nextAvail < 0){
		perror("No avaialable inode");
		return -1;
	}
	
	struct inode* rootDir = malloc(sizeof(struct inode));
	rootDir->ino = nextAvail;
	rootDir->valid = 0;
	rootDir->size = 0;
	rootDir->type = 0;
	rootDir->link = 0;


	int i;
	for(i=0; i<sizeof(rootDir->direct_ptr)/sizeof(rootDir->direct_ptr[0]); i++){
		rootDir->direct_ptr[i] = 0;
	}
	for(i=0; i<sizeof(rootDir->indirect_ptr)/sizeof(rootDir->indirect_ptr[0]); i++){
		rootDir->indirect_ptr[i] = 0;
	}
	// free(data_bitmap);
	// free(superBlock);
	printf("Free successful\n");
	// update inode for root directory - no idea - just update the proerties. 
	
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
	int d = open(diskfile_path, O_RDWR, S_IRUSR | S_IWUSR);
  if(d < 0){ //maybe check this
	printf("The disk was created\n");
  	tfs_mkfs();
  }
  /* What are the in-memory data structures that we would like to initialize 
  superblock - whenever we change it biowrite
  malloc bitmaps
  in emort inode
  */
 	// superBlock = malloc(sizeof(superBlock));
	// if(superBlock == NULL){
	// 	printf("Superblock memory alloc failed\n");
	// }
	// else{
	// 	printf("Successful alloc 1\n");
	// }
	// inode_bitmap = malloc(superBlock->max_inum);
	// if(inode_bitmap == NULL){
	// 	printf("inode_bitmap memory alloc failed\n");
	// }
	// else{
	// 	printf("Successful alloc 2\n");
	// }
	// data_bitmap = malloc(superBlock->max_dnum); //char is 1 byte anyways
	// if(data_bitmap == NULL){
	// 	printf("data_bitmap memory alloc failed\n");
	// }
	// else{
	// 	printf("Successful alloc 3\n");
	// }
	// inode_mem = malloc(sizeof(struct inode));
	readi(18, NULL);
	readi(1023, NULL);
	readi(1024, NULL);
	readi(0, NULL);
	
	return NULL;
}

static void tfs_destroy(void *userdata) {

	// Step 1: De-allocate in-memory data structures
	printf("Are you called?\n");
	free(superBlock);
	free(inode_bitmap);
	free(data_bitmap);
	free(inode_mem);

	// Step 2: Close diskfile
	dev_close();

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
	// char* a = malloc(PATH_MAX);
	// strcpy(a, diskfile_path);
	// strcat(a, "/output.txt");
    // fd = open(a, O_WRONLY | O_APPEND | O_CREAT, 0644);
	// printf("Proof that the output file was created %d\n", fd);
	strcat(diskfile_path, "/DISKFILE");

	fuse_stat = fuse_main(argc, argv, &tfs_ope, NULL);
	return fuse_stat;
}

