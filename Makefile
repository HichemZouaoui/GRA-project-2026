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

# ------------------------------------------------------------
# SystemC configuration
# ------------------------------------------------------------
# Project requirement:
#   - C code: C17
#   - C++ code: C++14
#   - SystemC: 2.3.3 or 2.3.4
#
# Do NOT switch this project to C++17 just because a local SystemC
# installation requires it. In that case, SYSTEMC_HOME points to an
# incompatible SystemC installation. Use a SystemC 2.3.3/2.3.4 install.

NO_SYSTEMC_GOALS := clean print-tests
NEEDS_SYSTEMC := $(filter-out $(NO_SYSTEMC_GOALS),$(MAKECMDGOALS))
ifeq ($(strip $(MAKECMDGOALS)),)
NEEDS_SYSTEMC := yes
endif

ifneq ($(strip $(NEEDS_SYSTEMC)),)
ifndef SYSTEMC_HOME
$(error SYSTEMC_HOME is not set. Please export SYSTEMC_HOME to your SystemC 2.3.3/2.3.4 installation path.)
endif

SYSTEMC_INC := -I$(SYSTEMC_HOME)/include
SYSTEMC_LIB_DIRS := $(SYSTEMC_HOME)/lib-linux64 $(SYSTEMC_HOME)/lib-linux $(SYSTEMC_HOME)/lib64 $(SYSTEMC_HOME)/lib
SYSTEMC_LIB_DIR := $(firstword $(wildcard $(SYSTEMC_LIB_DIRS)))
ifeq ($(SYSTEMC_LIB_DIR),)
$(error Could not find SystemC library directory under SYSTEMC_HOME=$(SYSTEMC_HOME). Make sure SYSTEMC_HOME points to a built/installed SystemC 2.3.3 or 2.3.4 directory.)
endif
SYSTEMC_LIB := -L$(SYSTEMC_LIB_DIR) -lsystemc
else
SYSTEMC_INC :=
SYSTEMC_LIB :=
SYSTEMC_LIB_DIR :=
endif

CPPFLAGS := -I. -I$(INC_DIR) $(SYSTEMC_INC)
CXXFLAGS := $(CXX_STD) $(WARNINGS)
CFLAGS   := $(C_STD) $(WARNINGS)
LDFLAGS  := $(SYSTEMC_LIB) -lm

CPP_SOURCES := $(wildcard $(SRC_DIR)/*.cpp)
ROOT_C_SOURCES := $(wildcard *.c)
SRC_C_SOURCES := $(wildcard $(SRC_DIR)/*.c)
C_SOURCES := $(ROOT_C_SOURCES) $(SRC_C_SOURCES)

PROJECT_CPP_SOURCES := $(filter-out $(SRC_DIR)/fpu_module.cpp,$(CPP_SOURCES))
PROJECT_CPP_OBJECTS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(PROJECT_CPP_SOURCES))
PROJECT_ROOT_C_OBJECTS := $(patsubst %.c,$(BUILD_DIR)/root_%.co,$(ROOT_C_SOURCES))
PROJECT_SRC_C_OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.co,$(SRC_C_SOURCES))
PROJECT_OBJECTS := $(PROJECT_CPP_OBJECTS) $(PROJECT_ROOT_C_OBJECTS) $(PROJECT_SRC_C_OBJECTS)

LIB_CPP_SOURCES := $(filter-out $(SRC_DIR)/main.cpp $(SRC_DIR)/fpu_module.cpp,$(CPP_SOURCES))
LIB_ROOT_C_SOURCES := $(filter-out main.c,$(ROOT_C_SOURCES))
LIB_SRC_C_SOURCES := $(filter-out $(SRC_DIR)/main.c,$(SRC_C_SOURCES))
LIB_CPP_OBJECTS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/lib_%.o,$(LIB_CPP_SOURCES))
LIB_ROOT_C_OBJECTS := $(patsubst %.c,$(BUILD_DIR)/lib_root_%.co,$(LIB_ROOT_C_SOURCES))
LIB_SRC_C_OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/lib_%.co,$(LIB_SRC_C_SOURCES))
LIB_OBJECTS := $(LIB_CPP_OBJECTS) $(LIB_ROOT_C_OBJECTS) $(LIB_SRC_C_OBJECTS)

TEST_SOURCES := $(shell find $(TEST_DIR) -name 'test_*.cpp' | sort)
TEST_TARGETS := $(foreach s,$(TEST_SOURCES),$(BIN_DIR)/$(basename $(notdir $(s))))
SYSTEMC_TEST_NAMES := test_systemc_module test_systemc_full_ops_contract
SYSTEMC_MODULE_OBJECT := $(BUILD_DIR)/lib_fpu_module.o

.PHONY: all tests clean run-tests print-tests check-systemc
all: project tests
tests: $(TEST_TARGETS) | check-systemc

project: $(PROJECT_OBJECTS) | check-systemc
	$(CXX) $(PROJECT_OBJECTS) -o $@ $(LDFLAGS)

check-systemc:
	@mkdir -p $(BUILD_DIR)
	@(printf '%s\n' '#include <systemc.h>' 'int sc_main(int argc, char** argv) { (void)argc; (void)argv; return 0; }' | \
		$(CXX) $(CPPFLAGS) $(CXXFLAGS) -x c++ -c -o $(BUILD_DIR)/systemc_check.o - \
		>$(BUILD_DIR)/systemc_check.out 2>$(BUILD_DIR)/systemc_check.err) || { \
		echo; \
		echo '[ERROR] SystemC cannot be compiled with the required C++14 standard.'; \
		echo '[ERROR] The project specification requires C++14 and SystemC 2.3.3 or 2.3.4.'; \
		echo '[ERROR] Your SYSTEMC_HOME=$(SYSTEMC_HOME) probably points to a newer SystemC that requires C++17.'; \
		echo '[ERROR] Fix: export SYSTEMC_HOME=/path/to/systemc-2.3.3-or-2.3.4'; \
		echo; \
		cat $(BUILD_DIR)/systemc_check.err; \
		exit 1; \
	}

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/root_%.co: %.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.co: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/lib_%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/lib_root_%.co: %.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/lib_%.co: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

define MAKE_TEST_RULE
$(BIN_DIR)/$(basename $(notdir $(1))): $(1) $(LIB_OBJECTS) $(if $(filter $(basename $(notdir $(1))),$(SYSTEMC_TEST_NAMES)),$(SYSTEMC_MODULE_OBJECT),)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $$< $(LIB_OBJECTS) $(if $(filter $(basename $(notdir $(1))),$(SYSTEMC_TEST_NAMES)),$(SYSTEMC_MODULE_OBJECT),) -o $$@ $(LDFLAGS)
endef
$(foreach src,$(TEST_SOURCES),$(eval $(call MAKE_TEST_RULE,$(src))))

run-tests: tests
	@for test in $(TEST_TARGETS); do echo "Running $$test"; $$test; done

print-tests:
	@printf '%s\n' $(TEST_TARGETS)

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR) $(PROJECT_EXE)
	rm -f *.vcd runtime_cli_valid.csv runtime_cli_output.txt strict_runtime_*.csv strict_runtime_*.vcd strict_runtime_output.txt
	rm -f cli_help_output_contract.txt cli_run_output_contract.txt cli_output_contract_valid.csv
	rm -f csv_contract_valid.csv csv_contract_invalid.csv simulation_contract_*.vcd test_tracefile_waveforms.vcd test_simulation_trace.vcd
