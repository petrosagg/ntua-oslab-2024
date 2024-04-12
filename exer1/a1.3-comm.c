#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>

int children = 0;

void signal_handler(int signum){
    printf("There are %d workers currently\n", children);
}
void do_nothing(int signum){}

int main(int argc, char *argv[]){
    char buf[10];
    int status;
    struct sigaction sa;
    sigset_t sigset;
    sa.sa_flags = SA_RESTART;
    sa.sa_mask = sigset;

    int count = 0;
    int pfd[2];


    if (argc != 4) {
        printf("Usage: %s <input_file> <char> <num of workers>\n", argv[0]);
        return EXIT_FAILURE;
    }
    children = atoi(argv[3]);
    // DONE: check for length using strlen, it should always be 1
    if (strlen(argv[2]) != 1) {
        printf("Please insert a single character to search for\n");
        return EXIT_FAILURE;
    }
    char needle = argv[2][0];

    ssize_t fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("open");
        return EXIT_FAILURE;
    } 

    if (pipe(pfd) < 0) {
        perror("pipe");
        exit(1);
    }

    for (int i = 0; i < children; i++) {
        ssize_t pid = fork();
        if (pid < 0) {
            perror("fork");
            return EXIT_FAILURE;
        } else if (pid == 0) {
            sa.sa_handler = do_nothing;
            if (sigaction(SIGINT, &sa, NULL) < 0) {
                perror("sigaction");
                exit(1);
            }
            while (1) {
                ssize_t p = read(fd, buf, sizeof(buf));
                if (p < 0) {
                    perror("read");
                    return EXIT_FAILURE;
                }
                if (p == 0) {
                    close(pfd[0]);
                    ssize_t pwr = write(pfd[1], &count, sizeof(count));
                    if (pwr != sizeof(count)) {
                        perror("write to pipe");
                    }

                    close(pfd[1]);
                    return EXIT_SUCCESS;
                }

                for (int j = 0; j < p; j++) {
                    if (buf[j] == needle) {
                        count++;
                    }
                }
                sleep(1);
            }
        }
    }

    // wait for all children
    int total = 0;
    int value;

    sa.sa_handler = signal_handler;
    if (sigaction(SIGINT, &sa, NULL) < 0) {
        perror("sigaction");
        exit(1);
    }
    for (int i = 0; i < children; i++) {
        wait(&status);
    }
    //read from pipe and add it to the total
    close(pfd[1]);
    while (1) {
        ssize_t prd = read(pfd[0], &value, sizeof(value));
        if (prd == 0) {
            break;
        }
        total = total + value;
        children--;
        sleep(1);
    }
    close(pfd[0]);

    printf("Found %d occurences in total\n", total);

    return 0;
}
