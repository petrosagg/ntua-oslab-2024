#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    int status;
    pid_t p;

    if (argc != 4) {
        printf("Usage: %s <input_file> <output_file> <char>\n", argv[0]);
        return EXIT_FAILURE;
    }
    char *child_argv[] = {"a1.1-C", argv[1], argv[2], argv[3], NULL};
    char *path = "./a1.1-C";

    p = fork();
    if (p < 0){
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (p == 0){
        ssize_t val = execv(path, child_argv);
        if (val == -1) {
            perror("execv");
            exit(EXIT_FAILURE);
        }
    } else{
        wait(&status);
    }

    return 0;
}

