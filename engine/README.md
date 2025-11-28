# Engine

Core methods for building the "DB", storing the B+ Trees, and executing commands.

## Summarized Contents

The engine will contain implementations for serial, OpenMP, and OpenMPI versions.

Each directory will contain:

- **bplus**- The data structure that performs core operations and whatnot of each tree
- **buildEngine**- Builds the various trees based on defined indexes, which will be referenced by pointers in the b+ tree. 
- **executeEngine**- Interprets the query, determines which index (built tree) to use, holds the full "DB" for the trees to reference via pointers, and defines the actual functions used to be used by the main files.