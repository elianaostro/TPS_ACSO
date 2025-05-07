#include "pathname.h"
#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>  // Añadir esta línea para free, strdup, etc.

/**
 * TODO
 */
int pathname_lookup(struct unixfilesystem *fs, const char *pathname) {
    // Verificar que el pathname comience con '/'
    if (pathname[0] != '/') {
        return -1;
    }
    
    // Caso especial: si es solo "/", retornar inode raíz
    if (pathname[1] == '\0') {
        return ROOT_INUMBER;
    }
    
    // Empezar desde la raíz
    int current_inumber = ROOT_INUMBER;
    
    // Crear una copia del pathname para tokenización
    char *path_copy = strdup(pathname);
    if (!path_copy) {
        return -1;
    }
    
    // Eliminar el primer '/'
    char *token = strtok(path_copy + 1, "/");
    
    // Procesar cada componente del path
    while (token != NULL) {
        struct direntv6 dirEnt;
        
        // Buscar el siguiente componente
        if (directory_findname(fs, token, current_inumber, &dirEnt) < 0) {
            free(path_copy);
            return -1;
        }
        
        // Actualizar el inode actual
        current_inumber = dirEnt.d_inumber;
        
        // Siguiente token
        token = strtok(NULL, "/");
    }
    
    free(path_copy);
    return current_inumber;
}
