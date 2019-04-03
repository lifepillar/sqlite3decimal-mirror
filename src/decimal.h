#ifndef sqlite_decimal_h
#define sqlite_decimal_h

#include "version.h"

/**
 * \brief SQLite Decimal long version string.
 */
#define SQLITE_DECIMAL_VERSION "Decimal v" SQLITE_DECIMAL_SHORT_VERSION

#ifdef __cplusplus
extern "C" {
#endif

#include "sqlite3.h"

int sqlite3_decimal_init(sqlite3* db, char** pzErrMsg, sqlite3_api_routines const* pApi);

#ifdef __cplusplus
}
#endif

#endif /* sqlite_decimal_h */

