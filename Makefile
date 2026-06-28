# ==============================================================================
#  Floating-Point Unit — Makefile
# ==============================================================================
#
#  Targets:
#    make project          Release build   → ./project         (required by spec)
#    make debug            ASan + UBSan + LSan build → ./project_debug
#    make run-valgrind     Valgrind memcheck   (pass ARGS="..." for CLI args)
#    make callgrind        Callgrind profiler  (pass ARGS="..." for CLI args)
#    make cppcheck         Static analysis with cppcheck
#    make clean            Remove all build artifacts
#    make help             Print this summary
#
#  Example:
#    make run-valgrind ARGS="--cycles 500 test/requests.csv"
# ==============================================================================

TARGET    = project
SRC_DIR   = src
INC_DIR   = include
BUILD_DIR = build

# ── SystemC ───────────────────────────────────────────────────────────────────
# Accessed via $SYSTEMC_HOME, exactly like the homework assignments.
# Supports multiple possible lib directory names across different installs.
ifndef SYSTEMC_HOME
    $(error SYSTEMC_HOME is not set. Run: export SYSTEMC_HOME=/path/to/systemc)
endif

SYSTEMC_INC = $(SYSTEMC_HOME)/include

# Auto-detect the lib directory (lib-linux64 on 64-bit, lib-linux on 32-bit)
SYSTEMC_LIB := $(firstword $(wildcard \
    $(SYSTEMC_HOME)/lib-linux64 \
    $(SYSTEMC_HOME)/lib-linux   \
    $(SYSTEMC_HOME)/lib))

ifeq ($(SYSTEMC_LIB),)
    $(error Could not find SystemC lib directory under $(SYSTEMC_HOME))
endif

# ── Compilers ─────────────────────────────────────────────────────────────────
CC  = gcc
CXX = g++

# ── Language standards (required by spec) ─────────────────────────────────────
CSTD   = -std=c17
CXXSTD = -std=c++14

# ── Includes ──────────────────────────────────────────────────────────────────
# -isystem suppresses warnings coming from SystemC's own headers (not our code)
INCLUDES = -I$(INC_DIR) -isystem $(SYSTEMC_INC)

# ── Warning flags ─────────────────────────────────────────────────────────────
#
#  -Wall              Standard set: uninit vars, missing returns, implicit funcs…
#  -Wextra            Extra checks: unused params, sign comparison, empty body…
#  -Wpedantic         Strict ISO compliance; catches non-standard extensions
#  -Wshadow           Warns when a local var hides an outer-scope var
#  -Wformat=2         Strict printf/scanf format string validation
#  -Wnull-dereference Flags potential null pointer dereferences (GCC 6+)
#  -Wdouble-promotion Warns when float is silently promoted to double
#  -Wconversion       Warns on implicit type conversions that may lose data
#                     (can be noisy — comment out if it overwhelms you)
#
WARNS = \
    -Wall               \
    -Wextra             \
    -Wpedantic          \
    -Wshadow            \
    -Wformat=2          \
    -Wnull-dereference  \
    -Wdouble-promotion  \
    -Wconversion

# ── Sanitizer flags ───────────────────────────────────────────────────────────
#
#  address    → buffer overflow, use-after-free, heap/stack corruption
#  undefined  → UB like signed overflow, null deref, misaligned access, etc.
#  leak       → memory leaks on exit (included with ASan on Linux)
#
#  -fno-omit-frame-pointer: keeps stack frames so sanitizer traces are readable
#
#  !! MSan (-fsanitize=memory) detects use of uninitialised values but is
#     clang-only and CANNOT be combined with ASan. Use a separate clang build
#     if you need it: clang -fsanitize=memory -fno-omit-frame-pointer ...
#
#  !! Sanitizers CANNOT run under Valgrind — that's why there's a separate
#     valgrind build target with no sanitizer flags.
#
SANITIZE = -fsanitize=address,undefined,leak -fno-omit-frame-pointer

# ── Per-build flag sets ───────────────────────────────────────────────────────
# Release — optimized, no debug info
CFLAGS_RELEASE   = $(CSTD)   $(WARNS) $(INCLUDES) -O2
CXXFLAGS_RELEASE = $(CXXSTD) $(WARNS) $(INCLUDES) -O2

# Debug — sanitizers + full debug info, no optimization
CFLAGS_DEBUG   = $(CSTD)   $(WARNS) $(INCLUDES) -g -O0 $(SANITIZE)
CXXFLAGS_DEBUG = $(CXXSTD) $(WARNS) $(INCLUDES) -g -O0 $(SANITIZE)

# Valgrind — debug info only, NO sanitizers (they conflict with Valgrind)
CFLAGS_VALGRIND   = $(CSTD)   $(WARNS) $(INCLUDES) -g -O0
CXXFLAGS_VALGRIND = $(CXXSTD) $(WARNS) $(INCLUDES) -g -O0

# ── Linker flags ──────────────────────────────────────────────────────────────
# -Wl,-rpath embeds the SystemC lib path so you don't need LD_LIBRARY_PATH at runtime
LDFLAGS = -L$(SYSTEMC_LIB) -lsystemc -lpthread -Wl,-rpath,$(SYSTEMC_LIB)

# ── Source discovery ──────────────────────────────────────────────────────────
C_SRCS   = $(wildcard $(SRC_DIR)/*.c)
CXX_SRCS = $(wildcard $(SRC_DIR)/*.cpp)

# ── Object file paths ─────────────────────────────────────────────────────────
C_OBJS_RELEASE     = $(patsubst $(SRC_DIR)/%.c,   $(BUILD_DIR)/release/%.o,  $(C_SRCS))
CXX_OBJS_RELEASE   = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/release/%.o,  $(CXX_SRCS))

C_OBJS_DEBUG       = $(patsubst $(SRC_DIR)/%.c,   $(BUILD_DIR)/debug/%.o,    $(C_SRCS))
CXX_OBJS_DEBUG     = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/debug/%.o,    $(CXX_SRCS))

C_OBJS_VALGRIND    = $(patsubst $(SRC_DIR)/%.c,   $(BUILD_DIR)/valgrind/%.o, $(C_SRCS))
CXX_OBJS_VALGRIND  = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/valgrind/%.o, $(CXX_SRCS))

# ==============================================================================
#  BUILD TARGETS
# ==============================================================================

.PHONY: all project debug valgrind run-valgrind callgrind cppcheck clean help

all: project

# ── Release ───────────────────────────────────────────────────────────────────
# Spec requires: `make project` → executable named `project` in current dir
project: $(C_OBJS_RELEASE) $(CXX_OBJS_RELEASE)
	$(CXX) $^ $(LDFLAGS) -o $(TARGET)
	@echo ""
	@echo "  [OK] Release build complete: ./$(TARGET)"
	@echo ""

$(BUILD_DIR)/release/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)/release
	$(CC) $(CFLAGS_RELEASE) -c $< -o $@

$(BUILD_DIR)/release/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)/release
	$(CXX) $(CXXFLAGS_RELEASE) -c $< -o $@

$(BUILD_DIR)/release:
	mkdir -p $@

# ── Debug / Sanitizers ────────────────────────────────────────────────────────
debug: $(C_OBJS_DEBUG) $(CXX_OBJS_DEBUG)
	$(CXX) $^ $(LDFLAGS) $(SANITIZE) -o $(TARGET)_debug
	@echo ""
	@echo "  [OK] Debug build complete: ./$(TARGET)_debug"
	@echo "       Run normally — ASan/UBSan/LSan output appears automatically."
	@echo ""

$(BUILD_DIR)/debug/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)/debug
	$(CC) $(CFLAGS_DEBUG) -c $< -o $@

$(BUILD_DIR)/debug/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)/debug
	$(CXX) $(CXXFLAGS_DEBUG) -c $< -o $@

$(BUILD_DIR)/debug:
	mkdir -p $@

# ── Valgrind build (no sanitizers) ────────────────────────────────────────────
valgrind: $(C_OBJS_VALGRIND) $(CXX_OBJS_VALGRIND)
	$(CXX) $^ $(LDFLAGS) -o $(TARGET)_valgrind
	@echo ""
	@echo "  [OK] Valgrind build complete: ./$(TARGET)_valgrind"
	@echo ""

$(BUILD_DIR)/valgrind/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)/valgrind
	$(CC) $(CFLAGS_VALGRIND) -c $< -o $@

$(BUILD_DIR)/valgrind/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)/valgrind
	$(CXX) $(CXXFLAGS_VALGRIND) -c $< -o $@

$(BUILD_DIR)/valgrind:
	mkdir -p $@

# ── Run with Valgrind memcheck ─────────────────────────────────────────────────
#
#  --leak-check=full        full report per leak site, not just a summary
#  --show-leak-kinds=all    includes "still reachable" and "indirectly lost"
#  --track-origins=yes      traces uninitialised values to where they came from
#                           (makes Valgrind ~2× slower, but the traces are gold)
#  --error-exitcode=1       non-zero exit if any errors — useful in scripts/CI
#
#  Usage: make run-valgrind ARGS="--cycles 500 test/requests.csv"
#
ARGS ?=
run-valgrind: valgrind
	valgrind                        \
	    --tool=memcheck             \
	    --leak-check=full           \
	    --show-leak-kinds=all       \
	    --track-origins=yes         \
	    --error-exitcode=1          \
	    ./$(TARGET)_valgrind $(ARGS)

# ── Callgrind (profiling) ──────────────────────────────────────────────────────
#
#  Callgrind records every function call + instruction count.
#  Visualize the output with kcachegrind (sudo apt install kcachegrind).
#
#  Usage: make callgrind ARGS="--cycles 1000 test/requests.csv"
#
callgrind: valgrind
	valgrind                                    \
	    --tool=callgrind                        \
	    --callgrind-out-file=callgrind.out      \
	    ./$(TARGET)_valgrind $(ARGS)
	@echo ""
	@echo "  [OK] Profiling data written to: callgrind.out"
	@echo "       Visualize with: kcachegrind callgrind.out"
	@echo ""

# ── Static analysis ───────────────────────────────────────────────────────────
#
#  Requires: sudo apt install cppcheck
#
#  --enable=all         enables all checkers (style, performance, portability…)
#  --inconclusive       reports issues even when cppcheck isn't 100% certain
#  --suppress=...       silences false positives from system/library headers
#
cppcheck:
	cppcheck                            \
	    --enable=all                    \
	    --std=c17                       \
	    --inconclusive                  \
	    --suppress=missingIncludeSystem \
	    -I$(INC_DIR)                    \
	    $(SRC_DIR)/

# ── Clean ─────────────────────────────────────────────────────────────────────
clean:
	rm -rf $(BUILD_DIR) $(TARGET) $(TARGET)_debug $(TARGET)_valgrind callgrind.out
	@echo "  [OK] Cleaned."

# ── Help ──────────────────────────────────────────────────────────────────────
help:
	@echo ""
	@echo "  make project                   Release build → ./project"
	@echo "  make debug                     ASan + UBSan + LSan → ./project_debug"
	@echo "  make run-valgrind [ARGS=...]   Valgrind memcheck"
	@echo "  make callgrind    [ARGS=...]   Callgrind profiling (view with kcachegrind)"
	@echo "  make cppcheck                  Static analysis (requires cppcheck)"
	@echo "  make clean                     Remove all build artifacts"
	@echo ""