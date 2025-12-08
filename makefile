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
CFLAGS   := $(CSTD) -Wall -Wextra -O2 -g -Iinclude -Wno-unused-variable  # Supress unused variable warnings
LDFLAGS  :=
LDLIBS   :=

# Root-level query processor sources (any file starting with QPE and ending .c)
QPE_SRCS  := $(wildcard QPE*.c)
QPE_OBJS  := $(QPE_SRCS:.c=.o)
# Executables (only those sources that currently define a main). Adjust as others gain mains.
QPE_EXES  := QPESeq QPEOMP QPEMPI
CE_SRCS    := connectEngine.c
CE_OBJS    := $(CE_SRCS:.c=.o)

# Test sources (all .c in tests directory)
TEST_SRCS    := $(wildcard tests/*.c)
TEST_BIN_DIR := build/tests
TEST_BINS    := $(patsubst tests/%.c,$(TEST_BIN_DIR)/%,$(TEST_SRCS))

# engine sources required for linking (only the modern B+ tree for now)
ENGINE_COMMON_SRCS := engine/bplus.c engine/recordSchema.c engine/printHelper.c
ENGINE_SERIAL_SRCS := $(ENGINE_COMMON_SRCS) engine/serial/buildEngine-serial.c engine/serial/executeEngine-serial.c
ENGINE_SERIAL_OBJS := $(ENGINE_SERIAL_SRCS:.c=.o)

# OMP engine sources
ENGINE_OMP_SRCS := $(ENGINE_COMMON_SRCS) engine/omp/executeEngine-omp.c engine/omp/buildEngine-omp.c
ENGINE_OMP_OBJS := $(ENGINE_OMP_SRCS:.c=.o)

# MPI engine sources
ENGINE_MPI_SRCS := $(ENGINE_COMMON_SRCS) engine/mpi/executeEngine-mpi.c engine/mpi/buildEngine-mpi.c
ENGINE_MPI_OBJS := $(ENGINE_MPI_SRCS:.c=.o)

# Tokenizer sources
TOKENIZER_SRCS := tokenizer/src/tokenizer.c
TOKENIZER_OBJS := $(TOKENIZER_SRCS:.c=.o)

.PHONY: all clean test show run

all: $(ENGINE_SERIAL_OBJS) $(ENGINE_OMP_OBJS) $(ENGINE_MPI_OBJS) $(QPE_OBJS) $(QPE_EXES) $(TEST_BINS)

# Ensure engine object built before parallel links

# Object build rule for all QPE sources (compile only if no main yet)
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Specific build rule for QPEOMP.o to include OpenMP flags
QPEOMP.o: QPEOMP.c
	$(CC) $(CFLAGS) -fopenmp -pthread -c $< -o $@

# Specific build rule for QPEMPI.o to include MPI flags
QPEMPI.o: QPEMPI.c
	mpicc $(CFLAGS) -c $< -o $@

# Link rule for QPESeq (has a main)
QPESeq: QPESeq.o $(ENGINE_SERIAL_OBJS) $(TOKENIZER_OBJS) connectEngine.o
	$(CC) $(CFLAGS) QPESeq.o $(ENGINE_SERIAL_OBJS) $(TOKENIZER_OBJS) connectEngine.o $(LDFLAGS) $(LDLIBS) -o $@

# Link rule for QPEOMP (has a main, needs OpenMP)
QPEOMP: QPEOMP.o $(ENGINE_OMP_OBJS) $(TOKENIZER_OBJS)
	$(CC) $(CFLAGS) -fopenmp -pthread QPEOMP.o $(ENGINE_OMP_OBJS) $(TOKENIZER_OBJS) $(LDFLAGS) $(LDLIBS) -o $@

# Link rule for QPEMPI (has a main, needs MPI)
QPEMPI: QPEMPI.o $(ENGINE_MPI_OBJS) $(TOKENIZER_OBJS)
	mpicc $(CFLAGS) QPEMPI.o $(ENGINE_MPI_OBJS) $(TOKENIZER_OBJS) $(LDFLAGS) $(LDLIBS) -o $@

# Pattern rule for test executables (placed under build/tests)
$(TEST_BIN_DIR)/%: tests/%.c $(ENGINE_SERIAL_OBJS) $(TOKENIZER_OBJS)
	@mkdir -p $(TEST_BIN_DIR)
	$(CC) $(CFLAGS) $< $(ENGINE_SERIAL_OBJS) $(TOKENIZER_OBJS) $(LDFLAGS) $(LDLIBS) -o $@

# Special case: test_tokenizer_new needs tokenizer objects
$(TEST_BIN_DIR)/test_tokenizer_new: tests/test_tokenizer_new.c $(ENGINE_SERIAL_OBJS) $(TOKENIZER_OBJS)
	@mkdir -p $(TEST_BIN_DIR)
	$(CC) $(CFLAGS) $< $(ENGINE_SERIAL_OBJS) $(TOKENIZER_OBJS) $(LDFLAGS) $(LDLIBS) -o $@

# Engine object build rule
engine/serial/%.o: engine/serial/%.c include/*.h
	$(CC) $(CFLAGS) -c $< -o $@

engine/omp/%.o: engine/omp/%.c include/*.h
	$(CC) $(CFLAGS) -fopenmp -c $< -o $@

engine/mpi/%.o: engine/mpi/%.c include/*.h
	mpicc $(CFLAGS) -c $< -o $@

engine/%.o: engine/%.c include/*.h
	$(CC) $(CFLAGS) -c $< -o $@

# Tokenizer object build rule
tokenizer/src/%.o: tokenizer/src/%.c include/sql.h
	$(CC) $(CFLAGS) -c $< -o $@

# Convenience target to run all tests sequentially
test: $(TEST_BINS)
	@echo "Running tests..."
	@set -e; for t in $(TEST_BINS); do echo "==> $$t"; $$t || exit 1; done
	@echo "All tests completed."

# Run the serial version
run: QPESeq
	./QPESeq

# Default OpenMP thread count unless overridden
OMP_THREADS ?= 4

# Run OpenMP version correctly — env var applies only to this command
run-omp: QPEOMP
	@echo "Running with OMP_NUM_THREADS=$(OMP_THREADS)"
	@env OMP_NUM_THREADS=$(OMP_THREADS) ./QPEOMP

# Default MPI process count unless overridden
MPI_PROCS ?= 4

# Run MPI version with proper launcher syntax
run-mpi: QPEMPI
	@echo "Running with $(MPI_PROCS) MPI processes..."
	@mpirun -np $(MPI_PROCS) ./QPEMPI 2>/dev/null \
	 || mpiexec -np $(MPI_PROCS) ./QPEMPI

# Run QPESeq under Valgrind to check for memory leaks
valgrind: QPESeq
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=1 ./QPESeq

# Show discovered source collections
show:
	@echo "QPE_SRCS = $(QPE_SRCS)"
	@echo "QPE_OBJS = $(QPE_OBJS)"
	@echo "QPE_EXES = $(QPE_EXES)"
	@echo "TEST_SRCS = $(TEST_SRCS)"
	@echo "TEST_BINS = $(TEST_BINS)"
	@echo "ENGINE_SERIAL_SRCS = $(ENGINE_SERIAL_SRCS)"

clean:
	$(RM) $(QPE_EXES) $(QPE_OBJS) $(TEST_BINS) $(ENGINE_SERIAL_OBJS) $(ENGINE_OMP_OBJS) $(ENGINE_MPI_OBJS) $(TOKENIZER_OBJS) connectEngine.o
	@echo "Cleaned build artifacts."

# Default goal if user just runs `make` without target
.DEFAULT_GOAL := all