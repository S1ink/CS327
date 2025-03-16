rwildcard = $(foreach d,$(wildcard $(addsuffix *,$(1))),$(call rwildcard,$(d)/,$(2)) $(filter $(subst *,%,$(2)),$(d)))

OPT ?= export

ifeq ($(filter $(OPT),export debug colorful release),)
$(error invalid 'OPT' option - choose from {export/debug/colorful/release})
endif

CC := gcc
CXX := g++

CFLAGS := -Wall -Werror -funroll-loops -Isrc
CXXFLAGS := -Wall -Werror -funroll-loops -Isrc
LDFLAGS := -lm -lncurses

SRC_DIR := src
OBJ_DIR := build

ifeq ($(OPT),export)
CFLAGS += -g -O2 -DDUNGEON_PRINT_BORDER=0 -DENABLE_DEBUG_PRINTS=0 -DDUNGEON_PRINT_HARDNESS=0
CXXFLAGS += -g -O2 -DDUNGEON_PRINT_BORDER=0 -DENABLE_DEBUG_PRINTS=0 -DDUNGEON_PRINT_HARDNESS=0
LDFLAGS += -g
else ifeq ($(OPT),debug)
CFLAGS += -g -O2 -DDUNGEON_PRINT_BORDER=0 -DENABLE_DEBUG_PRINTS=1 -DDUNGEON_PRINT_HARDNESS=1
CXXFLAGS += -g -O2 -DDUNGEON_PRINT_BORDER=0 -DENABLE_DEBUG_PRINTS=1 -DDUNGEON_PRINT_HARDNESS=1
LDFLAGS += -g
else ifeq ($(OPT),colorful)
CFLAGS += -g -O2 -DDUNGEON_PRINT_BORDER=0 -DENABLE_DEBUG_PRINTS=0 -DDUNGEON_PRINT_HARDNESS=1
CXXFLAGS += -g -O2 -DDUNGEON_PRINT_BORDER=0 -DENABLE_DEBUG_PRINTS=0 -DDUNGEON_PRINT_HARDNESS=1
LDFLAGS += -g
else
CFLAGS += -O3
CXXFLAGS += -O3
endif

BIN := game
SRCS := $(call rwildcard,$(SRC_DIR)/,*.cpp *.c)
OBJS := $(SRCS:$(SRC_DIR)/%=$(OBJ_DIR)/%.o)
HEADERS := $(call rwildcard,$(SRC_DIR)/,*.h *.hpp)
OBJ_DIRS := $(sort $(dir $(OBJS)))

.PHONY: all rebuild clean

all: $(BIN)

$(BIN): $(OBJS)
	@echo Linking $@
	@$(CC) -o $@ $^ $(LDFLAGS)

-include $(OBJS:.o=.d)

$(OBJ_DIR)/%.c.o : $(SRC_DIR)/%.c | $(OBJ_DIR) $(OBJ_DIRS)
	@echo Compiling $(<F)
	@$(CC) $(CFLAGS) -MMD -MF $(OBJ_DIR)/$*.d -c -o $@ $<

$(OBJ_DIR) $(OBJ_DIRS):
	mkdir -p $@

rebuild: clean all

clean:
	rm -rf $(OBJ_DIR) $(BIN)
