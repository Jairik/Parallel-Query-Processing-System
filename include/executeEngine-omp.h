/* OMP Execute Engine */

#ifndef EXECUTE_ENGINE_OMP_H
#define EXECUTE_ENGINE_OMP_H

#include "executeEngine-serial.h"

struct resultSetS *executeQuerySelectOMP(
    struct engineS *engine,
    const char **selectItems,
    int numSelectItems,
    const char *tableName,
    struct whereClauseS *whereClause
);

bool executeQueryInsertOMP(
    struct engineS *engine,
    const char *tableName,
    const record *r
);

struct resultSetS *executeQueryUpdateOMP(
    struct engineS *engine,
    const char *tableName,
    const char *(*setItems)[2],
    int numSetItems,
    struct whereClauseS *whereClause
);

struct resultSetS *executeQueryDeleteOMP(
    struct engineS *engine,
    const char *tableName,
    struct whereClauseS *whereClause
);

struct engineS *initializeEngineOMP(
    int num_indexes,
    const char *indexed_attributes[],
    const int attribute_types[],
    const char *datafile,
    const char *tableName
);

void destroyEngineOMP(struct engineS *engine);

bool addAttributeIndexOMP(
    struct engineS *engine,
    const char *tableName,
    const char *attributeName,
    int attributeType
);

#endif // EXECUTE_ENGINE_OMP_H
