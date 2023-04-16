all: main.o cmd.o parse.o
	cc -o sh3 main.o cmd.o parse.o

main.o: main.c cmd.h parse.h
	cc -c main.c

cmd.o: cmd.c cmd.h
	cc -c cmd.c

parse.o: parse.c parse.h
	cc -c parse.c

clean:
	rm -f sh3 *.o