#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    int fd[2];
    int gate3[2], gate4[2];
    int result[4];
    pid_t p1, p2, p3, p4;
    char signal;

    pipe(fd);
    pipe(gate3);
    pipe(gate4);

    // Child 1
    p1 = fork();

    if (p1 == 0) {
        close(fd[0]);
        close(gate3[0]);
        close(gate3[1]);
        close(gate4[0]);
        close(gate4[1]);

        result[0] = 10 + 5;

        printf("Child 1 calculated: %d\n", result[0]);
        fflush(stdout);

        write(fd[1], &result[0], sizeof(int));

        close(fd[1]);
        exit(0);
    }

    waitpid(p1, NULL, 0);

    // Child 2
    p2 = fork();

    if (p2 == 0) {
        close(fd[0]);
        close(gate3[0]);
        close(gate3[1]);
        close(gate4[0]);
        close(gate4[1]);

        result[1] = 10 - 5;

        printf("Child 2 calculated: %d\n", result[1]);
        fflush(stdout);

        write(fd[1], &result[1], sizeof(int));

        close(fd[1]);
        exit(0);
    }

    waitpid(p2, NULL, 0);

    // Parent reads first two results
    read(fd[0], &result[0], sizeof(int));
    read(fd[0], &result[1], sizeof(int));

    // Child 3
    p3 = fork();

    if (p3 == 0) {
        close(fd[0]);
        close(gate3[1]);
        close(gate4[0]);
        close(gate4[1]);

        result[2] = 10 * 5;

        write(fd[1], &result[2], sizeof(int));
        close(fd[1]);

        read(gate3[0], &signal, 1);
        close(gate3[0]);

        printf("Child 3 calculated: %d\n", result[2]);
        fflush(stdout);

        exit(0);
    }

    // Parent waits until Child 3 sends its result
    read(fd[0], &result[2], sizeof(int));

    // Child 4
    p4 = fork();

    if (p4 == 0) {
        close(fd[0]);
        close(gate4[1]);
        close(gate3[0]);
        close(gate3[1]);

        result[3] = 10 / 5;

        write(fd[1], &result[3], sizeof(int));
        close(fd[1]);

        read(gate4[0], &signal, 1);
        close(gate4[0]);

        printf("Child 4 calculated: %d\n", result[3]);
        fflush(stdout);

        exit(0);
    }

    // Parent reads Child 4 result
    read(fd[0], &result[3], sizeof(int));

    close(fd[0]);
    close(fd[1]);

    // Parent displays all results
    for (int i = 0; i < 4; i++) {
        printf("Parent received: %d\n", result[i]);
    }

    // Allow Child 3 to print
    close(gate3[0]);
    write(gate3[1], "1", 1);
    close(gate3[1]);

    waitpid(p3, NULL, 0);

    // Allow Child 4 to print
    close(gate4[0]);
    write(gate4[1], "1", 1);
    close(gate4[1]);

    waitpid(p4, NULL, 0);

    return 0;
}