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
int currentInode = 0;
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
    printf("The superblock magic num %d\n", superBlock -> magic_num);
    printf("The d bitmap location block no is %d\n", superBlock -> d_bitmap_blk);
    superBlock = memcpy(superBlock, buffer, sizeof(struct superblock));
    a = bio_read(superBlock -> d_bitmap_blk, buffer); //this the problem
    data_bitmap = memcpy(data_bitmap, buffer, MAX_INUM);
    printf("The superblock magic num %d\n", superBlock -> magic_num);
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
    printf("Getting into readi\n");

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
    int a; // to check if bio_read operations happened or not
    void * buffer = malloc(BLOCK_SIZE);
    a = bio_read(0, buffer);
    if (a < 0) {
        perror("Problem reading into the supernode");
        return -1;
    }
    superBlock -> magic_num = 77;
    superBlock = memcpy(superBlock, buffer, sizeof(struct superblock));
    a = bio_read(superBlock -> i_start_blk + blockNo, buffer);
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
    printf("Getting into writei\n");

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
    int a; // to check if bio_read operations happened or not
    a = bio_read(0, buffer);
    if (a < 0) {
        perror("Problem reading into the supernode");
        return -1;
    }
    superBlock -> magic_num = 77;
    printf("The superblock is 1 %d\n", superBlock -> magic_num);
    superBlock = memcpy(superBlock, buffer, sizeof(struct superblock));
    printf("The superblock is 2 %d\n", superBlock -> magic_num);
    int o = sizeof(struct inode) * offset; // this tells me where to memcpy from
    void * tempBuf = NULL;
    tempBuf = memcpy(buffer + o, inode, sizeof(struct inode));
    a = bio_write(superBlock -> i_start_blk + blockNo, buffer);
    if (a < 0) {
        perror("Read not succesful");
        return -1;
    }
    tempBuf = NULL;
    free(buffer);
    return 0;
}

/* 
1. Given a buffer - goes through it and finds if the name matches. Helper function for dir_find
 */
int dir_find_help(void * buffer, struct dirent * dirent,
    const char * fname) {
    printf("Entering dir_find_help\n");
    int n = BLOCK_SIZE / sizeof(struct dirent); //the number of dirent structs in the bloc
    int i;
    for (i = 0; i < n; i++) { //we iterate through all of them
        dirent = memcpy(dirent, buffer + i * sizeof(struct dirent), sizeof(struct dirent));
        printf("The name of the dirents %s\n", dirent -> name);
        if (strcmp(fname, dirent -> name) == 0) {
            //Found a match, already in *dirent
            return 0;
        }
    }
    return -1;
}

/* 
 * directory operations
 */
int dir_find(uint16_t ino,
    const char * fname, size_t name_len, struct dirent * dirent) {

    printf("Getting into the dir_find function\n");
    // Step 1: Call readi() to get the inode using ino (inode number of current directory)
    struct inode * i_node;
    i_node = malloc(sizeof(struct inode));
    int a;
    a = readi(ino, i_node);
    if (a == -1) {
        printf("Readi problem");
        return -1;
    }
    //i_node contains the inode of the current directory

    void * buffer;
    buffer = malloc(BLOCK_SIZE);

    int i, j; // a is for checking if readi did its job
    j = 0; //this variable checks if it is found or not

    //We now get the superblock
    a = bio_read(0, buffer); // bio read the superblock
    if (a < 0) {
        perror("Problem reading into the supernode");
        return -1;
    }
    superBlock -> magic_num = 77; //this is to check for corruption
    printf("The superblock is magic number 1 %d\n", superBlock -> magic_num);
    superBlock = memcpy(superBlock, buffer, sizeof(struct superblock));
    printf("The superblock magic number 2 %d\n", superBlock -> magic_num);

    //we get the superblock too

    // Step 2: Get data block of current directory from inode - how many data blocks is that going to be

    for (i = 0; i < 16; i++) {
        if (i_node -> direct_ptr[i] == superBlock -> max_dnum) { //If 0 it is not allocated
            continue;
        } //if there is nothing in the direct_ptr then nah
        a = bio_read(superBlock -> d_start_blk + i_node -> direct_ptr[i], buffer); //So this contains the datablock and we put it in the buffer
        if (a < 0) {
            perror("Oh no");
            return -1;
        }
        //let's just say we have the buffer
        a = dir_find_help(buffer, dirent, fname);
        if (a == 0) { //name dir found
            j = 1; //flag variable 
            break;
        } // this will also copy 
    } //for closed

    // Step 3: Read directory's data block and check each directory entry.
    //If the name matches, then copy directory entry to dirent structure
    free(buffer);
    if (j != 1) {
        return -1; // not found
    }
    return 0;
} //method closed

int init_blk(void * buffer) { //this is a data block
    struct dirent * ptr = malloc(sizeof(struct dirent));
    ptr -> ino = 0;
    ptr -> valid = 0;
    ptr -> len = 0;
    int i;
    for (i = 0; i < BLOCK_SIZE / sizeof(struct dirent); i++) {
        memcpy(buffer + sizeof(struct dirent) * i, (void * ) ptr, sizeof(struct dirent));
    }
    free(ptr);
    return 0;
}

int dir_add(struct inode dir_inode, uint16_t f_ino,
    const char * fname, size_t name_len) {

    //get the superblock
    printf("Entering the dir_add method\n");
    // Step 1: Read dir_inode's data block and check each directory entry of dir_inode
    // Step 2: Check if fname (directory name) is already used in other entries

    int a; //check variable
    struct dirent * dir = malloc(sizeof(struct dirent));
    a = dir_find(dir_inode.ino, fname, name_len, dir);
    if (a == 0) {
        printf("The file or dir already exists\n");
        return -1;
    }

    // Step 3: Add directory entry in dir_inode's data block and write to disk
    dir -> ino = f_ino;
    dir -> valid = 1;
    strcpy(dir -> name, fname);
    dir -> len = name_len;

    for (int i = 0; i < 16; i++) {
        if (dir_inode.direct_ptr[i] == MAX_DNUM) { //then get a new data block make it dirent type and start setting it
            printf("The add function did not find a data block allocated\n");
            dir_inode.direct_ptr[i] = get_avail_blkno();
            void * buffer = malloc(BLOCK_SIZE);
            init_blk(buffer);
            memcpy(buffer, dir, sizeof(struct dirent));
            bio_write(superBlock -> d_start_blk + dir_inode.direct_ptr[i], buffer);
            break;
        } else { //there is something that already exists and we just go through that and see if there are any invalid bits, if yes break, if no repeat
            printf("The add function found this data block %d\n", dir_inode.direct_ptr[i]);
            void * buffer = malloc(BLOCK_SIZE);
            a = bio_read(superBlock -> d_start_blk + dir_inode.direct_ptr[i], buffer);
            int j;
            struct dirent * temp = malloc(sizeof(struct dirent));
            for (j = 0; j < BLOCK_SIZE / sizeof(struct dirent); j++) {
                memcpy(temp, buffer + sizeof(struct dirent) * j, sizeof(struct dirent));
                if (temp -> valid == 0) { //it is not in use
                    printf("j values are %d\n", j);
                    memcpy(buffer + j * sizeof(struct dirent), dir, sizeof(struct dirent));
                    bio_write(superBlock -> d_start_blk + dir_inode.direct_ptr[i], buffer);
                    return 0;
                }
            } //for closed
        }
    }
    return -1;
}

int dir_remove_help(void * buffer, const char * fname) {
        printf("Enters dir remove help\n");
    int n = BLOCK_SIZE / sizeof(struct dirent); //the number of dirent structs in the bloc
    int i;
    struct dirent* dirent = malloc(sizeof(struct dirent));
    for (i = 0; i < n; i++) { //we iterate through all of them
        dirent = memcpy(dirent, buffer + i * sizeof(struct dirent), sizeof(struct dirent));
        if (strcmp(fname, dirent -> name) == 0) {
            //Found a match, already in diren

            printf("The name is1 %s\n", fname);

            printf("The name is2 %s\n", dirent->name);
            dirent -> valid = 0;
            memcpy(buffer + sizeof(struct dirent)*i, dirent, sizeof(struct dirent));

    printf("The name is  exittttt %s\n", fname);
            return 0;
        }
    }

    printf("The name is  exittttt %s\n", fname);
    return -1;
}

int dir_remove(struct inode dir_inode,
    const char * fname, size_t name_len) {

    printf("Getting into the dir_remove fucniton\n");
    // Step 1: Read dir_inode's data block and checks each directory entry of dir_inode

    // Step 2: Check if fname exist

    // Step 3: If exist, then remove it from dir_inode's data block and write to disk

    int i, a;
    void * buffer = malloc(BLOCK_SIZE);
    // struct dirent * ptr = malloc(sizeof(struct dirent));
    for (i = 0; i < 16; i++) {
        if (dir_inode.direct_ptr[i] == MAX_DNUM) {
            //directory doesn't exist in dir_inode
            return -1;
        }
        printf("Gets here\n");
        bio_read(superBlock -> d_start_blk + dir_inode.direct_ptr[i], buffer);
        a = dir_remove_help(buffer, fname);
        if (a == 0) {
            //It was found
            bio_write(superBlock -> d_start_blk + dir_inode.direct_ptr[i], buffer);

    printf("The name is  exittttt %s\n", fname);
        }
    }
    free(buffer);
    // free(ptr);
    printf("Dir remove is done\n");
    return 0;
}

/* 
 * namei operation
 */
int get_node_by_path(const char * path, uint16_t ino, struct inode * inode) {

    // Step 1: Resolve the path name, walk through path, and finally, find its inode.
    // Note: You could either implement it in a iterative way or recursive way

    readi(ino, inode);
    if (strlen(path) == 1) {
        //It's the root directory
        return 1;
    }
    const char s[2] = "/";
    char * tempD = strdup(path);
    char * token = malloc(sizeof( * path));
    struct dirent * de = malloc(sizeof(struct dirent));
    token = strtok(tempD, s);
    printf("Reaches here\n");

    while (token) {
        if (dir_find(inode -> ino, token, strlen(token), de) == -1) {
            //invalid path
            inode = NULL;
            return -1;
        } //if ends
        printf("%s\n", token);
        token = strtok(NULL, s);
        readi(de -> ino, inode);
    }
    return 2;
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
    int nextAvail = get_avail_ino(); // 0 is the next inode in this case
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
    rootDir -> valid = 1;
    rootDir -> size = 0;
    rootDir -> type = 0; //dir type or file
    rootDir -> link = 2;
    rootDir -> no_dirents = 0; //prolly never going to use this
    int i;
    for (i = 0; i < sizeof(rootDir -> direct_ptr) / sizeof(rootDir -> direct_ptr[0]); i++) {
        rootDir -> direct_ptr[i] = superBlock -> max_dnum;
    }
    for (i = 0; i < sizeof(rootDir -> indirect_ptr) / sizeof(rootDir -> indirect_ptr[0]); i++) {
        rootDir -> indirect_ptr[i] = superBlock -> max_dnum;
    }
    writei(rootDir -> ino, rootDir);
    // free(rootDir);
    inode_mem = malloc(sizeof(struct inode));
    if (inode_mem == NULL) {
        printf("inode mem memory alloc failes");
    }

    //testing dir fucntions
    nextAvail = get_avail_blkno();
    rootDir -> direct_ptr[0] = nextAvail;
    writei(0, rootDir);
    readi(0, inode_mem);
    init_blk(buffer);
    struct dirent * dir = malloc(sizeof(struct dirent));
    dir -> ino = 1;
    dir -> valid = 1;
    // dir->name = "Eshaan";
    strcpy(dir -> name, "Eshaan");
    dir -> len = strlen(dir -> name);
    memcpy(buffer, dir, sizeof(struct dirent));
    dir -> ino = 1;
    dir -> valid = 1;
    // dir->name = "Eshaan";
    strcpy(dir -> name, "AJ");
    printf("The name test %s\n", dir -> name);
    dir -> len = strlen(dir -> name);
    memcpy(buffer + sizeof(struct dirent), dir, sizeof(struct dirent));
    bio_write(superBlock -> d_start_blk + rootDir -> direct_ptr[0], buffer);
    strcpy(dir -> name, "Mike");
    printf("The name before %s\n", dir -> name);
    dir_find(inode_mem -> ino, "AJ", strlen("AJ"), dir);
    printf("The name after %s\n", dir -> name);

    dir_add( * rootDir, get_avail_ino(), "Mike", strlen("Mike"));

    dir_add( * rootDir, get_avail_ino(), "Noah", strlen("Noah"));
    dir_find(inode_mem -> ino, "Noah", strlen("Noah"), dir);
    printf("The name after %s\n", dir -> name);
    dir_remove(*rootDir, "AJ", strlen("AJ"));
    dir_find(inode_mem -> ino, "AJ", strlen("AJ"), dir);
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
    } else {
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
    char * tempD = strdup(path);
    char * tempB = strdup(path);
    char * dname = dirname(tempD);
    char * bname = basename(tempB);
    uint16_t inodeNumber = 0;
    printf("The dirname is %s\n", dname);
    printf("The basename is %s\n", bname);
    //How do you get the inodenumber of the root of this path

    // Step 2: Call get_node_by_path() to get inode of parent directory

    get_node_by_path(path, inodeNumber, inode_mem);

    // Step 3: Call get_avail_ino() to get an available inode number

    int nextAvail = get_avail_ino();
    printf("The next available inode number is %d\n", nextAvail);

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
    if (a == -1) {
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