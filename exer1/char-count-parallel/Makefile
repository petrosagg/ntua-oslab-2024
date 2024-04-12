all: a1.4-worker a1.4-dispatcher a1.4-frontend

CC = gcc
CFLAGS = -Wall -Wextra -Werror -O2

a1.4-frontend: a1.4-frontend.o util.o # config.h
	$(CC) $(CFLAGS) $^ -o $@
	
a1.4-dispatcher: a1.4-dispatcher.o util.o # config.h
	$(CC) $(CFLAGS) $^ -o $@
	
a1.4-worker: a1.4-worker.o util.o
	$(CC) $(CFLAGS) $^ -o $@
	
%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<
    
clean:
	rm -f *.o a1.1-C a1.1-system_calls a1.2-fork a1.3-comm a1.4-frontend a1.4-dispatcher
