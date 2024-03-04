all: exec exec2

exec: zing.o main.o
	gcc zing.o main.o -o exec

main.o: main.c
	gcc -c main.c

exec2: zing2.o main.o
	gcc zing2.o main.o -o exec2
zing2.o: zing2.c
	gcc -c zing2.c
clean:
	rm -f main.o exec
