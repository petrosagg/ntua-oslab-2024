#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>

#include "util.h"

#define DELIM " \n"

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

    int handles[2];
    char *child_argv[] = {"a1.4-dispatcher", argv[1], argv[2], NULL};
    pid_t dispatcher_pid = spawn("a1.4-dispatcher", child_argv, handles);
    int dispatcher_rx = handles[0];
    int dispatcher_tx = handles[1];

    // Continously read commands from stdin
    size_t num_workers = 0;
    char cmd_buf[64];
    struct rsp rsp;
    while (1) {
        printf("> ");
        if (fgets(cmd_buf, sizeof(cmd_buf), stdin) == NULL) {
            break;
        }
        char *token = strtok(cmd_buf, DELIM);
        // Ignore empty commands
        if (token == NULL) {
            continue;
        }

        // Handle "add <n> workers" and "remove <n> workers" commands
        if (strcmp(token, "add") == 0 || strcmp(token, "remove") == 0) {
            bool is_add = strcmp(token, "add") == 0;

            token = strtok(NULL, DELIM);
            if (token == NULL) {
                printf("error: expected number of workers\n");
                continue;
            }
            errno = 0;
            // Try to parse the number of workers which must be a positive integer
            long num_workers = strtol(token, NULL, 10);
            if (errno != 0) {
                printf("error: %s is not a valid number\n");
                continue;
            }
            if (num_workers <= 0) {
                printf("error: number of workers must be a positive number\n");
                continue;
            }
            token = strtok(NULL, DELIM);
            if (token == NULL || (strcmp(token, "worker") != 0 && strcmp(token, "workers") != 0)) {
                printf("error: expected keyword \"worker\"\n");
                continue;
            }

            if (is_add) {
                send_cmd_add_workers(dispatcher_tx, num_workers);
            } else {
                send_cmd_remove_workers(dispatcher_tx, num_workers);
            }
        }
        // Handle "show" commands
        else if (strcmp(token, "show") == 0) {
            token = strtok(NULL, DELIM);
            if (token == NULL) {
                printf("error: expected token 'info' or 'progress'\n");
                continue;
            }
            // Handle "show info" command
            if (strcmp(token, "info") == 0) {
                send_cmd_info(dispatcher_tx);
                read_exact(dispatcher_rx, &rsp, sizeof(struct rsp));
                num_workers = rsp.info_header.num_workers;
                printf("  dispatcher_pid = %d, num_workers = %ld\n", dispatcher_pid, num_workers);
                for (size_t i = 0; i < num_workers; i++) {
                    read_exact(dispatcher_rx, &rsp, sizeof(struct rsp));
                    printf("    worker pid = %d\n", rsp.info.pid);
                }
            // Handle "show progress" command
            } else if (strcmp(token, "progress") == 0) {
                send_cmd_progress(dispatcher_tx);
                read_exact(dispatcher_rx, &rsp, sizeof(struct rsp));
                print_response("  progress: ", &rsp);
            } else {
                printf("error: expected token 'info' or 'progress'\n");
            }
        } else {
            printf("error: invalid command: %s\n", cmd_buf);
        }
    }
    printf("Hasta luego\n");
}
