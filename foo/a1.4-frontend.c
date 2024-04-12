#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "util.h"

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
    fprintf(stderr, "  frontend: scanning for character '%c'\n", needle);

    int handles[2];
    char *child_argv[] = {"a1.4-dispatcher", argv[1], argv[2], NULL};
    pid_t dispatcher_pid = spawn("a1.4-dispatcher", child_argv, handles);
    int dispatcher_rx = handles[0];
    int dispatcher_tx = handles[1];

    send_cmd_add_workers(dispatcher_tx, 4);
    sleep(1);

    size_t num_workers = 0;
    struct rsp rsp;
    while (1) {
        send_cmd_progress(dispatcher_tx);
        read_exact(dispatcher_rx, &rsp, sizeof(struct rsp));

        print_response("  frontend: received response: ", &rsp);
        send_cmd_info(dispatcher_tx);
        read_exact(dispatcher_rx, &rsp, sizeof(struct rsp));
        num_workers = rsp.info_header.num_workers;
        fprintf(stderr, "  frontend: dispatcher, workers = %ld, pid = %d\n", num_workers, dispatcher_pid);
        for (size_t i = 0; i < num_workers; i++) {
            read_exact(dispatcher_rx, &rsp, sizeof(struct rsp));
            fprintf(stderr, "  frontend: worker pid = %d\n", rsp.info.pid);
        }
        sleep(3);
    }
}
