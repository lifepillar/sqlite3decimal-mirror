/**
 * \file      runtests.c
 * \author    Lifepillar
 * \copyright Copyright (c) 2019 Lifepillar.
 *            This program is free software; you can redistribute it and/or
 *            modify it under the terms of the Simplified BSD License (also
 *            known as the "2-Clause License" or "FreeBSD License".)
 * \copyright This program is distributed in the hope that it will be useful,
 *            but without any warranty; without even the implied warranty of
 *            merchantability or fitness for a particular purpose.
 *
 * \brief Test suite for the SQLite3 Decimal extension.
 */
#include "mu_unit_sqlite.h"
#include "test_common.c"
#include "test_decquad.c"
#include "test_decquad_bench.c"

static sqlite3* db;

#pragma mark Test Runner

static int sqlite_decimal_test_init() {
  int rc = sqlite3_open(":memory:", &db);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return rc;
  }

  // sqlite3_enable_load_extension(db, 1);
  if (sqlite3_db_config(db, SQLITE_DBCONFIG_ENABLE_LOAD_EXTENSION, 1, NULL) != SQLITE_OK) {
    fprintf(stderr, "Can't enable extensions: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return rc;
  }


  // Initialize decimal extension
  char* zErrMsg;
  rc = sqlite3_load_extension(db, "./libsqlite3decimal", "sqlite3_decimal_init", &zErrMsg);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "[Decimal] %s\n", zErrMsg);
  }
  return rc;
}

static void sqlite_decimal_test_finish() {
  sqlite3_close(db);
}

int main(void) {
  int rc = sqlite_decimal_test_init();

  if (rc != SQLITE_OK) return EXIT_FAILURE;

  mu_init();
  mu_run_test_suite(sqlite_decimal_func_tests);
  mu_run_test_suite(sqlite_decimal_vtab_tests);
  mu_run_test_suite(sqlite_decquad_func_tests);
  mu_run_test_suite(sqlite_decquad_vtab_tests);
#ifdef SQLITE3_DECIMAL_BENCHMARKS
  mu_run_test_suite(sqlite3_decquad_bench_tests);
#else
  (void)sqlite3_decquad_bench_tests; // Silence warnings
#endif
  mu_test_summary();
  sqlite_decimal_test_finish();

  return (mu_tests_failed > 0);
}

