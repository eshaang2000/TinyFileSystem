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
    superBlock = memcpy(superBlock, buffer, sizeof(struct superblock));
    a = bio_read(superBlock -> i_bitmap_blk, buffer); //this the problem
    inode_bitmap = memcpy(inode_bitmap, buffer, MAX_INUM);
    printf("The superblock magic number is %d\n", superBlock -> magic_num);
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
    printf("The next available inode is %d\n", i);
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
    printf("The next available data block is %d\n", i);
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
    superBlock = memcpy(superBlock, buffer, sizeof(struct superblock));
    printf("The superblock magic num is %d\n", superBlock -> magic_num);
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
        if(dirent->valid == 0){
            continue;
        }
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

//this will only take a directory
//if fname is not inside the dir then it will return -1
int dir_find(uint16_t ino,
    const char * fname, size_t name_len, struct dirent * dirent) { //takes ino number and reads the inode, then searches through all the dirent files 

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

    void * buffer = malloc(BLOCK_SIZE);

    int i, j; // a is for checking if readi did its job
    j = 0; //this variable checks if it is found or not

    //We now get the superblock
    a = bio_read(0, buffer); // bio read the superblock
    if (a < 0) {
        perror("Problem reading into the supernode");
        return -1;
    }
    superBlock -> magic_num = 77; //this is to check for corruption
    superBlock = memcpy(superBlock, buffer, sizeof(struct superblock));
    printf("The superblock magic number is %d\n", superBlock -> magic_num);

    //we get the superblock too

    // Step 2: Get data block of current directory from inode - how many data blocks is that going to be

    for (i = 0; i < 16; i++) {
        if (i_node -> direct_ptr[i] == superBlock -> max_dnum) { //If max_dnum it is not allocated
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

//when allocating a new dir block you need to call this to format the data block 
int init_blk(void * buffer) { //this is a data block
    struct dirent * ptr = malloc(sizeof(struct dirent));
    ptr -> ino = MAX_INUM;
    ptr -> valid = 0;
    ptr -> len = 0;
    int i;
    for (i = 0; i < BLOCK_SIZE / sizeof(struct dirent); i++) {
        memcpy(buffer + sizeof(struct dirent) * i, (void * ) ptr, sizeof(struct dirent));
    }
    free(ptr);
    return 0;
}

//gives the new dirent entry f_ino and fname
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
            dir_inode.direct_ptr[i] = get_avail_blkno(); //this will also set the block number
            void * buffer = malloc(BLOCK_SIZE);
            init_blk(buffer);
            memcpy(buffer, dir, sizeof(struct dirent));
            bio_write(superBlock -> d_start_blk + dir_inode.direct_ptr[i], buffer);
            break;
        } 
        else { //there is something that already exists and we just go through that and see if there are any invalid bits, if yes break, if no repeat
            printf("The add function found this data block %d\n", dir_inode.direct_ptr[i]);
            void * buffer = malloc(BLOCK_SIZE);
            a = bio_read(superBlock -> d_start_blk + dir_inode.direct_ptr[i], buffer);
            int j;
            struct dirent * temp = malloc(sizeof(struct dirent));
            for (j = 0; j < BLOCK_SIZE / sizeof(struct dirent); j++) {
                memcpy(temp, buffer + sizeof(struct dirent) * j, sizeof(struct dirent));
                if (temp -> valid == 0) { //it is not in use
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
            dirent -> valid = 0;
            memcpy(buffer + sizeof(struct dirent)*i, dirent, sizeof(struct dirent));
            free(dirent);
            return 0;
        }
    }
    free(dirent);
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
            continue;
        }
        bio_read(superBlock -> d_start_blk + dir_inode.direct_ptr[i], buffer);
        a = dir_remove_help(buffer, fname);
        if (a == 0) {
            //It was found
            bio_write(superBlock -> d_start_blk + dir_inode.direct_ptr[i], buffer);
        }
    }
    free(buffer);
    printf("Dir remove is done\n");
    return 0;
}

/* 
 * namei operation
 */
int get_node_by_path(const char * path, uint16_t ino, struct inode * inode) {

    // Step 1: Resolve the path name, walk through path, and finally, find its inode.
    // Note: You could either implement it in a iterative way or recursive way
    printf("Getting into the get_node_by_path\n");
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
        printf("The token is %s\n", token);
        printf("The token is %d\n", inode->ino);
        
        int b = dir_find(inode -> ino, token, strlen(token), de);
        if (b == -1) {
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
    rootDir->vstat.st_nlink += 1;
    rootDir->vstat.st_atime = time(NULL);
    rootDir->vstat.st_mtime = time(NULL);

    int i;
    for (i = 0; i < sizeof(rootDir -> direct_ptr) / sizeof(rootDir -> direct_ptr[0]); i++) {
        rootDir -> direct_ptr[i] = superBlock -> max_dnum;
    }
    for (i = 0; i < sizeof(rootDir -> indirect_ptr) / sizeof(rootDir -> indirect_ptr[0]); i++) {
        rootDir -> indirect_ptr[i] = superBlock -> max_dnum;
    }
    writei(rootDir -> ino, rootDir);
    free(rootDir);
    inode_mem = malloc(sizeof(struct inode));
    if (inode_mem == NULL) {
        printf("inode mem memory alloc failes");
    }
    get_node_by_path("/", 0, inode_mem);
    printf("inode value %d\n", inode_mem->link);
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
    printf("Getting into the getattr funtion");
    int a = get_node_by_path(path, 0, inode_mem);//Just use the root dir inode to start
    if(a == -1){
        return -1;
    }
    // Step 2: fill attribute of file into stbuf from inode
    stbuf -> st_ino = inode_mem->ino;
    stbuf -> st_mode = S_IFDIR | 0755;
    stbuf -> st_nlink = 2;
    time( & stbuf -> st_mtime);

    return 0;
}

static int tfs_opendir(const char * path, struct fuse_file_info * fi) {

        // Step 1: Call get_node_by_path() to get inode from path
        int result = get_node_by_path(path, 0, inode_mem);//Set a global variable to store root directory inode num (second param)
        // Step 2: If not find, return -1
        if(result == -1){
            return -1;
        }
        return 0;
}

static int tfs_readdir(const char * path, void * buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info * fi) {

    printf("Getting into the read_dir\n");
    void * buf = malloc(BLOCK_SIZE);
    struct dirent* de = malloc(sizeof(struct dirent));
    // Step 1: Call get_node_by_path() to get inode from path
    int result = get_node_by_path(path, 0, inode_mem);
    if(result == -1){
        printf("invalid path");
        return -1;
    }
    // Step 2: Read directory entries from its data blocks, and copy them to filler
    int i;
    int j;
    for(i = 0; i < 16; i++){
        if(inode_mem->direct_ptr[i] != MAX_DNUM){
            bio_read(inode_mem->direct_ptr[i], buf);
            for(j=0; j<BLOCK_SIZE/sizeof(struct dirent); j++){
                memcpy(de, buf+j*sizeof(struct dirent), sizeof(struct dirent));
                if(de->valid == 0){
                    continue;
                }
                printf("Calling filler with %s\n", de->name);
                if (filler(buffer, de->name, NULL, 0) != 0)
                    return -1;
            }
        }
    }
    return 0;
}

static int tfs_mkdir(const char * path, mode_t mode) {

    // Step 1: Use dirname() and basename() to separate parent directory path and target directory name

    char * tempD = strdup(path);
    char * tempB = strdup(path);
    char * dir_name = dirname(tempD);
    char * base_name = basename(tempB);
    printf("The dirname is %s\n", dir_name);
    printf("The basename is %s\n", base_name);
    struct inode *inode = malloc(sizeof(struct inode));

  // Step 2: Call get_node_by_path() to get inode of parent directory

    int valid = get_node_by_path(dir_name, 0, inode);//if negative invalid
    if(valid == -1){
        printf("invalid path");
        return -1;
    }

  // Step 3: Call get_avail_ino() to get an available inode number

  int next_ino = get_avail_ino();

  // Step 4: Call dir_add to add directory entry of target directory to parent directory

  int add_test = dir_add(*inode, next_ino, base_name, strlen(base_name));
  inode->link += 1;
  inode->vstat.st_nlink += 1;
  inode->vstat.st_atime = time(NULL);
  inode->vstat.st_mtime = time(NULL);
  writei(inode->ino, inode);

  if(add_test == -1){
    printf("Directory already exists.\n");
    return -1;
  }

  // Step 5: Update inode for target directory
  // Not sure what needs to be updated

//   readi(next_ino, inode);

  inode->valid = 1;
  inode->ino = next_ino;
  inode->type = 1; //directory
  inode->link = 2;
  inode->vstat.st_nlink = 2;
  inode->vstat.st_atime = time(NULL);
  inode->vstat.st_mtime = time(NULL);

  //May have to update st_atime and st_mtime in vstat of inode
  // Step 6: Call writei() to write inode to disk

  writei(next_ino, inode);


  return 0;
}

static int tfs_rmdir(const char * path) {

    // Step 1: Use dirname() and basename() to separate parent directory path and target directory name
    char * tempD = strdup(path);
    char * tempB = strdup(path);
    char * dir_name = dirname(tempD);
    char * base_name = basename(tempB);
    printf("The dirname is %s\n", dir_name);
    printf("The basename is %s\n", base_name);
    // Step 2: Call get_node_by_path() to get inode of target directory

    struct inode *target_dir = malloc(sizeof(struct inode));

  if(get_node_by_path(path, 0, target_dir) < 0){//Second parameter is the inode of root directory, make it a global var
    printf("Invalid path\n");
    return -1;
  }

  // Step 3: Clear data block bitmap of target directory

  int i;
  void* buffer = malloc(BLOCK_SIZE);
  bio_read(superBlock->d_bitmap_blk, buffer);
  data_bitmap = memcpy(data_bitmap, buffer, MAX_DNUM);
  for(i = 0; i < 16; i++){
      if(target_dir->direct_ptr[i] == MAX_DNUM){
          continue;
      }
    unset_bitmap(data_bitmap, target_dir->direct_ptr[i]);
  }
  data_bitmap = memcpy(buffer, data_bitmap, sizeof(MAX_DNUM));
  bio_write(superBlock->d_bitmap_blk, buffer);

  // Step 4: Clear inode bitmap and its data block

  bio_read(superBlock->i_bitmap_blk, buffer);
  inode_bitmap = memcpy(inode_bitmap, buffer, MAX_DNUM);
  unset_bitmap(inode_bitmap, target_dir->ino);
  for(i = 0; i < 16; i++){
    target_dir->direct_ptr[i] = MAX_DNUM;
  }
  inode_bitmap = memcpy(buffer, inode_bitmap, sizeof(MAX_INUM));
  bio_write(superBlock->i_bitmap_blk, buffer);

  // Step 5: Call get_node_by_path() to get inode of parent directory

  get_node_by_path(dir_name, 0, target_dir);

  // Step 6: Call dir_remove() to remove directory entry of target directory in its parent directory

  dir_remove(*target_dir, base_name, strlen(base_name));

  return 0;
    
}

static int tfs_releasedir(const char * path, struct fuse_file_info * fi) {
    // For this project, you don't need to fill this function
    // But DO NOT DELETE IT!
    return 0;
}

static int tfs_create(const char * path, mode_t mode, struct fuse_file_info * fi) {

    printf("Getting into the create method\n");
    // Step 1: Use dirname() and basename() to separate parent directory path and target file name
    int a;
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
    a = dir_add(*inode_mem, nextAvail, bname, strlen(bname));
    if(a == -1){
        printf("Problem in create");
        return -1;
    }

    // Step 5: Update inode for target file
    struct inode* inode = malloc(sizeof(struct inode));
    inode->ino = nextAvail;
    inode->valid = 1;
    inode->size = 0;
    inode->type = 2;
    inode->link = 1;
    int i;
    for (i = 0; i < sizeof(inode -> direct_ptr) / sizeof(inode -> direct_ptr[0]); i++) {
        inode -> direct_ptr[i] = superBlock -> max_dnum;
    }
    for (i = 0; i < sizeof(inode -> indirect_ptr) / sizeof(inode -> indirect_ptr[0]); i++) {
        inode -> indirect_ptr[i] = superBlock -> max_dnum;
    }
    // Step 6: Call writei() to write inode to disk
    writei(nextAvail, inode);

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