# Parallel Database Query Processing System

## Overview

This project implements a parallel query-processing engine designed to run SQL-like queries over large, structured datasets using high-performance computing techniques. The goal is to build a lightweight, command-line database system that supports fast data ingestion, indexing, and query execution using a combination of:

- LSM-Tree based storage
- Secondary indexes
- Serial, OpenMP, and MPI execution modes
- Parallel query evaluation and parallel data scanning

The system provides a full pipeline—from data generation to query parsing to parallel execution—making it useful for system administrators who need a fast, embeddable tool for scanning large logs or structured records.

<!-- Should modify this later
## Expected Components

* **`QPESeq.c`** — Serial query processing engine.
* **`QPEOMP.c`** — Parallel version using **OpenMP**.
* **`QPEMPI.c`** — Parallel version using **MPI**.
* **`Proj2.pdf`** — Documentation and runtime analysis.
* **`db.txt`** — Sample generated dataset.
* **`sample-queries.txt`** — Sample SQL-like queries.

-->
## Compilation & Execution

```bash
# Serial execution
gcc QPESeq.c -o QPESeq
./QPESeq db.txt sql.txt

# OpenMP version
gcc -fopenmp QPEOMP.c -o QPEOMP
./QPEOMP db.txt sql.txt

# MPI version
mpicc QPEMPI.c -o QPEMPI
mpirun -np <num_processes> ./QPEMPI db.txt sql.txt
```

<!--
## Report & Analysis

See **Proj2.pdf** for:

* Speedup and efficiency using Amdahl’s Law
* Scalability with increased problem size
* Optimal thread and process count for performance

## Contributors

* *Name A*: Data generation & serial QPE
* *Name B*: OpenMP implementation
* *Name C*: MPI implementation & runtime analysis
-->
---

