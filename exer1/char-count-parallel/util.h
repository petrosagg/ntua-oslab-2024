#include <stdint.h>

enum kind {
    ADD_WORKERS = 1,
    REMOVE_WORKERS = 2,
    PROGRESS = 3,
    INFO = 4,
};

// A command to alter the number of workers
struct cmd_alter {
    // The number of workers to add or remove
    size_t num_workers;
};

// A command to scan the bytes of the file starting at `offset` and ending at
// `offset + size`.
struct cmd_scan {
    // The offset to start scanning at
    off_t offset;
    // The number of bytes to scan
    size_t size;
};

struct rsp_scan {
    // The number of characters found
    uint64_t count;
};

struct rsp_progress {
    // The total number of bytes in the file
    uint64_t bytes_total;
    // The number of bytes searched so far
    uint64_t bytes_scanned;
    // The number of characters found
    uint64_t count;
};

struct rsp_info_header {
    // The number of workers
    size_t num_workers;
};

struct rsp_info {
    // The pid of a worker
    pid_t pid;
};

// A command from the frontend to the dispatcher
struct cmd {
    enum kind tag;
    struct cmd_alter alter;
};

// A response from the dispatcher to the frontend
struct rsp {
    enum kind tag;
    union {
        struct rsp_progress progress;
        struct rsp_info_header info_header;
        struct rsp_info info;
    };
};

// Read an exact number of bytes from a file descriptor.
int read_exact(int fd, void* data, size_t size);
// Write an exact number of bytes from a file descriptor.
void write_exact(int fd, void* data, size_t size);

void send_cmd_add_workers(int fd, size_t workers);
void send_cmd_remove_workers(int fd, size_t workers);

void send_cmd_info(int fd);
void send_rsp_info_header(int fd, size_t num_workers);
void send_rsp_info(int fd, pid_t worker_pid);

void send_cmd_progress(int fd);
void send_rsp_progress(int fd, uint64_t bytes_total, uint64_t bytes_scanned, uint64_t count);

// Prints a command in human readable form
void print_command(char* prefix, struct cmd* cmd);

// Prints a response in human readable form
void print_response(char* prefix, struct rsp* rsp);

// Spawns the `prog` with its stdout/stdin streams connected to the provided pipes.
pid_t spawn(char* prog, char* argv[], int pipes[2]);
