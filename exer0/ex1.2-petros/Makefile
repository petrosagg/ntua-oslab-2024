all: main main2

main.o: main.c
	gcc -c main.c

zing2.o: zing2.c
	gcc -c zing2.c

main2: main.o zing2.o
	gcc -o main2 main.o zing2.o

main: main.o zing.o
	gcc -o main main.o zing.o
