#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>

int main(int argc, char* argv[]){
    printf("hi\n");
    int fd;
    int status;
    int count = 0;
    int pfd1[2];
    int pfd2[2];
    char buff[1024];
    char buffer[1024];
    char sc = argv[3][0];
    if (argc < 4) {
        fprintf(stderr, "Insufficient command-line arguments\n");
        exit(1);
    }
    if (strlen(argv[3]) != 1) {
        fprintf(stderr, "Expected single character but '%s' was provided\n", argv[3]);
        exit(1);
    }
    if (pipe(pfd1) < 0) {
        perror("pipe");
        exit(1);
    }  
    if (pipe(pfd1) < 0) {
        perror("pipe");
        exit(1);
    }  

    fd = open(argv[1], O_RDONLY);
    if (fd == -1){
        perror("open");
        exit(1);
    }
    printf("before fork\n");
    pid_t p = fork();
    if(p<0){
        perror("fork");
        return EXIT_FAILURE;
    }
    if(p!=0){
        close(pfd1[0]);
        int bytes_read=0;
        ssize_t rcnt;
        while(1){
            rcnt = read(fd, buff, sizeof(buff)-1);
            if (rcnt == -1){
                perror("read");
                return 1;
            }
            if (rcnt == 0){
                break;
            }
            bytes_read+=rcnt;
            buff[rcnt] = '\0';

        }
        int written = 0;
        while (written<bytes_read) {
            ssize_t wcnt = write(pfd1[1], buff + written, rcnt - written);
            // TODO: error handling
            written +=wcnt;
        }

        close(fd);
        printf("parent1\n");
        wait(&status);
    } 
    //if child read the data from pipe search and write the result
    if(p==0){
        close(pfd1[1]);
        close(pfd2[0]);
        while(1){
            ssize_t rcnt2= read(pfd1[0],buffer,sizeof(buffer));
            if(rcnt2<0){
                perror("read");
                return EXIT_FAILURE;
            }
            if(rcnt2==0){
                break;
            }
            for(int j=0; j<rcnt2; j++){
                if(buffer[j]==sc){
                    count++;
                }
            }
        }
        write(pfd2[1],&count,sizeof(count));
        printf("child\n");
    }

    if(p!=0){ 
        int value;
        close(pfd2[1]);
        read(pfd2[0],&value,sizeof(value));
        fd = open(argv[2], O_CREAT | O_TRUNC | O_WRONLY, 0644);
        if (fd == -1) {
            perror("open");
            return 1;
        }

        int size = snprintf(buff, sizeof(buff), "Character '%c' appears %d times in file %s\n", sc, value, argv[1]);
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
}

