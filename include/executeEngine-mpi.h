/* MPI Execute Engine */

#ifndef EXECUTE_ENGINE_MPI_H
#define EXECUTE_ENGINE_MPI_H

#include "executeEngine-serial.h"

struct resultSetS *executeQuerySelectMPI(
    struct engineS *engine,
    const char **selectItems,
    int numSelectItems,
    const char *tableName,
    struct whereClauseS *whereClause
);

bool executeQueryInsertMPI(
    struct engineS *engine,
    const char *tableName,
    const record *r
);

struct resultSetS *executeQueryUpdateMPI(
    struct engineS *engine,
    const char *tableName,
    const char *(*setItems)[2],
    int numSetItems,
    struct whereClauseS *whereClause
);

struct resultSetS *executeQueryDeleteMPI(
    struct engineS *engine,
    const char *tableName,
    struct whereClauseS *whereClause
);

struct engineS *initializeEngineMPI(
    int num_indexes,
    const char *indexed_attributes[],
    const int attribute_types[],
    const char *datafile,
    const char *tableName
);

void destroyEngineMPI(struct engineS *engine);

bool addAttributeIndexMPI(
    struct engineS *engine,
    const char *tableName,
    const char *attributeName,
    int attributeType
);

#endif // EXECUTE_ENGINE_MPI_H
