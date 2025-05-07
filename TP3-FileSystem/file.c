#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "file.h"
#include "inode.h"
#include "diskimg.h"

int file_getblock(struct unixfilesystem *fs, int inumber, int blockNum, void *buf) {
    struct inode in;

    if (inode_iget(fs, inumber, &in) < 0) return -1;
    if (!(in.i_mode & IALLOC)) return -1;
    
    int fileSize = inode_getsize(&in);
    int numBlocks = (fileSize + DISKIMG_SECTOR_SIZE - 1) / DISKIMG_SECTOR_SIZE;
    
    if (blockNum < 0 || blockNum >= numBlocks) return -1;

    int physicalBlock = inode_indexlookup(fs, &in, blockNum);
    
    if (physicalBlock < 0) return -1;
    if (diskimg_readsector(fs->dfd, physicalBlock, buf) < 0) return -1;
    
    int remainingBytes = fileSize - (blockNum * DISKIMG_SECTOR_SIZE);
    
    if (remainingBytes >= DISKIMG_SECTOR_SIZE) return DISKIMG_SECTOR_SIZE;
    else return remainingBytes;
}