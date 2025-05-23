rwildcard = $(foreach d,$(wildcard $(addsuffix *,$(1))),$(call rwildcard,$(d)/,$(2)) $(filter $(subst *,%,$(2)),$(d)))

OPT ?= export

ifeq ($(filter $(OPT),export debug release),)
$(error invalid 'OPT' option - choose from {export/debug/release})
endif

CC := gcc
CXX := g++

CFLAGS := -Wall -Werror -funroll-loops -Isrc
CXXFLAGS := -std=c++17 -lstdc++ -Wall -Werror -Wno-narrowing -funroll-loops -Isrc -Iinclude/eigen #-fopenmp
LDFLAGS := -lm -lncurses #-fopenmp

SRC_DIR := src
OBJ_DIR := build

ifeq ($(OPT),export)
CFLAGS += -g -O2
CXXFLAGS += -g -O2
LDFLAGS += -g
else ifeq ($(OPT),debug)
CFLAGS += -g
CXXFLAGS += -g
LDFLAGS += -g
else
CFLAGS += -O3
CXXFLAGS += -O3
endif

BIN := fluid
SRCS := $(call rwildcard,$(SRC_DIR)/,*.cpp *.c)
OBJS := $(SRCS:$(SRC_DIR)/%=$(OBJ_DIR)/%.o)
HEADERS := $(call rwildcard,$(SRC_DIR)/,*.h *.hpp)
OBJ_DIRS := $(sort $(dir $(OBJS)))

.PHONY: all rebuild clean

all: $(BIN)

$(BIN): $(OBJS)
	@echo Linking $@
	@$(CXX) -o $@ $^ $(LDFLAGS)

-include $(OBJS:.o=.d)

$(OBJ_DIR)/%.c.o : $(SRC_DIR)/%.c | $(OBJ_DIR) $(OBJ_DIRS)
	@echo Compiling $(<F)
	@$(CC) $(CFLAGS) -MMD -MF $(OBJ_DIR)/$*.d -c -o $@ $<

$(OBJ_DIR)/%.cpp.o : $(SRC_DIR)/%.cpp | $(OBJ_DIR) $(OBJ_DIRS)
	@echo Compiling $(<F)
	@$(CXX) $(CXXFLAGS) -MMD -MF $(OBJ_DIR)/$*.d -c -o $@ $<

$(OBJ_DIR) $(OBJ_DIRS):
	mkdir -p $@

rebuild: clean all

clean:
	rm -rf $(OBJ_DIR) $(BIN)
