#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#include "util.h"

// Read an exact number of bytes from a file descriptor.
int read_exact(int fd, void* data, size_t size) {
    size_t cursor = 0;
    while (cursor < size) {
        int rcnt = read(fd, data + cursor, size - cursor);
        if (rcnt <= 0) {
            perror("read_exact");
            exit(EXIT_FAILURE);
        }
        cursor += rcnt;
    }
    return 0;
}

// Write an exact number of bytes from a file descriptor.
void write_exact(int fd, void* data, size_t size) {
    size_t cursor = 0;
    while (cursor < size) {
        int wcnt = write(fd, data + cursor, size - cursor);
        if (wcnt <= 0) {
            perror("write_exact");
            exit(EXIT_FAILURE);
        }
        cursor += wcnt;
    }
}

// ADD WORKERS command
void send_cmd_add_workers(int fd, size_t num_workers) {
    struct cmd cmd;
    cmd.tag = ADD_WORKERS;
    cmd.alter.num_workers = num_workers;
    write_exact(fd, &cmd, sizeof(struct cmd));
}
// REMOVE WORKERS command
void send_cmd_remove_workers(int fd, size_t num_workers) {
    struct cmd cmd;
    cmd.tag = REMOVE_WORKERS;
    cmd.alter.num_workers = num_workers;
    write_exact(fd, &cmd, sizeof(struct cmd));
}

// INFO command and response
void send_cmd_info(int fd) {
    struct cmd cmd;
    cmd.tag = INFO;
    write_exact(fd, &cmd, sizeof(struct cmd));
}
void send_rsp_info_header(int fd, size_t num_workers) {
    struct rsp rsp;
    rsp.tag = INFO;
    rsp.info_header.num_workers = num_workers;
    write_exact(fd, &rsp, sizeof(struct rsp));
}
void send_rsp_info(int fd, pid_t worker_pid) {
    struct rsp rsp;
    rsp.tag = INFO;
    rsp.info.pid = worker_pid;
    write_exact(fd, &rsp, sizeof(struct rsp));
}

// PROGRESS command and response
void send_cmd_progress(int fd) {
    struct cmd cmd;
    cmd.tag = PROGRESS;
    write_exact(fd, &cmd, sizeof(struct cmd));
}
void send_rsp_progress(int fd, uint64_t bytes_total, uint64_t bytes_scanned, uint64_t count) {
    struct rsp rsp;
    rsp.tag = PROGRESS;
    rsp.progress.bytes_total = bytes_total;
    rsp.progress.bytes_scanned = bytes_scanned;
    rsp.progress.count = count;
    write_exact(fd, &rsp, sizeof(struct rsp));
}

void print_command(char* prefix, struct cmd* cmd) {
    switch (cmd->tag) {
        case ADD_WORKERS:
            fprintf(stderr, "%sADD_WORKERS %ld\n", prefix, cmd->alter.num_workers);
            break;
        case REMOVE_WORKERS:
            fprintf(stderr, "%sREMOVE_WORKERS %ld\n", prefix, cmd->alter.num_workers);
            break;
        case PROGRESS:
            fprintf(stderr, "%sPROGRESS\n", prefix);
            break;
        case INFO:
            fprintf(stderr, "%sINFO\n", prefix);
            break;
        default:
            fprintf(stderr, "Invalid command tag\n");
            exit(EXIT_FAILURE);
    }
}

void print_response(char* prefix, struct rsp* rsp) {
    switch (rsp->tag) {
        case PROGRESS:
            fprintf(
                stderr,
                "%sPROGRESS total=%ld scanned=%ld count=%ld\n",
                prefix, rsp->progress.bytes_total,
                rsp->progress.bytes_scanned,
                rsp->progress.count
            );
            break;
        case INFO:
            fprintf(stderr, "%sINFO num_workers=%ld\n", prefix, rsp->info_header.num_workers);
            break;
        default:
            fprintf(stderr, "Invalid command tag\n");
            exit(EXIT_FAILURE);
    }
}

// Spawns the `prog` with its stdout/stdin streams connected to the provided pipes.
pid_t spawn(char* prog, char* argv[], int pipes[2]) {
    int txpair[2];
    int rxpair[2];
    if (pipe2(rxpair, O_CLOEXEC) < 0) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    if (pipe2(txpair, O_CLOEXEC) < 0) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    int pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (pid == 0) {
        // Connect rxpair[1] to the child's stdout
        dup2(rxpair[1], 1);
        // Connect txpair[0] to the child's stdin
        dup2(txpair[0], 0);
        char *child_argv[] = {prog, argv[1], argv[2], NULL};
        if (execve(prog, child_argv, NULL) < 0) {
            perror("execve");
            exit(EXIT_FAILURE);
        }
    }
    // Close the pipe ends used by the spawned process
    close(rxpair[1]);
    close(txpair[0]);
    pipes[0] = rxpair[0];
    pipes[1] = txpair[1];
    return pid;
}
