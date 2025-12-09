# Parallel Database Query Processing System

This project implements a parallel query-processing engine designed to run SQL-like queries over large, structured datasets using high-performance computing techniques. The goal is to build a lightweight, command-line database system that supports fast data ingestion, indexing, and query execution using a combination of:

- B+ tree based indexing
- Serial, OpenMP, and MPI execution modes
- Parallel query evaluation and parallel data scanning

The system provides a full pipeline—from data generation to query parsing to parallel execution—making it useful for system administrators who need a fast, embeddable tool for scanning large logs or structured records.

---

<!-- How to compile and run your programs (including how to generate the data) (makefile and python file) -->
## Compilation & Execution

Within this project, we have helpers for downloading dependencies, generating synthetic data, and executing the programs.

### Downloading Dependencies
To ensure that all requirements are satisfied, run the convienance `requirements.sh` file:

```bash
bash requirements.sh
```

### Generating synthetic data

Our data generation helper (`generate_commands.py`) will look at a bank of known commands to randomly generate a given amount of known data. This function takes in two parameters: a requiremented parameter of tuples to generate (we'll say 50,000) and an optimal parameter of a filename to save to.

```bash
python generate_commands.py 50000
```

### Executing the QPE Files

To execute our full tests, we can utilize predefined configs in the `makefile`.

Firstly, to **compile** all relevant .c files:

```bash
# Compile and link all relevant files
make
```

Once all files are compiled, we can use other makefile helpers to execute each version of our QPE testing functions: 

**Serial**:

```bash
# Serial version
make run-omp
```

**OpenMP Parallel Version**: 
```bash
# OpenMP Version
make run-omp
```

**OpenMPI Parallel Version**:
```bash
# OpenMPI Version
make run-mpi
```

Once testing is complete, the `make clean` command can be run to clean all artifacts and object files.

---

## File Structure

```text
project-root/
├── build/                      # Compiled binaries and test executables
├── data-generation/            # Scripts for generating synthetic datasets
├── docs/                       # Project documentation, diagrams, design notes
├── engine/                     # Core database engine implementation
│   ├── mpi/                    # MPI-specific build + execution logic
│   ├── omp/                    # OpenMP-specific build + execution logic
│   ├── serial/                 # Serial build + execution logic
│   └── bplus.c                 # B+ Tree data structure implementation
├── include/                    # Shared headers across modules
├── tests/                      # Unit test cases + verification utilities
├── tokenizer/                  # SQL parsing + tokenization logic
├── connectEngine.c             # Bridge between parser and execution engines
├── makefile                    # Build rules and compiler instructions
├── QPEMPI.c                    # Main entry for MPI execution engine
├── QPEOMP.c                    # Main entry for OpenMP execution engine
├── QPESeq.c                    # Main entry for serial execution engine
├── requirements.sh             # Environment + dependency setup script
└── sample-queries.txt          # Example queries for debugging + validation
```

---

## Report & Analysis

See **Proj2.pdf** for:

* Speedup and efficiency using Amdahl’s Law
* Scalability with increased problem size
* Optimal thread and process count for performance

## Ensuring Correctness Without Sacrificing Performance

We verified accuracy through targeted testing and edge-case checks, while profiling and optimizing critical paths to keep execution fast. This continual cycle of testing and refinement ensured the system remained both correct and efficient.

<!-- TODO Update these with finished deliverables -->
## Contributors

* *JJ McCauley*: Serial engines, makefiles/testing, docs, & QPE testing files
* *Ian Davis*:
* *Anthony Czerwinski*: Sample queries & Select parallelizations
* *Sam Dickerson*: Parser & Insert parallelizations
* *Logan Kelsch*: Data generation & 
