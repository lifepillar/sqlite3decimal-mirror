static sqlite3* db;

#pragma mark Vtable tests

static int
sqlite3_decquad_test_context_max_exp(void) {
  mu_assert_query(db, "select emax from decContext", "6144");
  return 0;
}

static int
sqlite3_decquad_test_context_min_exp(void) {
  mu_assert_query(db, "select emin from decContext", "-6143");
  return 0;
}

static int
sqlite3_decquad_test_context_precision(void) {
  mu_assert_query(db, "select prec from decContext", "34");
  return 0;
}

static int
sqlite3_decquad_test_context_rounding_default(void) {
  mu_assert_query(db, "select round from decContext", "ROUND_HALF_EVEN");
  return 0;
}

static int
sqlite3_decquad_test_status(void) {
  mu_assert_query(db, "select decStatus()", "00000000");

  mu_db_execute(db, "insert into decStatus values ('Clamped')");
  mu_db_execute(db, "insert into decStatus values ('Conversion syntax')");
  mu_db_execute(db, "insert into decStatus values ('Division by zero')");
  mu_db_execute(db, "insert into decStatus values ('Division impossible')");
  mu_db_execute(db, "insert into decStatus values ('Division undefined')");
  mu_db_execute(db, "insert into decStatus values ('Inexact result')");
  mu_db_execute(db, "insert into decStatus values ('Out of memory')");
  mu_db_execute(db, "insert into decStatus values ('Invalid context')");
  mu_db_execute(db, "insert into decStatus values ('Invalid operation')");
  mu_db_execute(db, "insert into decStatus values ('Overflow')");
  mu_db_execute(db, "insert into decStatus values ('Rounded result')");
  mu_db_execute(db, "insert into decStatus values ('Subnormal')");
  mu_db_execute(db, "insert into decStatus values ('Underflow')");
  mu_assert_query(db, "select count(*) from decStatus", "13");
  mu_assert_query(db, "select decStatus()", "00003eff"); // 0011 1110 1111 1111

  mu_db_execute(db, "delete from decStatus where flag = 'Overflow'");
  mu_db_execute(db, "update decStatus set flag = 'Overflow' where flag = 'Out of memory'");
  mu_db_execute(db, "update decStatus set flag = 'Overflow' where flag = 'Subnormal'");
  mu_assert_query(db, "select count(*) from decStatus", "11");
  mu_assert_query(db, "select decStatus()", "00002eef"); // 0010 1110 1110 1111

  return 0;
}

static int
sqlite3_decquad_test_traps_division_impossible_flag(void) {
  mu_assert_query(db, "select flag from decTraps where flag = 'Division impossible'", "Division impossible");
  mu_assert_query_fails(db, "select decstr(decdivint('1e6143','0.0000001'))", "Division impossible");
  mu_assert_query(db, "select count(*) from decStatus", "0");
  return 0;
}

#pragma mark Test runner

static void
sqlite3_test_context_setup() {
  // Clear status
  mu_db_execute(db, "delete from decStatus");
  // Default traps
  mu_db_execute(db, "delete from decTraps");
  mu_db_execute(db, "insert into decTraps values "
                "('Conversion syntax'),"
                "('Division by zero'),"
                "('Division undefined'),"
                "('Division impossible'),"
                "('Out of memory'),"
                "('Invalid context'),"
                "('Invalid operation')"
                );
}

static void
sqlite3_decquad_vtab_tests(void) {
  mu_setup = sqlite3_test_context_setup;

  mu_test(sqlite3_decquad_test_context_delete_fails);
  mu_test(sqlite3_decquad_test_context_insert_fails);
  mu_test(sqlite3_decquad_test_context_max_exp);
  mu_test(sqlite3_decquad_test_context_min_exp);
  mu_test(sqlite3_decquad_test_context_precision);
  mu_test(sqlite3_decquad_test_context_rounding_default);
  mu_test(sqlite3_decquad_test_context_update_set_null_fails);
  mu_test(sqlite3_decquad_test_context_set_rounding);
  mu_test(sqlite3_decquad_test_status);
  mu_test(sqlite3_decquad_test_status_inexact_flag);
  mu_test(sqlite3_decquad_test_status_insert_null_fails);
  mu_test(sqlite3_decquad_test_traps_default);
  mu_test(sqlite3_decquad_test_traps_division_impossible_flag);
  mu_test(sqlite3_decquad_test_traps_insert_null_fails);
}
