SRC 		:= *.c
LIBRARIES	:= 
MAIN		:= sh4
TARGET 		:= sh4

SOURCES		:= $(wildcard $(SRC))

OBJECTS		:= $(patsubst %.c, $(OBJDIR)/%.o, $(SOURCES))

OUTPUTMAIN	:= $(OUTPUT)/$(MAIN)

DEPS := Makefile.dep

.PHONY: all
all: MKDEP $(OUTPUTMAIN)
	@echo Executing $(TARGET) complete!

.PHONY: MKDEP
MKDEP: $(SOURCES)
	@$(CC) $(CFLAGS) $(INCLUDES) -MM $^ > $(DEPS)

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(OUTPUTMAIN): $(OBJECTS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(OUTPUTMAIN) $(OBJECTS) $(LFLAGS) $(LIBS) $(LIBRARIES)

.PHONY: clean
clean:
	$(RM) $(OUTPUTMAIN)
	$(RM) $(OBJECTS)
	@echo Cleanup $(TARGET) complete!

.PHONY: run
run: all
	./$(OUTPUTMAIN)

include $(DEPS)