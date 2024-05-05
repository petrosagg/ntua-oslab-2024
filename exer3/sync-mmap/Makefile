#
# Makefile
# 

CC = gcc

# CAUTION: Always use '-pthread' when compiling POSIX threads-based
# applications, instead of linking with "-lpthread" directly.
CFLAGS = -Wall -O2 -pthread
LIBS = 

all: mandel-fork-semaphore mandel-fork-sharedbuf


## Mandel

mandel-fork-semaphore: mandel-lib.o mandel-fork-semaphore.o
	$(CC) $(CFLAGS) -o mandel-fork-semaphore mandel-lib.o mandel-fork-semaphore.o $(LIBS)

mandel-fork-sharedbuf: mandel-lib.o mandel-fork-sharedbuf.o
	$(CC) $(CFLAGS) -o mandel-fork-sharedbuf mandel-lib.o mandel-fork-sharedbuf.o $(LIBS)

mandel-lib.o: mandel-lib.h mandel-lib.c
	$(CC) $(CFLAGS) -c -o mandel-lib.o mandel-lib.c $(LIBS)

mandel-fork-semaphore.o: mandel-fork.c
	$(CC) $(CFLAGS) -c -DSYNC_SEMAPHORE -o mandel-fork-semaphore.o mandel-fork.c $(LIBS)

mandel-fork-sharedbuf.o: mandel-fork.c
	$(CC) $(CFLAGS) -c -DSYNC_SHAREDBUF -o mandel-fork-sharedbuf.o mandel-fork.c $(LIBS)

clean:
	rm -f *.s *.o mandel-fork 
