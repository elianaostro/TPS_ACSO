#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "file.h"
#include "inode.h"
#include "diskimg.h"

/**
 * TODO
 */
int file_getblock(struct unixfilesystem *fs, int inumber, int blockNum, void *buf) {
    // Obtener el inode
    struct inode in;
    if (inode_iget(fs, inumber, &in) < 0) {
        return -1;
    }
    
    // Verificar que el inode esté asignado
    if (!(in.i_mode & IALLOC)) {
        return -1;
    }
    
    // Obtener el tamaño total del archivo
    int fileSize = inode_getsize(&in);
    
    // Calcular cuántos bloques tiene el archivo
    int numBlocks = (fileSize + DISKIMG_SECTOR_SIZE - 1) / DISKIMG_SECTOR_SIZE;
    
    // Verificar que el bloque solicitado sea válido
    if (blockNum < 0 || blockNum >= numBlocks) {
        return -1;
    }
    
    // Obtener el número de bloque físico usando inode_indexlookup
    int physicalBlock = inode_indexlookup(fs, &in, blockNum);
    if (physicalBlock < 0) {
        return -1;
    }
    
    // Leer el bloque del disco
    if (diskimg_readsector(fs->dfd, physicalBlock, buf) < 0) {
        return -1;
    }
    
    // Calcular cuántos bytes son válidos en este bloque
    int remainingBytes = fileSize - (blockNum * DISKIMG_SECTOR_SIZE);
    if (remainingBytes >= DISKIMG_SECTOR_SIZE) {
        return DISKIMG_SECTOR_SIZE;
    } else {
        return remainingBytes;
    }
}