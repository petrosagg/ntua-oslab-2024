#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "util.h"

#define BUFFER_SIZE 1024

int main (int argc, char* argv[]){
    if (argc != 3) {
        fprintf(stderr, "Wrong number of arguments\n");
        exit(EXIT_FAILURE);
    }
    if (strlen(argv[2]) != 1) {
        fprintf(stderr, "Invalid needle. Expected one character: %s\n", argv[2]);
        exit(EXIT_FAILURE);

    }
    char needle = argv[2][0]; 

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("open");
        exit(EXIT_FAILURE);
    } 

    // Read scan commands from stdin and count the corresponding bytes in the file.
    char buf[BUFFER_SIZE];
    struct cmd_scan cmd;
    struct rsp_scan rsp;
    while (1) {
        read_exact(0, &cmd, sizeof(struct cmd_scan));
        if (lseek(fd, cmd.offset, SEEK_SET) == -1) {
            perror("lseek");
            exit(EXIT_FAILURE);
        }

        rsp.count = 0;
        uint64_t scanned = 0;
        while (scanned < cmd.size) {
            size_t size = cmd.size - scanned;
            if (size > sizeof(buf)) {
                size = sizeof(buf);
            }

            ssize_t rcnt = read(fd, buf, size);
            if (rcnt < 0) {
                perror("read");
                exit(EXIT_FAILURE);
            }

            for (ssize_t i = 0; i < rcnt; i++) {
                if (buf[i] == needle) {
                    rsp.count++;
                }
            }
            scanned += rcnt;
        }
        sleep(2);
        write_exact(1, &rsp, sizeof(struct rsp_scan));
    }
}
