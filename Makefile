# ============================================================
# Makefile for GRA Floating Point Unit / FMA Project
# ============================================================

CXX := g++
CC  := gcc

CXX_STD := -std=c++14
C_STD   := -std=c17
WARNINGS := -Wall -Wextra -pedantic

SRC_DIR   := src
INC_DIR   := include
TEST_DIR  := test
BUILD_DIR := build
BIN_DIR   := bin
PROJECT_EXE := project

ifndef SYSTEMC_HOME
$(error SYSTEMC_HOME is not set. Please export SYSTEMC_HOME to your SystemC installation path.)
endif

SYSTEMC_INC := -I$(SYSTEMC_HOME)/include
SYSTEMC_LIB_DIRS := $(SYSTEMC_HOME)/lib-linux64 $(SYSTEMC_HOME)/lib-linux $(SYSTEMC_HOME)/lib64 $(SYSTEMC_HOME)/lib
SYSTEMC_LIB_DIR := $(firstword $(wildcard $(SYSTEMC_LIB_DIRS)))
ifeq ($(SYSTEMC_LIB_DIR),)
$(error Could not find SystemC library directory under SYSTEMC_HOME=$(SYSTEMC_HOME))
endif
SYSTEMC_LIB := -L$(SYSTEMC_LIB_DIR) -lsystemc

CPPFLAGS := -I$(INC_DIR) $(SYSTEMC_INC)
CXXFLAGS := $(CXX_STD) $(WARNINGS)
CFLAGS   := $(C_STD) $(WARNINGS)
LDFLAGS  := $(SYSTEMC_LIB) -lm

CPP_SOURCES := $(wildcard $(SRC_DIR)/*.cpp)
C_SOURCES   := $(wildcard $(SRC_DIR)/*.c)

PROJECT_CPP_OBJECTS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(CPP_SOURCES))
PROJECT_C_OBJECTS   := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.co,$(C_SOURCES))
PROJECT_OBJECTS := $(PROJECT_CPP_OBJECTS) $(PROJECT_C_OBJECTS)

LIB_CPP_SOURCES := $(filter-out $(SRC_DIR)/main.cpp,$(CPP_SOURCES))
LIB_C_SOURCES   := $(filter-out $(SRC_DIR)/main.c,$(C_SOURCES))
LIB_CPP_OBJECTS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/lib_%.o,$(LIB_CPP_SOURCES))
LIB_C_OBJECTS   := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/lib_%.co,$(LIB_C_SOURCES))
LIB_OBJECTS := $(LIB_CPP_OBJECTS) $(LIB_C_OBJECTS)

TEST_SOURCES := $(shell find $(TEST_DIR) -name 'test_*.cpp' | sort)
TEST_TARGETS := $(foreach s,$(TEST_SOURCES),$(BIN_DIR)/$(basename $(notdir $(s))))

.PHONY: all project tests clean run-tests print-tests
all: project tests
project: $(PROJECT_EXE)
tests: $(TEST_TARGETS)

$(PROJECT_EXE): $(PROJECT_OBJECTS)
	$(CXX) $^ -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.co: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/lib_%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/lib_%.co: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

define MAKE_TEST_RULE
$(BIN_DIR)/$(basename $(notdir $(1))): $(1) $(LIB_OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $$< $(LIB_OBJECTS) -o $$@ $(LDFLAGS)
endef
$(foreach src,$(TEST_SOURCES),$(eval $(call MAKE_TEST_RULE,$(src))))

run-tests: tests
	@for test in $(TEST_TARGETS); do echo "Running $$test"; $$test; done

print-tests:
	@printf '%s\n' $(TEST_TARGETS)

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR) $(PROJECT_EXE)
	rm -f *.vcd runtime_cli_valid.csv runtime_cli_output.txt
