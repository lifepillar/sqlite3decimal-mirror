/**
 * \file      mu_unit_sqlite.h
 * \author    Lifepillar
 * \copyright Copyright (c) 2019 Lifepillar.
 *            This program is free software; you can redistribute it and/or
 *            modify it under the terms of the Simplified BSD License (also
 *            known as the "2-Clause License" or "FreeBSD License".)
 * \copyright This program is distributed in the hope that it will be useful,
 *            but without any warranty; without even the implied warranty of
 *            merchantability or fitness for a particular purpose.
 *
 * \brief Extension of MU Unit for SQLite3 queries.
 *
 * Code adapted from `test/threadtest1.c` (see SQLite3's source code).
 */
#ifndef mu_unit_sqlite_h
#define mu_unit_sqlite_h

#include <time.h>
#include <sqlite3.h>
#include "mu_unit.h"

/**
 * \brief Used to accumulate query results by db_query().
 */
struct QueryResult {
  int nElem;          /**< Number of used entries in azElem[].     */
  int nAlloc;         /**< Number of slots allocated for azElem[]. */
  char** azElem;      /**< The result of the query.                */
};

#pragma mark Helper functions

/**
 * \brief The callback function for db_query().
 */
static int db_query_callback(
                  void* pUser,     /**< Pointer to the QueryResult structure. */
                  int nArg,        /**< Number of columns in this result row. */
                  char** azArg,    /**< Text of data in all columns.          */
                  char** NotUsed   /**< Names of the columns.                 */
) {
  (void)NotUsed;
  struct QueryResult* pResult = (struct QueryResult*)pUser;
  int i;
  if (pResult->nElem + nArg >= pResult->nAlloc) {
    if (pResult->nAlloc == 0) {
      pResult->nAlloc = nArg + 1;
    }
    else {
      pResult->nAlloc = pResult->nAlloc * 2 + nArg + 1;
    }
    pResult->azElem = realloc(pResult->azElem, pResult->nAlloc * sizeof(char*));
    if (pResult->azElem == 0) {
      MU_MSG("malloc failed\n");
      return 1;
    }
  }
  if (azArg == 0) return 0;
  for (i = 0; i < nArg; ++i) {
    pResult->azElem[pResult->nElem++] = sqlite3_mprintf("%s", azArg[i] ? azArg[i] : "");
  }
  return 0;
}

/**
 * \brief Executes a query against the database.
 *
 * NULL values are returned as an empty string. The list is terminated by
 * a single NULL pointer.
 */
static char** db_query(sqlite3* db, char const* zFormat, ...) {
  char* zSql;
  int rc;
  char* zErrMsg = 0;
  va_list ap;
  struct QueryResult sResult;
  va_start(ap, zFormat);
  zSql = sqlite3_vmprintf(zFormat, ap);
  va_end(ap);

  memset(&sResult, 0, sizeof(sResult));
  rc = sqlite3_exec(db, zSql, db_query_callback, &sResult, &zErrMsg);
  if (rc == SQLITE_SCHEMA) {
    if(zErrMsg) free(zErrMsg);
    sqlite3_exec(db, zSql, db_query_callback, &sResult, &zErrMsg);
  }
  sqlite3_free(zSql);
  if(zErrMsg) {
    sResult.azElem = malloc(2 * sizeof(char*));
    sResult.azElem[0] = sqlite3_mprintf("%s", zErrMsg);
    sResult.azElem[1] = 0;
    free(zErrMsg);
    return sResult.azElem;
  }
  if (sResult.azElem == 0) {
    db_query_callback(&sResult, 0, 0, 0);
  }
  sResult.azElem[sResult.nElem] = 0;
  return sResult.azElem;
}

/**
 * \brief Frees the results of a db_query() call.
 */
static void db_query_free(char** az){
  for (int i = 0; az[i]; ++i)
    sqlite3_free(az[i]);
  free(az);
}

/**
 * \brief Checks the result of a query.
 *
 * \return `1` on success, `0` on failure.
 */
static int db_check(char** az, ...) {
  va_list ap;
  int i;
  char* z;
  va_start(ap, az);
  for (i = 0; (z = va_arg(ap, char*)) != 0; ++i) {
    if (az[i] == 0 || strcmp(az[i], z) != 0) {
      MU_MSG("\n\n  ASSERTION %d FAILED\n", mu_assert_num);
      MU_MSG("  Expected in column %d: %s\n", i + 1, z);
      MU_MSG("                   got: %s\n\n", az[i]);
      db_query_free(az);
      return 0;
    }
  }
  va_end(ap);
  db_query_free(az);
  return 1;
}

#pragma mark Public interface

/**
 * \brief Executes an SQL statement.
 */
void mu_db_execute(sqlite3* db, char const* zFormat, ...) {
  char* zSql;
  int rc;
  char* zErrMsg = 0;
  va_list ap;
  va_start(ap, zFormat);
  zSql = sqlite3_vmprintf(zFormat, ap);
  va_end(ap);

  do {
    rc = sqlite3_exec(db, zSql, 0, 0, &zErrMsg);
  }
  while (rc == SQLITE_BUSY);

  if (zErrMsg) {
    MU_MSG("Command failed: %s - %s\n", zSql, zErrMsg);
    free(zErrMsg);
    sqlite3_free(zSql);
    exit(1);
  }
  sqlite3_free(zSql);
}

/**
 * \brief Checks whether a query returns the expected result.
 *
 * \param db The database connection
 * \param query A string containing a SQL query
 * \param ... The values of the first record in the expected result
 *
 * \note The given query should return a single record; if the query returns
 *       multiple records, only the first record is tested.
 */
#define mu_assert_query(db, query, ...)                          \
  do {                                                           \
    ++mu_assert_num;                                             \
    int success = db_check(db_query(db, query), __VA_ARGS__, 0); \
    if (!success)                                                \
      return 1;                                                  \
  } while (0)

/**
 * \brief Measures the time taken to execute a query \a n times.
 *
 * \param db The database connection
 * \param query A string containing a SQL query
 * \param n The number of times the query should be executed
 */
#define mu_benchmark_query(db, query, n)                    \
  do {                                                      \
    ++mu_assert_num;                                        \
    char* zErrMsg = 0;                                      \
    float startTime = (float)clock() / CLOCKS_PER_SEC;      \
    for (unsigned int i = 0; i < n; i++) {                  \
      int rc = sqlite3_exec(db, query, 0, 0, &zErrMsg);     \
      if (rc != SQLITE_OK) {                                \
        MU_MSG("\n\n  QUERY BENCHMARK FAILED\n");           \
        MU_MSG("  %s\n\n", zErrMsg);                        \
        sqlite3_free(zErrMsg);                              \
        return 1;                                           \
      }                                                     \
    }                                                       \
    float endTime = (float)clock() / CLOCKS_PER_SEC;        \
    MU_MSG("%.1fms", 1000.0 * (endTime - startTime) / n);   \
  } while (0)


/**
 * \brief Checks that a query fails with a given error message.
 *
 * \param db The database connection
 * \param query A string containing a SQL query
 * \param errmsg The expected error message
 */
#define mu_assert_query_fails(db, query, errmsg)                                  \
  do {                                                                            \
    ++mu_assert_num;                                                              \
    char** azElem = db_query(db, query);                                          \
    int fail = (strcmp(azElem[0], errmsg) != 0);                                  \
    if (fail) {                                                                   \
      MU_MSG("\n\n  ASSERTION %d FAILED\n", mu_assert_num);                       \
      MU_MSG("  Expected error: %s\n", errmsg);                                   \
      MU_MSG("             got: %s\n\n", azElem[0] == 0 ? "nothing" : azElem[0]); \
      db_query_free(azElem);                                                      \
      return 1;                                                                   \
    }                                                                             \
    db_query_free(azElem);                                                        \
  } while (0)

#endif /* mu_unit_sqlite_h */

