export CC := gcc

export MODE := debug

export ARGS :=

TARGET := all

target := sh4

export CFLAGS	:= -std=gnu17 -fdiagnostics-color=always -Wall -Wextra -Wno-implicit-fallthrough -Wno-unused-parameter

ifeq ($(MODE), debug)
export CFLAGS += -g3 -O0 -DDEBUG
endif

ifeq ($(MODE), release)
export CFLAGS += -O3 -DNDEBUG
endif

export OUTPUT := $(CURDIR)/build/$(MODE)
export OBJDIR := $(CURDIR)/build/.objs/$(MODE)

export LFLAGS := 

export INCLUDE	:= $(CURDIR)/include

export LIB		:= $(CURDIR)/lib

LIBRARIES	:= 
CMD 		:= cmdtest
PARSE 		:= parsetest
export INCLUDEDIRS	:= $(INCLUDE)
export LIBDIRS		:= $(LIB)
export RM			:= rm -rf
export MD			:= mkdir -p

export INCLUDES	:= $(patsubst %,-I%, $(INCLUDEDIRS:%/=%))

export LIBS		:= $(patsubst %,-L%, $(LIBDIRS:%/=%))

SOURCE_UTEST	:= utest.c
SOURCE_CMD		:= cmd_utest.c
SOURCE_PARSE	:= parse_utest.c
SOURCES 		:= $(SOURCE_CMD) $(SOURCE_PARSE) $(SOURCE_UTEST)

OBJECTS			:= $(patsubst %.c, $(OBJDIR)/%.o, $(SOURCES))

OBJ_UTEST 		:= $(patsubst %.c, $(OBJDIR)/%.o, $(SOURCE_UTEST))
OBJ_CMDTEST 	:= $(patsubst %.c, $(OBJDIR)/%.o, $(SOURCE_CMD))
OBJ_PARSETEST 	:= $(patsubst %.c, $(OBJDIR)/%.o, $(SOURCE_PARSE))
OBJ_CMD 		:= $(OBJDIR)/cmd.o
OBJ_PARSE 		:= $(OBJDIR)/parse.o

OBJ_DEP 		:= $(OBJ_CMD) $(OBJ_PARSE) $(OBJ_UTEST)

OUTPUT_CMDTEST	:= $(OUTPUT)/$(CMD)
OUTPUT_PARSETEST := $(OUTPUT)/$(PARSE)

ifeq ($(target), sh4)
	RUN_TARGET := $(OUTPUT)/sh4
endif
ifeq ($(target), $(CMD))
	RUN_TARGET := $(OUTPUT_CMDTEST)
endif
ifeq ($(target), $(PARSE))
	RUN_TARGET := $(OUTPUT_PARSETEST)
endif

DEPS := Makefile.dep

export exclude_dirs := build .objs	debug release
dirs:=$(shell find . -maxdepth 1 -type d)
dirs:=$(basename $(patsubst ./%,%,$(dirs)))
dirs:=$(filter-out $(exclude_dirs),$(dirs))
SUBDIRS := $(dirs)
CLEAN_SUBDIRS := $(dirs)

.PHONY: all
all: MKDIR $(DEPS) SUBMODULE $(OUTPUT_CMDTEST) $(OUTPUT_PARSETEST)
	@echo Executing $(TARGET) complete!

.PHONY: MKDIR
MKDIR:
	-@$(MD) $(OUTPUT)
	-@$(MD) $(OBJDIR)

$(DEPS): $(SOURCES)
	-@touch $(DEPS) > /dev/null
	@$(CC) $(CFLAGS) $(INCLUDES) -MM $^ > $(DEPS)

.PHONY: SUBMODULE
SUBMODULE: $(SUBDIRS)
	$(MAKE) -C $<
# $(foreach dir, $(SUBDIRS), $(MAKE) -C $(dir);)

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(OUTPUT_CMDTEST): $(OBJECTS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(OUTPUT_CMDTEST) $(OBJ_CMDTEST) $(OBJ_DEP) $(LFLAGS) $(LIBS) $(LIBRARIES)

$(OUTPUT_PARSETEST): $(OBJECTS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(OUTPUT_PARSETEST) $(OBJ_PARSETEST) $(OBJ_DEP) $(LFLAGS) $(LIBS) $(LIBRARIES)

.PHONY: clean
clean:
	$(foreach dir, $(CLEAN_SUBDIRS), $(MAKE) clean -C $(dir);)
	-$(RM) $(OUTPUT_CMDTEST) $(OUTPUT_PARSETEST)
	-$(RM) $(OBJECTS)
	@echo Cleanup $(TARGET) complete!

.PHONY: run
run: all
	$(RUN_TARGET) $(ARGS)
	@echo Executing 'run: $(TARGET)' complete!

include $(DEPS)