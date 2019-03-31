#include "decNumber/decQuad.h"

static sqlite3* db;

#pragma mark Function tests

static int
sqlite_decquad_test_dec_blob_with_wrong_size(void) {
  mu_db_execute(db, "drop table if exists T;");
  mu_db_execute(db, "create table T(n int, d blob);");
  char cmd[] = "insert into T(n,d) values (?1,?2)";
  sqlite3_stmt* stmt;
  sqlite3_prepare_v2(db, cmd, (int)strlen(cmd), &stmt, 0);
  for (int i = -1; i < 2; i++) {
    sqlite3_bind_int(stmt, 1, i + 2);
    sqlite3_bind_zeroblob(stmt, 2, DECQUAD_Bytes + i);
    int retVal = sqlite3_step(stmt);
    if (retVal != SQLITE_DONE)
      fprintf(stderr, "Commit Failed! %d\n", retVal);
    sqlite3_reset(stmt);
  }
  sqlite3_finalize(stmt);

  mu_assert_query_fails(db, "select decStr(d) from T where n=1", "Conversion syntax");
  mu_assert_query(db, "select decStr(d) from T where n=2", "0E-6176");
  mu_assert_query_fails(db, "select decStr(d) from T where n=3", "Conversion syntax");
  return 0;
}

static int
sqlite_decquad_test_decbits(void) {
  for (int i = 0; i < 2; i++) { // The result must be deterministic
    mu_assert_query(db, "select decBits(dec('-1.00'));",
                    "1 01000 100000011110 "
                    "0000000000 0000000000 0000000000 0000000000 "
                    "0000000000 0000000000 0000000000 0000000000 "
                    "0000000000 0000000000 0010000000");
  }
  return 0;
}

static int
sqlite_decquad_test_decbytes(void) {
  mu_assert_query(db, "select decBytes(dec('-1.00'))",
                  "a2 07 80 00 00 00 00 00 00 00 00 00 00 00 00 80");
  return 0;
}

static int
sqlite_decquad_test_decdivideinteger_impossible(void) {
  mu_assert_query_fails(db, "select decStr(decDivInt('1.0', '1e-34'))", "Division impossible");
  mu_assert_query(db, "select decStr(decDivInt('1.0', '1e-1'))", "10");
  mu_assert_query_fails(db, "select decStr(decDivInt('0.1', '1e-35'))", "Division impossible");
  return 0;
}

static int
sqlite_decquad_test_decexp_not_implemented(void) {
  mu_assert_query_fails(db, "select decExp('1')", "Operation not implemented");
  return 0;
}

static int
sqlite_decquad_test_decgetcoefficient(void) {
  mu_assert_query(db, "select decGetCoeff(dec('12.345'))", "0000000000000000000000000000012345");
  mu_assert_query(db, "select decGetCoeff(dec('-12.345'))", "0000000000000000000000000000012345");
  return 0;
}

static int
sqlite_decquad_test_decgreatest(void) {
  mu_assert_query(db, "select decStr(decGreatest())", "9.999999999999999999999999999999999E+6144");
  return 0;
}

static int
sqlite_decquad_test_decinvert(void) {
  mu_assert_query(db, "select decStr(decInvert('1011000'))", "1111111111111111111111111110100111");
  return 0;
}

static int
sqlite_decquad_test_decisfinite(void) {
  mu_assert_query(db, "select decIsFinite('1e6144')", "1");
  mu_assert_query(db, "select decIsFinite('1e6145')", "0");
  mu_assert_query(db, "select decIsFinite('-1e6144')", "1");
  mu_assert_query(db, "select decIsFinite('-1e6145')", "0");
  return 0;
}

static int
sqlite_decquad_test_decisinfinite(void) {
  mu_assert_query(db, "select decIsInf('1e6144')", "0");
  mu_assert_query(db, "select decIsInf('1e6145')", "1");
  mu_assert_query(db, "select decIsInf('-1e6144')", "0");
  mu_assert_query(db, "select decIsInf('-1e6145')", "1");
  return 0;
}

static int
sqlite_decquad_test_decisnormal(void) {
  mu_assert_query(db, "select decIsNormal('1e-6143')", "1");
  mu_assert_query(db, "select decIsNormal('1e-6144')", "0");
  mu_assert_query(db, "select decIsNormal('1e6144')", "1");
  mu_assert_query(db, "select decIsNormal('1e6145')", "0");
  return 0;
}

static int
sqlite_decquad_test_decissubnormal(void) {
  mu_assert_query(db, "select decIsSubnormal('1e-6143')", "0");
  mu_assert_query(db, "select decIsSubnormal('1e-6144')", "1");
  mu_assert_query(db, "select decIsSubnormal('1e6144')", "0");
  mu_assert_query(db, "select decIsSubnormal('1e6145')", "0"); // This is Inf
  return 0;
}

static int
sqlite_decquad_test_deciszero(void) {
  mu_assert_query(db, "select decIsZero('1e-6143')", "0");
  mu_assert_query(db, "select decIsZero('1e-6144')", "0");
  mu_assert_query(db, "select decIsZero('1e-6176')", "0");
  mu_assert_query(db, "select decIsZero('-1e-6176')", "0");
  mu_assert_query(db, "select decIsZero('1e-6177')", "1");
  mu_assert_query(db, "select decIsZero('-1e-6177')", "1");
  mu_assert_query(db, "select decIsZero('1e6144')", "0");
  mu_assert_query(db, "select decIsZero('1e6145')", "0");
  return 0;
}

static int
sqlite_decquad_test_decleast(void) {
  mu_assert_query(db, "select decStr(decLeast())", "-9.999999999999999999999999999999999E+6144");
  return 0;
}

static int
sqlite_decquad_test_decln_not_implemented(void) {
  mu_assert_query_fails(db, "select decLn('1')", "Operation not implemented");
  return 0;
}

static int
sqlite_decquad_test_decmaxmag(void) {
  mu_assert_query(db, "select decStr(decMaxMag())", "9.999999999999999999999999999999999E+6144");
  return 0;
}

static int
sqlite_decquad_test_decmax_null(void) {
  mu_assert_query(db, "select decStr(decMax(null))", "9.999999999999999999999999999999999E+6144");
  return 0;
}

static int
sqlite_decquad_test_decminmag(void) {
  mu_assert_query(db, "select decStr(decMinMag())", "-9.999999999999999999999999999999999E+6144");
  return 0;
}

static int
sqlite_decquad_test_decmin_null(void) {
  mu_assert_query(db, "select decStr(decMin(null))", "-9.999999999999999999999999999999999E+6144");
  return 0;
}

static int
sqlite_decquad_test_decnextdown(void) {
  mu_assert_query(db, "select decStr(decNextDown('1.23'))", "1.229999999999999999999999999999999");
  return 0;
}

static int
sqlite_decquad_test_decnextup(void) {
  mu_assert_query(db, "select decStr(decNextUp('1.23'))", "1.230000000000000000000000000000001");
  return 0;
}

static int
sqlite_decquad_test_decpower_not_implemented(void) {
  mu_assert_query_fails(db, "select decPow('10', '3')", "Operation not implemented");
  return 0;
}

static int
sqlite_decquad_test_decremainder(void) {
  mu_assert_query_fails(db, "select decStr(decRemainder('1.0', '1e-34'))", "Division impossible");
  return 0;
}

static int
sqlite_decquad_test_decrotate(void) {
  // Digits are not discarded, they... rotate:
  mu_assert_query(db, "select decStr(decRotate('1.2345', '-2'))", "450000000000000000000000000000.0123");
  return 0;
}

static int
sqlite_decquad_test_decsqrt_not_implemented(void) {
  mu_assert_query_fails(db, "select decSqrt('10')", "Operation not implemented");
  return 0;
}

static int
sqlite_decquad_test_dectoint64_not_implemented(void) {
  mu_assert_query_fails(db, "select decToInt64('10.1000')", "Operation not implemented");
  return 0;
}

static int
sqlite_decquad_test_dectrim_not_implemented(void) {
  mu_assert_query_fails(db, "select decTrim('10.1000')", "Operation not implemented");
  return 0;
}


#pragma mark Vtable tests

static int
sqlite_decquad_test_context_max_exp(void) {
  mu_assert_query(db, "select emax from decContext", "6144");
  return 0;
}

static int
sqlite_decquad_test_context_min_exp(void) {
  mu_assert_query(db, "select emin from decContext", "-6143");
  return 0;
}

static int
sqlite_decquad_test_context_precision(void) {
  mu_assert_query(db, "select prec from decContext", "34");
  return 0;
}

static int
sqlite_decquad_test_context_rounding_default(void) {
  mu_assert_query(db, "select round from decContext", "ROUND_HALF_EVEN");
  return 0;
}

static int
sqlite_decquad_test_status(void) {
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
sqlite_decquad_test_traps_division_impossible_flag(void) {
  mu_assert_query(db, "select flag from decTraps where flag = 'Division impossible'", "Division impossible");
  mu_assert_query_fails(db, "select decstr(decdivint('1e6143','0.0000001'))", "Division impossible");
  mu_assert_query(db, "select count(*) from decStatus", "0");
  return 0;
}

#pragma mark Test runner

static void
sqlite_decquad_func_tests(void) {
  mu_setup = sqlite_test_context_setup;
  mu_teardown = mu_noop;

  mu_test(sqlite_decquad_test_dec_blob_with_wrong_size);
  mu_test(sqlite_decquad_test_decbits);
  mu_test(sqlite_decquad_test_decbytes);
  mu_test(sqlite_decquad_test_decgetcoefficient);
  mu_test(sqlite_decquad_test_decdivideinteger_impossible);
  mu_test(sqlite_decquad_test_decexp_not_implemented);
  mu_test(sqlite_decquad_test_decgreatest);
  mu_test(sqlite_decquad_test_decinvert);
  mu_test(sqlite_decquad_test_decisfinite);
  mu_test(sqlite_decquad_test_decisinfinite);
  mu_test(sqlite_decquad_test_decisnormal);
  mu_test(sqlite_decquad_test_decissubnormal);
  mu_test(sqlite_decquad_test_deciszero);
  mu_test(sqlite_decquad_test_decleast);
  mu_test(sqlite_decquad_test_decln_not_implemented);
  mu_test(sqlite_decquad_test_decmaxmag);
  mu_test(sqlite_decquad_test_decmax_null);
  mu_test(sqlite_decquad_test_decminmag);
  mu_test(sqlite_decquad_test_decmin_null);
  mu_test(sqlite_decquad_test_decnextdown);
  mu_test(sqlite_decquad_test_decnextup);
  mu_test(sqlite_decquad_test_decpower_not_implemented);
  mu_test(sqlite_decquad_test_decremainder);
  mu_test(sqlite_decquad_test_decrotate);
  mu_test(sqlite_decquad_test_decsqrt_not_implemented);
  mu_test(sqlite_decquad_test_dectoint64_not_implemented);
  mu_test(sqlite_decquad_test_dectrim_not_implemented);
}

static void
sqlite_decquad_vtab_tests(void) {
  mu_setup = sqlite_test_context_setup;
  mu_teardown = mu_noop;

  mu_test(sqlite_decquad_test_context_max_exp);
  mu_test(sqlite_decquad_test_context_min_exp);
  mu_test(sqlite_decquad_test_context_precision);
  mu_test(sqlite_decquad_test_context_rounding_default);
  mu_test(sqlite_decquad_test_status);
  mu_test(sqlite_decquad_test_traps_division_impossible_flag);
}
