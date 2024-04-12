#include <fcntl.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "util.h"

// The amount of bytes to distribute to each worker
#define CHUNK 4

// The maximum number of workers that we're willing to spawn
#define MAX_WORKERS 1024

// A struct containing our knowledge about a worker
struct worker {
    // A file descriptor connected to the worker's stdin
    int fd;
    // The pid of the worker
    pid_t pid;
    // Whether the worker is alive
    bool is_alive;
    // Whether a command has been assigned to the worker
    bool is_pending;
    // The command that has been assigned, if any
    struct cmd_scan cmd;
};

int main (int argc, char* argv[]){
    if (argc != 3) {
        fprintf(stderr, "Wrong number of arguments\n");
        exit(EXIT_FAILURE);
    }

    struct stat sb;
    if (stat(argv[1], &sb) == -1) {
        perror("fstat");
        exit(EXIT_FAILURE);
    }

    // An array storing the state of each worker.
    struct worker workers[MAX_WORKERS];
    // An array of file descriptors that we're interested in reading.
    // There is one per worker and one for the frontend.
    struct pollfd fds[MAX_WORKERS + 1];

    // A stack of failed commands that need to be retried. Commands are pushed
    // onto the stack when active workers are removed or when workers die.
    struct cmd_scan retry_cmds[MAX_WORKERS];
    // The number of to-be-retried commands in the stack
    size_t retry_cmds_len = 0;

    // The current number of workers
    size_t num_workers = 0;
    // The total number of bytes that need to be scanned
    uint64_t bytes_total = sb.st_size;
    // The total number of bytes successfully scanned 
    uint64_t bytes_scanned = 0;
    // The offset that will be assigned next to a worker
    uint64_t next_offset = 0;
    // The total number of characters found in the scanned bytes
    uint64_t total_count = 0;

    // Register stdin for reading
    fds[0].fd = 0;
    fds[0].events = POLLIN;

    struct cmd cmd;
    struct rsp_scan worker_rsp;
    while (1) {
        if (poll(fds, num_workers + 1, -1) < 0) {
            perror("poll");
            exit(EXIT_FAILURE);
        }

        // First we process the commands from the frontend
        if (fds[0].revents != 0) {
            if (fds[0].revents & POLLIN) {
                read_exact(0, &cmd, sizeof(struct cmd));
                print_command("dispatcher: received command: ", &cmd);
                switch (cmd.tag) {
                    case ADD_WORKERS:
                        // Can't add more than MAX_WORKERS
                        if (num_workers + cmd.alter.num_workers > MAX_WORKERS) {
                            cmd.alter.num_workers = MAX_WORKERS - num_workers;
                        }
                        for (size_t i = 0; i < cmd.alter.num_workers; i++) {
                            fds[num_workers + 1].fd = -1;
                            workers[num_workers].is_alive = false;
                            num_workers += 1;
                        }
                        break;
                    case REMOVE_WORKERS:
                        // Can't remove more workers that the current number of them
                        if (num_workers < cmd.alter.num_workers) {
                            cmd.alter.num_workers = num_workers;
                        }
                        for (size_t i = 0; i < cmd.alter.num_workers; i++) {
                            if (workers[num_workers - 1].is_pending) {
                                retry_cmds[retry_cmds_len] = workers[num_workers - 1].cmd;
                                retry_cmds_len += 1;
                            }
                            if (workers[num_workers - 1].is_alive) {
                                close(workers[num_workers - 1].fd);
                                close(fds[num_workers].fd);
                            }
                            num_workers -= 1;
                        }
                        break;
                    case PROGRESS:
                        send_rsp_progress(1, bytes_total, bytes_scanned, total_count);
                        break;
                    case INFO:
                        send_rsp_info_header(1, num_workers);
                        for (size_t i = 0; i < num_workers; i++) {
                            send_rsp_info(1, workers[i].pid);
                        }
                        break;
                    default:
                        fprintf(stderr, "Invalid command tag\n");
                        exit(EXIT_FAILURE);
                }
            } else {
                fprintf(stderr, "dispatcher: frontend disconnected. exiting\n");
                return EXIT_SUCCESS;
            }
        }

        // Then process all events from existing workers
        for (size_t w = 0; w < num_workers; w++) {
            // First, process input from the workers. We will either get back
            // results or learn that a worker died.
            if (fds[w + 1].revents != 0) {
                if (fds[w + 1].revents & POLLIN) {
                    read_exact(fds[w + 1].fd, &worker_rsp, sizeof(struct rsp_scan));
                    total_count += worker_rsp.count;
                    workers[w].is_pending = false;
                    bytes_scanned += workers[w].cmd.size;
                    fprintf(stderr, "dispatcher: worker %ld counted %ld\n", w, worker_rsp.count);
                } else {
                    fprintf(stderr, "dispatcher: worker %ld died\n", w);
                    // If it had a command pending put it in the retry stack so
                    // that another worker picks it up
                    if (workers[w].is_pending) {
                        retry_cmds[retry_cmds_len] = workers[w].cmd;
                        retry_cmds_len += 1;
                    }
                    // Mark the worker as dead
                    fds[w + 1].fd = -1;
                    workers[w].is_alive = false;
                }
            }

            // If the worker is marked as dead we must start a new one
            if (!workers[w].is_alive) {
                int handles[2];
                char *child_argv[] = {"a1.4-worker", argv[1], argv[2], NULL};
                pid_t pid = spawn("a1.4-worker", child_argv, handles);
                // Store the file descriptors required to communicate with this worker
                fds[w + 1].fd = handles[0];
                fds[w + 1].events = POLLIN;

                workers[w].fd = handles[1];
                workers[w].pid = pid;
                workers[w].is_alive = true;
                workers[w].is_pending = false;
            }

            // Finally, assign work if there is any left and the worker is not busy
            if (!workers[w].is_pending && (retry_cmds_len > 0 || next_offset < bytes_total)) {
                if (retry_cmds_len > 0) {
                    retry_cmds_len -= 1;
                    workers[w].cmd = retry_cmds[retry_cmds_len];
                } else {
                    size_t size = CHUNK;
                    if (next_offset + size > bytes_total) {
                        size = bytes_total - next_offset;
                    }
                    workers[w].cmd.offset = next_offset;
                    workers[w].cmd.size = size;
                    next_offset += size;
                }
                workers[w].is_pending = true;
                write_exact(workers[w].fd, &workers[w].cmd, sizeof(struct cmd_scan));
                fprintf(
                    stderr,
                    "dispatcher: assigned offset=%ld size=%ld to worker %ld\n",
                    workers[w].cmd.offset, workers[w].cmd.size, w
                );
            }
        }
        // Reap any dead workers
        waitpid(-1, NULL, WNOHANG);
    }
}
