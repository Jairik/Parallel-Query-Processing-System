# File Structure & Architectural Motives

Currently, this repo is split into numerous directories/files based on functionality. The motivation behind this is to make everything more modular and aid in debugging and developer experience. File size should ultimately be minimized.

Below, each directory's purposes will be briefly explained.

## `/data-generation`

Focuses on holding the current csv file to test and the python script used to generate said data.

## `/docs`

Documentation and reports. Used mostly for developer experience and to help explain any confusing decisions.

## `/engine`

This serves as the **main powerhouse** of the program. It contains:
- `bplus.c`: The core B+ tree implementation.
- `recordSchema.c`: Schema definitions and helpers.
- `serial/`, `omp/`, `mpi/`: Subdirectories containing specific implementations for Serial, OpenMP, and MPI execution engines.

Each subdirectory contains:
- `buildEngine-*.c`: Utility functions for *building* the indexes.
- `executeEngine-*.c`: Functions for executing specific commands (SELECT, INSERT, DELETE).

This structure abstracts lower-level functionality for use in the root-level files (`QPE*.c`). For more explanation, see `bplus.md` and `engine.md`.

## `/include`

This will hold **all** header files, used as a centralized way to use different utilities and whatnot across all directories. This will just make importing and stuff easier.

## `/tests`

Any basic tests ran during development to verify the functionality of any utilities.

## `/tokenizer`

Given a string SQL command, will parse it to determine the actual functionality desired by the user.

## `QPE*.c`

The `QPEMPI.c`, `QPEOMP.c`, and `QPESeq.c` files use the wrapper functions in the `/engine` directory to perform high-level queries. These files read commands from `sample-queries.txt`, use the tokenizer to parse them, and then run them through the appropriate engine (Serial, OpenMP, or MPI) to get results. They also handle high-level benchmarking.