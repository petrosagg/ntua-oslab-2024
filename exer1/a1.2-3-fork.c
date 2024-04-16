#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>



int main(int argc, char* argv[]){
    int fd;
    int status;
    int count = 0;
    int pfd1[2];
    int pfd2[2];
    char buff[1024];
    
    if (argc < 4) {//argument number check
        fprintf(stderr, "Insufficient command-line arguments\n");
        exit(1);
    }
    if (strlen(argv[3]) != 1) {//length check
        fprintf(stderr, "Expected single character but '%s' was provided\n", argv[3]);
        exit(1);
    }
    char sc = argv[3][0];
    
    if (pipe(pfd1) < 0) {
        perror("pipe");
        exit(1);
    }  
    if (pipe(pfd2) < 0) {
        perror("pipe");
        exit(1);
    }  
    //open file for reading
    fd = open(argv[1], O_RDONLY);
    if (fd == -1){
        perror("open");
        exit(1);
    }
    //forking
    pid_t p = fork();
    if(p < 0){
        perror("fork");
        return EXIT_FAILURE;
    }
    //PARENT PROCESS
    if(p!=0){
        close(pfd1[0]);
        while(1){
            ssize_t rcnt = read(fd, buff, sizeof(buff));

            if (rcnt == -1){
                perror("read");
                return 1;
            }
            if (rcnt == 0){
                break;
            }

            int written = 0;
            while (written < rcnt) {
                ssize_t wcnt = write(pfd1[1], buff + written, rcnt - written);
                if (wcnt == -1){ 
                    perror("write");
                    return 1;
                }
               written +=wcnt;
            }
        }
        close(pfd1[1]);
        close(fd);

        //wait for child to execute the searching process
        wait(&status);

        int value;
        close(pfd2[1]);
        if(read(pfd2[0],&value,sizeof(value)) < 0){
            perror("read");
            return EXIT_FAILURE;
        }
        //open file for writing the result
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

    //CHILD PROCESS
    if(p == 0){
        close(pfd1[1]);
        close(pfd2[0]);
        //read from the pipe and count the character appearances
        while(1){
            ssize_t rcnt2= read(pfd1[0],buff,sizeof(buff));
            if(rcnt2 < 0){
                perror("read");
                return EXIT_FAILURE;
            }
            if(rcnt2 == 0){
                break;
            }
            for(int j=0; j<rcnt2; j++){
                if(buff[j] == sc){
                    count++;
                }
            }
        }
        if(write(pfd2[1],&count,sizeof(count)) < 0){
            perror("write");
            return EXIT_FAILURE;
        }
        return EXIT_FAILURE;
    }
}

