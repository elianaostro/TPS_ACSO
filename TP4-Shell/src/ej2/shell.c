#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>
#include <errno.h>

#define MAX_LINE 4096
#define MAX_ARGS 64
#define MAX_CMDS 256

void imprimir_error(const char *msg) {
    fprintf(stderr, "Error: %s\n", msg);
    fflush(stderr);
}

int comillas_sin_cerrar(const char *linea) {
    int dobles = 0, simples = 0;
    for (int i = 0; linea[i]; i++) {
        if (linea[i] == '"') dobles++;
        else if (linea[i] == '\'') simples++;
    }
    return (dobles % 2 != 0) || (simples % 2 != 0);
}

int parsear_argumentos(char *comando, char **args) {
    int count = 0;
    char *ptr = comando;
    char *write_ptr;

    while (*ptr && count < MAX_ARGS) {
        // Saltar espacios
        while (isspace((unsigned char)*ptr)) {
            *ptr = '\0';
            ptr++;
        }
        if (!*ptr) break;

        if (*ptr == '"' || *ptr == '\'') {
            char quote = *ptr;
            args[count++] = ptr;  // Guardar el inicio del token
            
            write_ptr = ptr;      // Puntero para escribir
            ptr++;                // Avanzar después de la comilla
            
            // Copiar el contenido entre comillas
            while (*ptr && *ptr != quote) {
                *write_ptr++ = *ptr++;
            }
            *write_ptr = '\0';    // Terminar el token
            
            if (*ptr != quote) {
                fprintf(stderr, "Error: comillas sin cerrar\n");
                fflush(stderr);
                return -1;
            }
            *ptr = '\0';          // Eliminar la comilla de cierre
            ptr++;                // Avanzar después de la comilla
        } else {
            args[count++] = ptr;  // Guardar el inicio del token
            while (*ptr && !isspace((unsigned char)*ptr) && *ptr != '"' && *ptr != '\'') {
                ptr++;
            }
            if (*ptr) {
                *ptr = '\0';
                ptr++;
            }
        }
    }

    // Verificar si hay más argumentos después del límite
    char *check_excess = ptr;
    while (*check_excess && isspace((unsigned char)*check_excess)) {
        *check_excess = '\0';
        check_excess++;
    }

    if (count >= MAX_ARGS && *check_excess) {
        fprintf(stderr, "Error: demasiados argumentos\n");
        fflush(stderr);
        return -2;
    }

    args[count] = NULL;
    return count;
}

int parsear_comando_seguro(char *linea, char **comandos) {
    int count = 0;
    int in_quote = 0;
    char quote_char = 0;
    char *start = linea;

    for (char *p = linea; *p; p++) {
        if (!in_quote && (*p == '"' || *p == '\'')) {
            in_quote = 1;
            quote_char = *p;
        } else if (in_quote && *p == quote_char) {
            in_quote = 0;
        } else if (!in_quote && *p == '|') {
            *p = '\0';
            while (isspace(*start)) start++;
            if (*start == '\0') return -1;
            comandos[count++] = start;
            start = p + 1;
        }
    }
    while (isspace(*start)) start++;
    if (*start == '\0') return -1;
    comandos[count++] = start;
    comandos[count] = NULL;
    return count;
}

void ejecutar_pipeline(char **comandos, int num_comandos) {
    int prev_fd = -1;
    pid_t pids[MAX_CMDS];

    for (int i = 0; i < num_comandos; i++) {
        int pipe_fd[2];
        if (i < num_comandos - 1 && pipe(pipe_fd) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }

        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if (pid == 0) {
            if (prev_fd != -1) {
                dup2(prev_fd, STDIN_FILENO);
                close(prev_fd);
            }
            if (i < num_comandos - 1) {
                close(pipe_fd[0]);
                dup2(pipe_fd[1], STDOUT_FILENO);
                close(pipe_fd[1]);
            }

            char *args[MAX_ARGS];
            int r = parsear_argumentos(comandos[i], args);
            if (r == -1) {
                imprimir_error("comillas sin cerrar");
                exit(EXIT_FAILURE);
            } else if (r == -2) {
                // Ya se imprimió el error
                exit(EXIT_FAILURE);
            }
            execvp(args[0], args);
            fprintf(stderr, "execvp: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        } else {
            pids[i] = pid;
            if (prev_fd != -1) close(prev_fd);
            if (i < num_comandos - 1) {
                close(pipe_fd[1]);
                prev_fd = pipe_fd[0];
            }
        }
    }
    for (int i = 0; i < num_comandos; i++) {
        waitpid(pids[i], NULL, 0);
    }
}

int main() {
    char linea[MAX_LINE];
    int modo_interactivo = isatty(STDIN_FILENO);

    while (1) {
        if (modo_interactivo) {
            printf("$ ");
            fflush(stdout);
        }

        if (fgets(linea, MAX_LINE, stdin) == NULL) {
            break;
        }

        size_t len = strlen(linea);
        if (len > 0 && linea[len - 1] == '\n') linea[len - 1] = '\0';

        if (comillas_sin_cerrar(linea)) {
            imprimir_error("comillas sin cerrar");
            continue;
        }

        char *comandos[MAX_CMDS];
        char copia_linea[MAX_LINE];
        strncpy(copia_linea, linea, MAX_LINE);

        int num_comandos = parsear_comando_seguro(copia_linea, comandos);
        if (num_comandos == -1 || comandos[0] == NULL || comandos[num_comandos - 1] == NULL) {
            imprimir_error("sintaxis de pipes inválida");
            continue;
        }

        if (strcmp(comandos[0], "exit") == 0 && num_comandos == 1) {
            break;
        }

        ejecutar_pipeline(comandos, num_comandos);
    }
    return 0;
}
