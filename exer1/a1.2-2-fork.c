#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main() { 
    int status;
    pid_t mypid, p;
    mypid = getpid();
    p = fork();
    int x;
    if (p < 0){
        perror("fork");
        exit(1);
    }

    if(p == 0){
        x = 5;
        printf("Hello world my process ID is %d and my parent's is %d\n", getpid(), mypid);
    } else {
        x = 10;
        wait(&status);
        printf("Hello world my pid as a parent is %d\n", mypid);
    }
    printf("%d\n", x);
    return 0;
}

