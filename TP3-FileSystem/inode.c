#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "inode.h"
#include "diskimg.h"
#define ENTRIES_PER_BLOCK (DISKIMG_SECTOR_SIZE / sizeof(uint16_t))
#define MAX_DIRECT_BLOCKS 7
#define MAX_DIRECT_ENTRIES (MAX_DIRECT_BLOCKS * ENTRIES_PER_BLOCK)

int inode_iget(struct unixfilesystem *fs, int inumber, struct inode *inp) {

    if (inumber < 1 || inumber >= (fs->superblock.s_isize * 16)) return -1;
    
    int block = INODE_START_SECTOR + (inumber - 1) / (DISKIMG_SECTOR_SIZE / sizeof(struct inode));
    int offset = (inumber - 1) % (DISKIMG_SECTOR_SIZE / sizeof(struct inode));
    
    struct inode inodes[DISKIMG_SECTOR_SIZE / sizeof(struct inode)];
    if (diskimg_readsector(fs->dfd, block, inodes) != DISKIMG_SECTOR_SIZE) return -1;
    
    *inp = inodes[offset];
    return 0;
}

int inode_indexlookup(struct unixfilesystem *fs, struct inode *inp, int blockNum) {
    if (inp->i_mode & ILARG) {

        if (blockNum < (int)MAX_DIRECT_ENTRIES) {
            int which_indirect = blockNum / (DISKIMG_SECTOR_SIZE / sizeof(uint16_t));
            int offset_in_indirect = blockNum % (DISKIMG_SECTOR_SIZE / sizeof(uint16_t));
            
            uint16_t indirect_block[DISKIMG_SECTOR_SIZE / sizeof(uint16_t)];
            if (diskimg_readsector(fs->dfd, inp->i_addr[which_indirect], indirect_block) != DISKIMG_SECTOR_SIZE) return -1;
            return indirect_block[offset_in_indirect];

        } else {
            int double_block_num = blockNum - 7 * (DISKIMG_SECTOR_SIZE / sizeof(uint16_t));
            int index1 = double_block_num / (DISKIMG_SECTOR_SIZE / sizeof(uint16_t));
            int index2 = double_block_num % (DISKIMG_SECTOR_SIZE / sizeof(uint16_t));
            
            uint16_t double_indirect[DISKIMG_SECTOR_SIZE / sizeof(uint16_t)];
            if (diskimg_readsector(fs->dfd, inp->i_addr[7], double_indirect) != DISKIMG_SECTOR_SIZE) return -1;
            
            uint16_t indirect_block[DISKIMG_SECTOR_SIZE / sizeof(uint16_t)];
            if (diskimg_readsector(fs->dfd, double_indirect[index1], indirect_block) != DISKIMG_SECTOR_SIZE) return -1;
            
            return indirect_block[index2];
        }
    } else {
        if (blockNum < 8) return inp->i_addr[blockNum];
        else return -1;
    }
}

int inode_getsize(struct inode *inp) {
  return ((inp->i_size0 << 16) | inp->i_size1); 
}
