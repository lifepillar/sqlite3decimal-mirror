static sqlite3* db;

#pragma mark Tests

static int
sqlite3_decnumber_test_big_number() {
  mu_assert_query(db, "select decStr(dec('1234567890123456789012345678901234567890'))", "1234567890123456789012345678901234567890");
  return 0;
}

static int
sqlite3_decnumber_test_version(void) {
  mu_assert_query(db, "select decVersion();", "Decimal v0.0.1 (decNumber 3.68)");
  return 0;
}

#pragma mark Test runner

static void sqlite_decnumber_func_tests(void) {
  mu_setup = mu_noop;
  mu_teardown = mu_noop;

  mu_test(sqlite3_decnumber_test_big_number);
  mu_test(sqlite3_decnumber_test_version);
}
