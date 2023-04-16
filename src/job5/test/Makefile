all: main.o sci.o
	cc -o app main.o sci.o

main.o: main.c sci.h
	cc -c main.c

sci.o: sci.c sci.h
	cc -c sci.c

clean: 
	rm -f app *.o