#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "diskimg.h"
#include "unixfilesystem.h"
#include "inode.h"
#include "file.h"
#include "directory.h"
#include "pathname.h"


//Prueba de robustez y manejo de errores
void test_robustness(struct unixfilesystem *fs) {
    printf("=== PRUEBAS DE ROBUSTEZ ===\n");
    
    // Prueba 1: Inodos inválidos
    printf("Prueba inodo inexistente (debería fallar):\n");
    struct inode in;
    int result = inode_iget(fs, 9999, &in);
    printf("  Resultado: %d (esperado: -1)\n", result);
    
    // Prueba 2: Bloques fuera de límite
    printf("Prueba bloque fuera de límite (debería fallar):\n");
    char buf[DISKIMG_SECTOR_SIZE];
    result = file_getblock(fs, ROOT_INUMBER, 9999, buf);
    printf("  Resultado: %d (esperado: -1)\n", result);
    
    // Prueba 3: Pathname malformado
    printf("Prueba pathname malformado (debería fallar):\n");
    result = pathname_lookup(fs, "//invalid//path//");
    printf("  Resultado: %d (esperado: -1)\n", result);
    
    // Prueba 4: Directorio inexistente
    printf("Prueba directorio inexistente (debería fallar):\n");
    result = pathname_lookup(fs, "/directorio_que_no_existe");
    printf("  Resultado: %d (esperado: -1)\n", result);
    
    // Prueba 5: Archivo dentro de un archivo (no directorio)
    int file_inumber = pathname_lookup(fs, "/bigfile");
    if (file_inumber > 0) {
        printf("Prueba archivo como directorio (debería fallar):\n");
        struct direntv6 entry;
        result = directory_findname(fs, "test", file_inumber, &entry);
        printf("  Resultado: %d (esperado: -1)\n", result);
    }
}

//Prueba de rendimiento
void test_performance(struct unixfilesystem *fs) {
    printf("=== PRUEBAS DE RENDIMIENTO ===\n");
    
    clock_t start, end;
    double cpu_time_used;
    
    // Prueba 1: Acceso a archivo grande
    printf("Tiempo de acceso a archivo grande:\n");
    start = clock();
    int inumber = pathname_lookup(fs, "/verybig");
    if (inumber > 0) {
        struct inode in;
        inode_iget(fs, inumber, &in);
        int size = inode_getsize(&in);
        int num_blocks = (size + DISKIMG_SECTOR_SIZE - 1) / DISKIMG_SECTOR_SIZE;
        
        char buf[DISKIMG_SECTOR_SIZE];
        for (int i = 0; i < num_blocks; i += 10) {  // Salta bloques para no tardar tanto
            file_getblock(fs, inumber, i, buf);
        }
    }
    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("  Tiempo: %f segundos\n", cpu_time_used);
    
    // Prueba 2: Búsqueda de pathname profundo
    printf("Tiempo de búsqueda de pathname profundo:\n");
    start = clock();
    for (int i = 0; i < 1000; i++) {
        pathname_lookup(fs, "/foo/CVS/Repository");
    }
    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("  Tiempo para 1000 búsquedas: %f segundos\n", cpu_time_used);
}

//Verificación recursiva de directorios
void recursive_directory_check(struct unixfilesystem *fs, int inumber, char *path, int depth) {
    if (depth > 10) return;  // Evitar recursión infinita
    
    struct inode in;
    if (inode_iget(fs, inumber, &in) < 0) {
        printf("ERROR: No se pudo obtener inodo %d en %s\n", inumber, path);
        return;
    }
    
    if (!(in.i_mode & IALLOC)) {
        printf("ERROR: Inodo %d no asignado en %s\n", inumber, path);
        return;
    }
    
    if ((in.i_mode & IFMT) != IFDIR) {
        // No es un directorio, terminamos el recorrido
        return;
    }
    
    // Es un directorio, procesamos sus entradas
    int size = inode_getsize(&in);
    int num_blocks = (size + DISKIMG_SECTOR_SIZE - 1) / DISKIMG_SECTOR_SIZE;
    
    for (int bno = 0; bno < num_blocks; bno++) {
        char buf[DISKIMG_SECTOR_SIZE];
        int bytesRead = file_getblock(fs, inumber, bno, buf);
        
        if (bytesRead < 0) {
            printf("ERROR: No se pudo leer bloque %d del directorio %s\n", bno, path);
            continue;
        }
        
        struct direntv6 *entries = (struct direntv6 *)buf;
        int numEntries = bytesRead / sizeof(struct direntv6);
        
        for (int i = 0; i < numEntries; i++) {
            if (entries[i].d_inumber == 0) continue;  // Entrada vacía
            
            // Ignorar "." y ".."
            if (entries[i].d_name[0] == '.' && 
                (entries[i].d_name[1] == 0 || 
                 (entries[i].d_name[1] == '.' && entries[i].d_name[2] == 0))) {
                continue;
            }
            
            char new_path[1024];
            snprintf(new_path, sizeof(new_path), "%s/%s", path, entries[i].d_name);
            
            // Verificar que el inodo existe
            struct inode entry_inode;
            if (inode_iget(fs, entries[i].d_inumber, &entry_inode) < 0) {
                printf("ERROR: Inodo %d referenciado pero no existente en %s\n", 
                       entries[i].d_inumber, new_path);
                continue;
            }
            
            // Recursivamente procesar subdirectorios
            recursive_directory_check(fs, entries[i].d_inumber, new_path, depth + 1);
        }
    }
}

void test_filesystem_consistency(struct unixfilesystem *fs) {
    printf("=== VERIFICACIÓN DE CONSISTENCIA DEL FILESYSTEM ===\n");
    recursive_directory_check(fs, ROOT_INUMBER, "", 0);
    printf("Verificación completa\n");
}

//Programa de prueba completo
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <diskimagePath>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    char *diskpath = argv[1];
    int fd = diskimg_open(diskpath, 1);
    
    if (fd < 0) {
        fprintf(stderr, "Can't open diskimagePath %s\n", diskpath);
        exit(EXIT_FAILURE);
    }
    
    struct unixfilesystem *fs = unixfilesystem_init(fd);
    if (!fs) {
        fprintf(stderr, "Failed to initialize unix filesystem\n");
        exit(EXIT_FAILURE);
    }
    
    printf("=== PRUEBAS DEL SISTEMA DE ARCHIVOS UNIX V6 ===\n");
    printf("Imagen de disco: %s\n\n", diskpath);
    
    test_robustness(fs);
    printf("\n");
    
    test_performance(fs);
    printf("\n");
    
    test_filesystem_consistency(fs);
    
    int err = diskimg_close(fd);
    if (err < 0) fprintf(stderr, "Error closing %s\n", diskpath);
    free(fs);
    
    return 0;
}