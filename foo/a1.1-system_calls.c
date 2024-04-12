#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdlib.h>

int main(int argc, char *argv[]){
    int fd;
    int count = 0;
    char buff[1024];

    if (argc < 4) {
        fprintf(stderr, "Insufficient command-line arguments\n");
        exit(1);
    }

    // open the file to read from
    fd = open(argv[1], O_RDONLY);
    if (fd == -1){
        perror("open");
        exit(1);
    }

    if (strlen(argv[3]) != 1) {
        fprintf(stderr, "Expected single character but '%s' was provided\n", argv[3]);
        exit(1);
    }
    char sc = argv[3][0];

    // read from the file and save to buffer
    while(1){
        ssize_t rcnt = read(fd, buff, sizeof(buff));
        if (rcnt == -1){
            perror("read");
            return 1;
        }
        if (rcnt == 0){
            break;
        }

        for (int i = 0; i < rcnt; i++) {
            if (buff[j] == sc) {
                count++;
            }
        }
    }
    close(fd);

    // now open the 2d file and write the result in an output file
    fd = open(argv[2], O_CREAT | O_TRUNC | O_WRONLY, 0644);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    int size = snprintf(buff, sizeof(buff), "Character '%c' appears %d times in file %s\n", sc, count, argv[1]);
    int written = 0;
    while (written < size) {
        ssize_t wcnt = write(fd, buff + written, size - written);
        if (wcnt == -1) { 
            perror("write");
            return 1;
        }
        written += wcnt;
    }
    close(fd);
}
