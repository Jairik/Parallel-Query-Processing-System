# Parallel Insert Implementation Walkthrough

I have implemented parallel versions of the `executeQueryInsertSerial` function for both OpenMP and MPI execution engines.

## Changes

### Makefile
- Updated `makefile` to include build targets for `QPEOMP` and `QPEMPI`.
- Linked `QPEOMP` with `engine/omp/executeEngine-omp.c`.
- Linked `QPEMPI` with `engine/mpi/executeEngine-mpi.c`.

### OpenMP Implementation
- **File**: `engine/omp/executeEngine-omp.c`
- **Function**: `executeQueryInsertSerial`
- **Parallelization**: Used `#pragma omp parallel for` to parallelize the B+ tree index updates. Each index is updated in a separate thread, as they are independent.
- **Verification**: Successfully built `QPEOMP` and ran sample queries. The output confirms correct insertion and query execution.

### MPI Implementation
- **File**: `engine/mpi/executeEngine-mpi.c`
- **Function**: `executeQueryInsertSerial`
- **Parallelization**: Implemented a distributed index update strategy.
    - **Rank 0**: Handles file I/O (appending to CSV) to ensure data integrity.
    - **All Ranks**: Update their local in-memory record list.
    - **Distributed Indexing**: Indexes are distributed among ranks using `i % size == rank`. Each rank updates only its assigned indexes.
- **Verification**: The code is implemented, but verification was skipped because `mpicc` was not found in the current environment.

## How to Run

### OpenMP
```bash
make QPEOMP
./QPEOMP
```

### MPI
Ensure `mpicc` is in your PATH.
```bash
make QPEMPI
mpirun -np 4 ./QPEMPI
```

> [!NOTE]
> If `make all` fails due to missing `mpicc`, you can still build the OpenMP version using `make QPEOMP`.
