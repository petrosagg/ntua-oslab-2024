#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define BUF_SIZE 1024

// A static buffer for io
char buffer[BUF_SIZE];

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

  int occurences = 0;
  while (1) {
    int count = read(input_fd, buffer, BUF_SIZE);
    if (count == -1) {
      perror("Could not read input");
      exit(EXIT_FAILURE);
    }
    // Reached EOF so our job is done
    if (count == 0) {
      break;
    }
    // count the occurences of the given character
    for (int i = 0; i < count; i++) {
      if (buffer[i] == needle) {
        occurences += 1;
      }
    }
  }
  if (close(input_fd) == -1) {
    perror("Could not close input file");
    exit(EXIT_FAILURE);
  }

  // write the result in the output file
  int size = snprintf(buffer, BUF_SIZE,
                      "The character '%c' appears %d times in file %s.\n",
                      needle, occurences, argv[1]);
  int written = 0;
  while (written < size) {
    int count = write(output_fd, buffer + written, size - written);
    if (count == -1) {
      perror("Could not write output");
      exit(EXIT_FAILURE);
    }
    written += count;
  }

  if (close(output_fd) == -1) {
    perror("Could not close output file");
    exit(EXIT_FAILURE);
  }

  exit(EXIT_SUCCESS);
}
