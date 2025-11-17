# Parallel Database Query Processing System

## Overview

This project implements a simplified database query processing engine using **C** with serial and parallel (OpenMP and MPI) versions. The system queries a large dataset (`db.txt`) representing a **TODO**, supporting SQL-like queries for selection and filtering.

## Expected Components

* **`QPESeq.c`** — Serial query processing engine.
* **`QPEOMP.c`** — Parallel version using **OpenMP**.
* **`QPEMPI.c`** — Parallel version using **MPI**.
* **`Proj2.pdf`** — Documentation and runtime analysis.
* **`db.txt`** — Sample generated dataset.
* **`sample-queries.txt`** — Sample SQL-like queries.

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

