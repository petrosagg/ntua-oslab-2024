#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/wait.h>

// TODO: define signal handler function

int main(int argc, char *argv[]) {
  char buf[1024];
  int status;
  // TODO: Receive the number of desired processes from argv
  int children = 3;
  int count = 0;

  if (argc != 3) {
    printf("Usage: %s <input_file> <char>\n", argv[0]);
    return EXIT_FAILURE;
  }

  // TODO: check for length using strlen, it should always be 1
  char needle = argv[2][0];

  ssize_t fd = open(argv[1], O_RDONLY);

  // TODO: Use pipe() to create a pair of file descriptors. We will need the
  // pipe to be in O_DIRECT mode in order for the counts to be sent as separate
  // "packets". See man 2 pipe for O_DIRECT details.

  for (int i = 0; i < children; i++) {
    ssize_t pid = fork();
    if (pid < 0) {
      perror("fork");
      return EXIT_FAILURE;
    } else if (pid == 0) {
      // read the file
      while (1) {
        ssize_t p = read(fd, buf, sizeof(buf));
        if (p < 0) {
          perror("read");
          return EXIT_FAILURE;
        }
        if (p == 0) {
          // TODO: Send this child's count in the pipe
          return EXIT_SUCCESS;
        }

        for (int j = 0; j < p; j++) {
          if (buf[j] == needle) {
            counts++;
          }
        }
        sleep(1);
      }
      return EXIT_SUCCESS;
    }
  }

  // wait for all children
  int total = 0;
  for (int i = 0; i < children; i++) {
    wait(&status);
    // TODO: read one count from the receiving end of pipe and add it to the total
  }
  printf("Found %d occurences in total\n", total);

  return 0;
}
