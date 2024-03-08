#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define HELPER "./a1.1-C"

void explain_wait_status(pid_t pid, int status) {
  if (WIFEXITED(status))
    fprintf(stderr,
            "Child with PID = %ld terminated normally, exit status = %d\n",
            (long)pid, WEXITSTATUS(status));
  else if (WIFSIGNALED(status))
    fprintf(stderr,
            "Child with PID = %ld was terminated by a signal, signo = %d\n",
            (long)pid, WTERMSIG(status));
  else if (WIFSTOPPED(status))
    fprintf(stderr,
            "Child with PID = %ld has been stopped by a signal, signo = %d\n",
            (long)pid, WSTOPSIG(status));
  else {
    fprintf(stderr,
            "%s: Internal error: Unhandled case, PID = %ld, status = %d\n",
            __func__, (long)pid, status);
    exit(EXIT_FAILURE);
  }
  fflush(stderr);
}

void main(int argc, char *argv[]) {
  if (argc < 4) {
    printf("Usage: %s INPUT OUTPUT CHAR\nSearches for character CHAR in INPUT "
           "and saves the result in OUTPUT.\n",
           argv[0]);
    exit(EXIT_FAILURE);
  }

  int input_fd = open(argv[1], O_RDONLY);
  if (input_fd == -1) {
    perror("Could not open input file");
    exit(EXIT_FAILURE);
  }

  int output_fd = open(argv[2], O_WRONLY | O_TRUNC | O_CREAT, 0644);
  if (output_fd == -1) {
    perror("Could not open output file");
    exit(EXIT_FAILURE);
  }

  if (strlen(argv[3]) != 1) {
    fprintf(stderr, "Expected single character but '%s' was provided", argv[3]);
    exit(EXIT_FAILURE);
  }
  char needle = argv[3][0];

  pid_t child_pid = fork();
  if (child_pid == -1) {
    perror("Failed to fork child");
    exit(EXIT_FAILURE);
  }
  if (child_pid == 0) {
    printf("Hello world! Child PID is %d. Parent PID is %d\n", getpid(),
           getppid());

    // We need enough bytes to write "/dev/fd/" (8 bytes), the maximum possible
    // fd number (20 bytes) and a null byte.
    char input[30];
    snprintf(input, 30, "/dev/fd/%d", input_fd);
    char output[30];
    snprintf(output, 30, "/dev/fd/%d", output_fd);
    char *child_argv[] = {HELPER, input, output, argv[3], NULL};

    if (execve(HELPER, child_argv, NULL) == -1) {
      perror("Could execute helper program");
      exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
  } else {
    // Wait for the child to exit
    int status;

    int ret = waitpid(child_pid, &status, 0);
    if (ret == -1) {
      perror("waitpid");
      exit(EXIT_FAILURE);
    }
    explain_wait_status(child_pid, status);

    if (close(input_fd) == -1) {
      perror("Could not close input file");
      exit(EXIT_FAILURE);
    }

    if (close(output_fd) == -1) {
      perror("Could not close output file");
      exit(EXIT_FAILURE);
    }
  }

  exit(EXIT_SUCCESS);
}
