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
struct superblock * superBlock; //Represnts the inmemory superblock
bitmap_t inode_bitmap;
bitmap_t data_bitmap;
int superBlockblock = 0; //the superblock starts at 0
struct inode * inode_mem; //represent the inmemory inode
struct superblock * s;
// Declare your in-memory data structures here

/* 
 * Get available inode number from bitmap
 */
int get_avail_ino() {
    printf("Getting into the get_avail_ino method\n");
    // Step 1: Read inode bitmap from disk
    int a;
    void * buffer;
    buffer = malloc(BLOCK_SIZE);
    a = bio_read(0, buffer);
    if (a < 0) {
        perror("Problem reading into the supernode");
        return -1;
    }
    superBlock -> magic_num = 77;
    printf("The superblock location %d\n", superBlock -> magic_num);
    printf("The superblock location %d\n", superBlock -> i_bitmap_blk);
    superBlock = memcpy(superBlock, buffer, sizeof(struct superblock));
    a = bio_read(superBlock -> i_bitmap_blk, buffer); //this the problem
    inode_bitmap = memcpy(inode_bitmap, buffer, MAX_INUM);
    printf("The superblock location %d\n", superBlock -> magic_num);
    if (a < 0) {
        perror("Problem reading into the inode");
        return -1;
    }

    // Step 2: Traverse inode bitmap to find an available slot

    int i;
    for (i = 0; i < superBlock -> max_inum; i++) {
        if (get_bitmap(inode_bitmap, i) == 0) {
            set_bitmap(inode_bitmap, i);
            break;
        }
    }
    if (i == superBlock -> max_inum) {
        perror("No available inodes");
        return -1;
    }
    // Step 3: Update inode bitmap and write to disk 
    buffer = memcpy(buffer, (void * ) inode_bitmap, sizeof( * inode_bitmap));
    a = bio_write(superBlock -> i_bitmap_blk, buffer);
    if (a < 0) {
        printf("Problem wriring into the inode");
        return -1;
    }
    free(buffer);
    return i;
}

/* 
 * Get available data block number from bitmap
 */
int get_avail_blkno() {

    printf("Getting into the get_avail_blkno method\n");
    // Step 1: Read inode bitmap from disk
    int a;
    void * buffer;
    buffer = malloc(BLOCK_SIZE);
    a = bio_read(0, buffer);
    if (a < 0) {
        perror("Problem reading into the supernode");
        return -1;
    }
    superBlock -> magic_num = 77;
    printf("The superblock location %d\n", superBlock -> magic_num);
    printf("The superblock location %d\n", superBlock -> d_bitmap_blk);
    superBlock = memcpy(superBlock, buffer, sizeof(struct superblock));
    a = bio_read(superBlock -> d_bitmap_blk, buffer); //this the problem
    data_bitmap = memcpy(data_bitmap, buffer, MAX_INUM);
    printf("The superblock location %d\n", superBlock -> magic_num);
    if (a < 0) {
        perror("Problem reading into the inode");
        return -1;
    }

    // Step 2: Traverse inode bitmap to find an available slot

    int i;
    for (i = 0; i < superBlock -> max_dnum; i++) {
        if (get_bitmap(data_bitmap, i) == 0) {
            set_bitmap(data_bitmap, i);
            break;
        }
    }
    if (i == superBlock -> max_dnum) {
        perror("No available data nodes");
        return -1;
    }
    // Step 3: Update inode bitmap and write to disk 
    buffer = memcpy(buffer, (void * ) data_bitmap, sizeof( * data_bitmap));
    a = bio_write(superBlock -> d_bitmap_blk, buffer);
    if (a < 0) {
        printf("Problem writing into the disk");
        return -1;
    }
    free(buffer);
    return i;
}

/* 
 * inode operations
 */
int readi(uint16_t ino, struct inode * inode) {

    //first check if this is a valid inode number or not 
    if (ino >= MAX_INUM) {
        perror("Invalid ino number\n");
        return -1;
    }

    // Step 1: Get the inode's on-disk block number

    int n = MAX_INUM * sizeof(struct inode) / BLOCK_SIZE; //total no of blocks
    int inodesInEach = MAX_INUM / n;
    int blockNo = ino / inodesInEach;
    printf("The block number is %d\n", blockNo);

    // Step 2: Get offset of the inode in the inode on-disk block
    int offset = ino % inodesInEach;
    printf("The offset is %d\n", offset);

    // Step 3: Read the block from disk and then copy into inode structure
    void * buffer = malloc(BLOCK_SIZE);
    int a; // to check if bio_read operations happened or not
    a = bio_read(blockNo, buffer);
    if (a < 0) {
        perror("Read not succesful");
        free(buffer);
        return -1;
    }
    int o = sizeof(struct inode) * offset; // this tells me where to memcpy from
    inode = memcpy(inode, buffer + o, sizeof(struct inode));
    free(buffer);
    return 0;
}

int writei(uint16_t ino, struct inode * inode) {

    // Step 1: Get the block number where this inode resides on disk
    if (ino >= MAX_INUM) {
        perror("Invalid ino number\n");
        return -1;
    }
    int n = MAX_INUM * sizeof(struct inode) / BLOCK_SIZE; //total no of blocks
    int inodesInEach = MAX_INUM / n;
    int blockNo = ino / inodesInEach;
    printf("The block number is %d\n", blockNo);
    // Step 2: Get the offset in the block where this inode resides on disk
    int offset = ino % inodesInEach;
    printf("The offset is %d\n", offset);
    // Step 3: Write inode to disk 

    void * buffer = malloc(BLOCK_SIZE);
    int o = sizeof(struct inode) * offset; // this tells me where to memcpy from
    void * tempBuf = NULL;
    tempBuf = memcpy(buffer + o, inode, sizeof(struct inode));
    int a; // to check if bio_read operations happened or not
    a = bio_write(blockNo, buffer);
    if (a < 0) {
        perror("Read not succesful");
        return -1;
    }
    tempBuf = NULL;
    free(buffer);
    return 0;
}

/* 
 * directory operations
 */
int dir_find(uint16_t ino,
    const char * fname, size_t name_len, struct dirent * dirent) {
    
    // Step 1: Call readi() to get the inode using ino (inode number of current directory)
    

    // Step 2: Get data block of current directory from inode

    // Step 3: Read directory's data block and check each directory entry.
    //If the name matches, then copy directory entry to dirent structure

    return 0;
}

int dir_add(struct inode dir_inode, uint16_t f_ino,
    const char * fname, size_t name_len) {

    // Step 1: Read dir_inode's data block and check each directory entry of dir_inode

    // Step 2: Check if fname (directory name) is already used in other entries

    // Step 3: Add directory entry in dir_inode's data block and write to disk

    // Allocate a new data block for this directory if it does not exist

    // Update directory inode

    // Write directory entry

    return 0;
}

int dir_remove(struct inode dir_inode,
    const char * fname, size_t name_len) {

    // Step 1: Read dir_inode's data block and checks each directory entry of dir_inode

    // Step 2: Check if fname exist

    // Step 3: If exist, then remove it from dir_inode's data block and write to disk

    return 0;
}

/* 
 * namei operation
 */
int get_node_by_path(const char * path, uint16_t ino, struct inode * inode) {

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
    void * buffer; // buffer to put stuff in

    // write superblock information

    superBlock = malloc(sizeof(struct superblock));
    superBlock -> magic_num = MAGIC_NUM;
    superBlock -> max_inum = MAX_INUM;
    superBlock -> max_dnum = MAX_DNUM;
    superBlock -> i_bitmap_blk = superBlockblock + 1;
    superBlock -> d_bitmap_blk = superBlock -> i_bitmap_blk + 1;
    superBlock -> i_start_blk = superBlock -> d_bitmap_blk + 1;
    //we need to calculate how many blocks we need for inodes
    /* The size of one inode is 256 Bytes and there are 1024 no of inodes. Ofc this can easily be changed. So going to do the calculation now */
    int n = MAX_INUM * sizeof(struct inode) / BLOCK_SIZE;
    superBlock -> d_start_blk = superBlock -> d_bitmap_blk + n;
    //superblock infomration done
    int a; // to check bio fucntions
    buffer = malloc(BLOCK_SIZE);
    buffer = memcpy(buffer, (void * ) superBlock, sizeof( * superBlock));
    a = bio_write(superBlockblock, buffer); //0th block to the superblock
    if (a <= 0) {
        perror("No bytes were written\n");
        return -1;
    }
    //write to the disk compelete - superblock

    // initialize inode bitmap
    inode_bitmap = malloc((superBlock -> max_inum) * sizeof(char));
    memset(inode_bitmap, 0, superBlock -> max_inum);
    buffer = memcpy(buffer, (void * ) inode_bitmap, sizeof( * inode_bitmap));
    a = bio_write(superBlock -> i_bitmap_blk, buffer);
    if (a <= 0) {
        perror("No bytes were read\n");
        return -1;
    }
    //write to disk complete - inode bitmap

    // initialize data block bitmap
    data_bitmap = malloc((superBlock -> max_dnum) * sizeof(char));
    memset(data_bitmap, 0, superBlock -> max_dnum);
    buffer = memcpy(buffer, (void * ) data_bitmap, sizeof( * data_bitmap));
    a = bio_write(superBlock -> d_bitmap_blk, buffer);
    if (a <= 0) {
        perror("No bytes were read\n");
        return -1;
    }
    //write to disk complete - data bitmap

    // update bitmap information for root directory
    int nextAvail = get_avail_ino();
    if (nextAvail < 0) {
        perror("No avaialable inode");
        return -1;
    }

    //update bitmap information for root directory done

    // update inode for root directory - no idea - just update the proerties. 

    /* inode for the root directory. The first inode is for the root directory. 
    Use the set inode function to do this. make that
    1. Make an inode. Initialize all attributes. Write to the disk drive. The zeroth entry in the table would be the roor directot */

    struct inode * rootDir = malloc(sizeof(struct inode));
    rootDir -> ino = nextAvail;
    rootDir -> valid = 0;
    rootDir -> size = 0;
    rootDir -> type = 0;
    rootDir -> link = 0;
    int i;
    for (i = 0; i < sizeof(rootDir -> direct_ptr) / sizeof(rootDir -> direct_ptr[0]); i++) {
        rootDir -> direct_ptr[i] = 0;
    }
    for (i = 0; i < sizeof(rootDir -> indirect_ptr) / sizeof(rootDir -> indirect_ptr[0]); i++) {
        rootDir -> indirect_ptr[i] = 0;
    }
    writei(rootDir -> ino, rootDir);
    free(rootDir);
    inode_mem = malloc(sizeof(struct inode));
    if (inode_mem == NULL) {
        printf("inode mem memory alloc failes");
    }
    printf("End of mkfs reached\n");
    return 0;
}

/* 
 * FUSE file operations
 */
static void * tfs_init() {
    //struct fuse_conn_info *conn
    // Step 1a: If disk file is not found, call mkfs
    // Step 1b: If disk file is found, just initialize in-memory data structures
    // and read superblock from disk
    int d = open(diskfile_path, O_RDWR, S_IRUSR | S_IWUSR);
    if (d < 0) { // This means that the disk is not there
        printf("The disk was created\n");
        tfs_mkfs();
    }
    /* What are the in-memory data structures that we would like to initialize 
    superblock - whenever we change it biowrite
    malloc bitmaps
    in emort inode
    */
    else {
        superBlock = malloc(sizeof(superBlock));
        if (superBlock == NULL) {
            printf("Superblock memory alloc failed\n");
        }
        inode_bitmap = malloc(superBlock -> max_inum);
        if (inode_bitmap == NULL) {
            printf("inode_bitmap memory alloc failed\n");
        }
        data_bitmap = malloc(superBlock -> max_dnum); //char is 1 byte anyways
        if (data_bitmap == NULL) {
            printf("data_bitmap memory alloc failed\n");
        }
        inode_mem = malloc(sizeof(struct inode));
        if (inode_mem == NULL) {
            printf("inode mem memory alloc failes");
        }
    }
    // char* a = malloc(100);
    char* a = "/eshaan/a.txt";
    char* b = strdup(a);
    printf("Seq\n");
    // char* b = malloc(100);
    char* c = dirname(b);

    printf("Seq\n");
    // b = basename("/ehsaan/a.txt");
    printf("This is the dirname %s\n", c);
    // printf("This is the dirname %s\n", b);

    return NULL;
}

static void tfs_destroy(void * userdata) {

    // Step 1: De-allocate in-memory data structures
    printf("We have entered tfs destroy\n");
    free(superBlock);
    free(inode_bitmap);
    free(data_bitmap);
    free(inode_mem);

    // Step 2: Close diskfile
    dev_close();

}

static int tfs_getattr(const char * path, struct stat * stbuf) {

    // Step 1: call get_node_by_path() to get inode from path

    // Step 2: fill attribute of file into stbuf from inode

    stbuf -> st_mode = S_IFDIR | 0755;
    stbuf -> st_nlink = 2;
    time( & stbuf -> st_mtime);

    return 0;
}

static int tfs_opendir(const char * path, struct fuse_file_info * fi) {

    // Step 1: Call get_node_by_path() to get inode from path

    // Step 2: If not find, return -1

    return 0;
}

static int tfs_readdir(const char * path, void * buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info * fi) {

    // Step 1: Call get_node_by_path() to get inode from path

    // Step 2: Read directory entries from its data blocks, and copy them to filler

    return 0;
}

static int tfs_mkdir(const char * path, mode_t mode) {

    // Step 1: Use dirname() and basename() to separate parent directory path and target directory name

    // Step 2: Call get_node_by_path() to get inode of parent directory

    // Step 3: Call get_avail_ino() to get an available inode number

    // Step 4: Call dir_add() to add directory entry of target directory to parent directory

    // Step 5: Update inode for target directory

    // Step 6: Call writei() to write inode to disk

    return 0;
}

static int tfs_rmdir(const char * path) {

    // Step 1: Use dirname() and basename() to separate parent directory path and target directory name

    // Step 2: Call get_node_by_path() to get inode of target directory

    // Step 3: Clear data block bitmap of target directory

    // Step 4: Clear inode bitmap and its data block

    // Step 5: Call get_node_by_path() to get inode of parent directory

    // Step 6: Call dir_remove() to remove directory entry of target directory in its parent directory

    return 0;
}

static int tfs_releasedir(const char * path, struct fuse_file_info * fi) {
    // For this project, you don't need to fill this function
    // But DO NOT DELETE IT!
    return 0;
}


static int tfs_create(const char * path, mode_t mode, struct fuse_file_info * fi) {

    // Step 1: Use dirname() and basename() to separate parent directory path and target file name
    char* tempD = strdup(path);
    char* tempB = strdup(path);
    char* dname = dirname(tempD);
    char* bname = basename(tempB);
    uint16_t inodeNumber = 0;
    //How do you get the inodenumber of the root of this path

    // Step 2: Call get_node_by_path() to get inode of parent directory

    get_node_by_path(path, inodeNumber, inode_mem);

    // Step 3: Call get_avail_ino() to get an available inode number

    int nextAvail = get_avail_ino();

    // Step 4: Call dir_add() to add directory entry of target file to parent directory

    // Step 5: Update inode for target file

    // Step 6: Call writei() to write inode to disk

    return 0;
}

static int tfs_open(const char * path, struct fuse_file_info * fi) {

    // Step 1: Call get_node_by_path() to get inode from path
    int rootInode = 0;
    int a = get_node_by_path(path, rootInode, inode_mem);

    // Step 2: If not find, return -1
    if(a == -1){
        return -1;
    }

    return 0;
}

static int tfs_read(const char * path, char * buffer, size_t size, off_t offset, struct fuse_file_info * fi) {

    // Step 1: You could call get_node_by_path() to get inode from path

    // Step 2: Based on size and offset, read its data blocks from disk

    // Step 3: copy the correct amount of data from offset to buffer

    // Note: this function should return the amount of bytes you copied to buffer
    return 0;
}

static int tfs_write(const char * path,
    const char * buffer, size_t size, off_t offset, struct fuse_file_info * fi) {
    // Step 1: You could call get_node_by_path() to get inode from path

    // Step 2: Based on size and offset, read its data blocks from disk

    // Step 3: Write the correct amount of data from offset to disk

    // Step 4: Update the inode info and write it to disk

    // Note: this function should return the amount of bytes you write to disk
    return size;
}

static int tfs_unlink(const char * path) {

    // Step 1: Use dirname() and basename() to separate parent directory path and target file name

    // Step 2: Call get_node_by_path() to get inode of target file

    // Step 3: Clear data block bitmap of target file

    // Step 4: Clear inode bitmap and its data block

    // Step 5: Call get_node_by_path() to get inode of parent directory

    // Step 6: Call dir_remove() to remove directory entry of target file in its parent directory

    return 0;
}

static int tfs_truncate(const char * path, off_t size) {
    // For this project, you don't need to fill this function
    // But DO NOT DELETE IT!
    return 0;
}

static int tfs_release(const char * path, struct fuse_file_info * fi) {
    // For this project, you don't need to fill this function
    // But DO NOT DELETE IT!
    return 0;
}

static int tfs_flush(const char * path, struct fuse_file_info * fi) {
    // For this project, you don't need to fill this function
    // But DO NOT DELETE IT!
    return 0;
}

static int tfs_utimens(const char * path,
    const struct timespec tv[2]) {
    // For this project, you don't need to fill this function
    // But DO NOT DELETE IT!
    return 0;
}

static struct fuse_operations tfs_ope = {
    .init = tfs_init,
    .destroy = tfs_destroy,

    .getattr = tfs_getattr,
    .readdir = tfs_readdir,
    .opendir = tfs_opendir,
    .releasedir = tfs_releasedir,
    .mkdir = tfs_mkdir,
    .rmdir = tfs_rmdir,

    .create = tfs_create,
    .open = tfs_open,
    .read = tfs_read,
    .write = tfs_write,
    .unlink = tfs_unlink,

    .truncate = tfs_truncate,
    .flush = tfs_flush,
    .utimens = tfs_utimens,
    .release = tfs_release
};

int main(int argc, char * argv[]) {
    int fuse_stat;
    printf("This is how you start\n");
    getcwd(diskfile_path, PATH_MAX);
    strcat(diskfile_path, "/DISKFILE");

    fuse_stat = fuse_main(argc, argv, & tfs_ope, NULL);
    return fuse_stat;
}