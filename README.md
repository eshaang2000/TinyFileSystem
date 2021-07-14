# TinyFileSystem

Built a File System using the fuse library that can so basic FS commands like "ls", "mkdir", "touch", "cat", etc. This project was for my operating systems class in the Spring of 2021. 

## Fuse Library

FUSE is a kernel module that redirects system calls to one's file system from the OS to the code one write at the user level. The fuse library API documentation can be found here https://libfuse.github.io/doxygen/index.html

## Tiny File System Structure

1. code/block.c : Basic block operations, acts as a disk driver reading blocks from disk. Also configures disk size via DISK_SIZE
2. code/block.h : Block layer headers and configures the block size via BLOCK_SIZE
3. code/tfs.c : User-facing file system operations
4. code/tfs.h : Contains inode, superblock, and dirent structures. Also, provides functions for bitmap operations.

## Some operations that are supported

1. mkdir
2. touch
3. cat
4. cd
5. ls
6. open
7. close

## Learnings

Learning File System logic and organisation, and persistent storage paradigms. We emulated a working disk by storing all data in a flat file that you will access like a block device. All user data as well as all indexing blocks, management information, or metadata about files is stored in that single flat file.
