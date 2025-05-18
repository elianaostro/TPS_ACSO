#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>


int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <n> <c> <s>\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]); // cantidad de procesos
    int c = atoi(argv[2]); // valor inicial
    int s = atoi(argv[3]); // proceso que inicia

    if (n < 3 || s < 0 || s >= n) {
        fprintf(stderr, "Error: n >= 3 y 0 <= s < n\n");
        return 1;
    }

    int pipes[n][2];
    for (int i = 0; i < n; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            exit(1);
        }
    }

    int parent_pipe[2]; // para que hijo s devuelva resultado al padre
    if (pipe(parent_pipe) == -1) {
        perror("pipe padre");
        exit(1);
    }

    for (int i = 0; i < n; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            // proceso hijo i
            int read_fd = pipes[(i + n - 1) % n][0];
            int write_fd = pipes[i][1];

            for (int j = 0; j < n; j++) {
                if (j != i)
                    close(pipes[j][1]);
                if (j != (i + n - 1) % n)
                    close(pipes[j][0]);
            }
            close(parent_pipe[0]); // hijos no leen del padre

            int val;
            if (read(read_fd, &val, sizeof(int)) == sizeof(int)) {
                val++;
                if (i == s) {
                    write(parent_pipe[1], &val, sizeof(int));
                } else {
                    write(write_fd, &val, sizeof(int));
                }
            }

            close(read_fd);
            close(write_fd);
            close(parent_pipe[1]);
            exit(0);
        }
    }

    // padre
    for (int i = 0; i < n; i++) {
        close(pipes[i][0]);
        if (i != s)
            close(pipes[i][1]);
    }
    close(parent_pipe[1]);

    write(pipes[s][1], &c, sizeof(int));
    close(pipes[s][1]);

    int result;
    if (read(parent_pipe[0], &result, sizeof(int)) == sizeof(int)) {
        printf("Resultado final: %d\n", result);
    } else {
        fprintf(stderr, "Error al leer resultado final\n");
    }

    close(parent_pipe[0]);

    for (int i = 0; i < n; i++) {
        wait(NULL);
    }

    return 0;
}