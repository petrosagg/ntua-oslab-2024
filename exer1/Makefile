all: a1.1-C a1.1-system_calls a1.2-2-fork a1.2-4-fork a1.3-comm a1.4-frontend a1.4-dispatcher a1.4-worker

CC = gcc
CFLAGS = -Wall -O2
#CFLAGS =  -O2  

a1.1-C: a1.1-C.o
	$(CC) $(CFLAGS) $^ -o $@

a1.1-system_calls: a1.1-system_calls.o
	$(CC) $(CFLAGS) $^ -o $@
	
a1.2-2-fork: a1.2-2-fork.o
	$(CC) $(CFLAGS) $^ -o $@

a1.2-4-fork: a1.2-4-fork.o
	$(CC) $(CFLAGS) $^ -o $@

a1.3-comm: a1.3-comm.o
	$(CC) $(CFLAGS) $^ -o $@

a1.4-frontend: a1.4-frontend.o util.o
	$(CC) $(CFLAGS) $^ -o $@
	
a1.4-dispatcher: a1.4-dispatcher.o util.o
	$(CC) $(CFLAGS) $^ -o $@
	
a1.4-worker: a1.4-worker.o util.o
	$(CC) $(CFLAGS) $^ -o $@
	
%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<
    
clean:
	rm -f *.o a1.1-C a1.1-system_calls a1.2-2-fork a1.2-2-fork a1.3-comm a1.4-frontend a1.4-dispatcher a1.2-4-fork a1.4-worker
