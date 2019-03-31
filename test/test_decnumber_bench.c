static sqlite3* db;

static int sqlite3_decnumber_test_numsum_benchmark(void) {
  mu_db_execute(db, "drop table if exists T;");
  mu_db_execute(db, "create table T(n int, d blob);");
  char cmd[] = "insert into T(n,d) values (?1,num(?2))";
  sqlite3_stmt* stmt;
  sqlite3_prepare_v2(db, cmd, (int)strlen(cmd), &stmt, 0);
  sqlite3_bind_text(stmt, 2, "0.000001", -1, SQLITE_STATIC);
  for (int i = 0; i < 1000000; i++) {
    sqlite3_bind_int(stmt, 1, i);
    int retVal = sqlite3_step(stmt);
    if (retVal != SQLITE_DONE)
      fprintf(stderr, "Commit Failed! %d\n", retVal);
    sqlite3_reset(stmt);
  }
  sqlite3_finalize(stmt);
  mu_benchmark_query(db, "select numStr(numSum(d)) from T", 10);
  mu_assert_query(db, "select max(n) from T", "999999");
  mu_assert_query(db, "select numStr(numSum(d)) from T", "1.000000");
  mu_assert_query(db, "select numStr(numMax(d)) from T", "0.000001");
  mu_assert_query(db, "select numStr(numMin(d)) from T", "0.000001");
  mu_assert_query(db, "select numStr(numAvg(d)) from T", "0.000001");
  return 0;
}

static void
sqlite_decnumber_bench_tests() {
  mu_setup = mu_noop;
  mu_teardown = mu_noop;

  mu_test(sqlite3_decnumber_test_numsum_benchmark);
}

