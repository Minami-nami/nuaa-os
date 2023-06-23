CX = gcc
CXFLAGS = -O2

all: 
	$(CX) -c $(CXFLAGS) t1.c
	$(CX) $(CXFLAGS) -o t1 t1.o
	$(CX) -c $(CXFLAGS) t2.c
	$(CX) $(CXFLAGS) -o t2 t2.o
	$(CX) -c $(CXFLAGS) t3.c
	$(CX) $(CXFLAGS) -o t3 t3.o
	$(CX) -c $(CXFLAGS) t4.c
	$(CX) $(CXFLAGS) -o t4 t4.o

.phony: clean
clean:
	rm -f *.o t1 t2 t3 t4
