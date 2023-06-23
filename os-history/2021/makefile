CX := gcc
CXFLAGS := -O2
SRCS         := $(wildcard *.c)
OBJS         := $(subst .c,.o,$(SRCS))
ELFS 	     := $(subst .c,,$(SRCS))
all: $(OBJS)
	$(CX) $(CXFLAGS) -o $(ELFS) $(OBJS)
%.o: %.c
	$(CX) $(CXFLAGS) -c $<

.phony: clean
clean:
	rm -f $(OBJS) $(ELFS)