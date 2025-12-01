###############################################################################
# Project Makefile
# Builds:
#  - All root-level sources starting with QPE (QPEMPI.c, QPEOMP.c, QPESeq.c)
#  - All C test sources under tests/ (e.g. tests/*.c)
# Links each executable against the serial B+ tree implementation.
# Used primarily for CI builds and testing.
###############################################################################

CC       := gcc
CSTD     := -std=c11
CFLAGS   := $(CSTD) -Wall -Wextra -O2 -g -Iinclude
LDFLAGS  :=
LDLIBS   :=

# Root-level query processor sources (any file starting with QPE and ending .c)
QPE_SRCS  := $(wildcard QPE*.c)
QPE_OBJS  := $(QPE_SRCS:.c=.o)
# Executables (only those sources that currently define a main). Adjust as others gain mains.
QPE_EXES  := QPESeq

# Test sources (all .c in tests directory)
TEST_SRCS    := $(wildcard tests/*.c)
TEST_BIN_DIR := build/tests
TEST_BINS    := $(patsubst tests/%.c,$(TEST_BIN_DIR)/%,$(TEST_SRCS))

# Serial engine sources required for linking (only the modern B+ tree for now)
ENGINE_SERIAL_SRCS := engine/bplus.c engine/recordSchema.c engine/serial/executeEngine-serial.c engine/serial/buildEngine-serial.c engine/printHelper.c
ENGINE_SERIAL_OBJS := $(ENGINE_SERIAL_SRCS:.c=.o)

# Tokenizer sources
TOKENIZER_SRCS := tokenizer/src/tokenizer.c
TOKENIZER_OBJS := $(TOKENIZER_SRCS:.c=.o)

.PHONY: all clean test show run

all: $(ENGINE_SERIAL_OBJS) $(QPE_OBJS) $(QPE_EXES) $(TEST_BINS)

# Ensure engine object built before parallel links
.NOTPARALLEL:

# Object build rule for all QPE sources (compile only if no main yet)
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Link rule for QPESeq (has a main)
QPESeq: QPESeq.c $(ENGINE_SERIAL_OBJS) $(TOKENIZER_OBJS)
	$(CC) $(CFLAGS) $< $(ENGINE_SERIAL_OBJS) $(TOKENIZER_OBJS) $(LDFLAGS) $(LDLIBS) -o $@

# Pattern rule for test executables (placed under build/tests)
$(TEST_BIN_DIR)/%: tests/%.c $(ENGINE_SERIAL_OBJS) $(TOKENIZER_OBJS)
	@mkdir -p $(TEST_BIN_DIR)
	$(CC) $(CFLAGS) $< $(ENGINE_SERIAL_OBJS) $(TOKENIZER_OBJS) $(LDFLAGS) $(LDLIBS) -o $@

# Special case: test_tokenizer_new needs tokenizer objects
$(TEST_BIN_DIR)/test_tokenizer_new: tests/test_tokenizer_new.c $(ENGINE_SERIAL_OBJS) $(TOKENIZER_OBJS)
	@mkdir -p $(TEST_BIN_DIR)
	$(CC) $(CFLAGS) $< $(ENGINE_SERIAL_OBJS) $(TOKENIZER_OBJS) $(LDFLAGS) $(LDLIBS) -o $@

# Engine object build rule
engine/serial/%.o: engine/serial/%.c include/bplus.h
	$(CC) $(CFLAGS) -c $< -o $@

engine/%.o: engine/%.c include/bplus.h
	$(CC) $(CFLAGS) -c $< -o $@

# Tokenizer object build rule
tokenizer/src/%.o: tokenizer/src/%.c include/sql.h
	$(CC) $(CFLAGS) -c $< -o $@

# Convenience target to run all tests sequentially
test: $(TEST_BINS)
	@echo "Running tests..."
	@set -e; for t in $(TEST_BINS); do echo "==> $$t"; $$t || exit 1; done
	@echo "All tests completed."

# Show discovered source collections
show:
	@echo "QPE_SRCS = $(QPE_SRCS)"
	@echo "QPE_OBJS = $(QPE_OBJS)"
	@echo "QPE_EXES = $(QPE_EXES)"
	@echo "TEST_SRCS = $(TEST_SRCS)"
	@echo "TEST_BINS = $(TEST_BINS)"
	@echo "ENGINE_SERIAL_SRCS = $(ENGINE_SERIAL_SRCS)"

clean:
	$(RM) $(QPE_EXES) $(QPE_OBJS) $(TEST_BINS) $(ENGINE_SERIAL_OBJS) $(TOKENIZER_OBJS)
	@echo "Cleaned build artifacts."

# Default goal if user just runs `make` without target
.DEFAULT_GOAL := all


# Run the main serial query processor
run: QPESeq
	./QPESeq
