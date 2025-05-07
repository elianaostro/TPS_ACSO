#include "pathname.h"
#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>  

int pathname_lookup(struct unixfilesystem *fs, const char *pathname) {
    if (pathname[0] != '/') return -1;
    
    if (pathname[1] == '\0') return ROOT_INUMBER;
    
    int current_inumber = ROOT_INUMBER;
    
    char *path_copy = strdup(pathname);
    if (!path_copy) return -1;
    
    char *token = strtok(path_copy + 1, "/");
    
    while (token != NULL) {
        struct direntv6 dirEnt;
        
        if (directory_findname(fs, token, current_inumber, &dirEnt) < 0) {
            free(path_copy);
            return -1;
        }
        
        current_inumber = dirEnt.d_inumber;
        token = strtok(NULL, "/");
    }
    
    free(path_copy);
    return current_inumber;
}
