SRC 		:= *.c
LIBRARIES	:= 
MAIN		:= sh4
TARGET 		:= sh4

SOURCES		:= $(wildcard $(SRC))

OBJECTS		:= $(patsubst %.c, $(OBJDIR)/%.o, $(SOURCES))

OUTPUTMAIN	:= $(OUTPUT)/$(MAIN)

DEPS := Makefile.dep

.PHONY: all
all:

ifeq ($(MAKELEVEL),0)
all: msg
else
all: $(DEPS) $(OUTPUTMAIN)
	@echo Executing $(TARGET) complete!
endif

$(DEPS): $(SOURCES)
	@echo Generating dependencies...
	-@touch $(DEPS) > /dev/null
	@$(CC) $(CFLAGS) $(INCLUDES) -MM $^ > $(DEPS)

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(OUTPUTMAIN): $(OBJECTS)
	@echo Linking $(TARGET)...
	$(CC) $(CFLAGS) $(INCLUDES) -o $(OUTPUTMAIN) $(OBJECTS) $(LFLAGS) $(LIBS) $(LIBRARIES)

msg:
	$(error Please run make from the root directory)

.PHONY: clean
clean:
	$(RM) $(OUTPUTMAIN)
	$(RM) $(OBJECTS)
	@echo Cleanup $(TARGET) complete!

.PHONY: run
run: all
	./$(OUTPUTMAIN)

include $(DEPS)