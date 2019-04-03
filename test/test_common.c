/**
 * \file test_common.c
 * \brief Test suite for the decInfinite implementation of SQLite3 Decimal.
 *
 * Note that this implementation does not conform to SQL standard for decimals.
 * Besides, despite using the decNumber library, it does not conforms to IEEE
 * 754 decimal arithmetic. The two key differences (currently) are as follows:
 *
 * - decInfinite has only one NaN, which is greater than any other value in the
 *   total ordering of decimals (IEEE 754 distinguishes between -NaN and +NaN);
 * - decNumber and IEEE 754 use different internal representations for numbers
 *   that are mathematically equal (for instance, 1.0 and 1.00 have different
 *   representations). decInfinite uses a unique (and totally ordered) internal
 *   representation, so that two decimals are mathematically equal if and only
 *   if their encodings are bit by bit equal.
 */
static sqlite3* db;

#pragma mark Test functions

static int sqlite_decimal_test_decbytes(void) {
  mu_assert_query(db, "select decBytes(dec('-1.00'))", "0f 84");
  mu_assert_query(db, "select decBytes(dec('-Inf'))", "00");
  mu_assert_query(db, "select decBytes(dec('+Inf'))", "c0");
  mu_assert_query(db, "select decBytes(dec('-0'))", "40");
  mu_assert_query(db, "select decBytes(dec('0'))", "80");
  mu_assert_query(db, "select decBytes(dec('-NaN'))", "e0");
  mu_assert_query(db, "select decBytes(dec('NaN'))", "e0");
  mu_assert_query( db,
      "select decBytes(dec('-0.001213872189473241234987231984754353498798734892374289399999999999'))",
      "19 db a6 4c 34 34 a8 f6 c7 d5 14 c8 60 c8 d2 8a 1e 00"
      );
  return 0;
}

static int sqlite_decimal_test_decbits(void) {
  mu_assert_query(db, "select decBits(dec('-1.23'));", "00 0 011 1101101101");
  return 0;
}

static int sqlite_decimal_test_dec_NaN(void) {
  mu_assert_query(db, "select decStr(dec('NaN'))", "NaN");
  mu_assert_query(db, "select decStr(dec('nan'))", "NaN");
  mu_assert_query(db, "select decStr(dec('NAN'))", "NaN");
  // decInfinite has only one NaN
  mu_assert_query(db, "select decStr(dec('-NaN'))", "NaN");
  mu_assert_query(db, "select decStr(dec('-nan'))", "NaN");
  mu_assert_query(db, "select decStr(dec('-NAN'))", "NaN");
  return 0;
}

static int sqlite_decimal_test_dec_Inf(void) {
  mu_assert_query(db, "select decStr(dec('Inf'))", "Infinity");
  mu_assert_query(db, "select decStr(dec('+Inf'))", "Infinity");
  mu_assert_query(db, "select decStr(dec('-Inf'))", "-Infinity");
  return 0;
}

static int sqlite_decimal_test_dec_neg(void) {
  mu_assert_query(db, "select decStr(dec('-12.345'))", "-12.345");
  return 0;
}

static int sqlite_decimal_test_dec_parse_error(void) {
  mu_assert_query(db, "select count(*) from decTraps where flag = 'Conversion syntax'", "1");
  mu_assert_query_fails(db, "select dec('asdkf')", "Conversion syntax");
  mu_assert_query(db, "select count(*) from decStatus where flag = 'Conversion syntax'", "0");
  mu_db_execute(db, "delete from decTraps where flag = 'Conversion syntax'");
  mu_assert_query(db, "select decStr(dec('asdkf'))", "NaN");
  mu_assert_query(db, "select count(*) from decStatus where flag = 'Conversion syntax'", "1");
  mu_db_execute(db, "delete from decStatus");
  return 0;
}

static int sqlite_decimal_test_decabs(void) {
  mu_assert_query(db, "select decStr(decAbs(dec('-12.345')))", "12.345");
  mu_assert_query(db, "select decStr(decAbs(dec('+12.345')))", "12.345");
  mu_assert_query(db, "select decStr(decAbs(dec('0.00')))", "0");
  mu_assert_query(db, "select decStr(decAbs(dec('-0.00')))", "0");
  return 0;
}

static int sqlite_decimal_test_decabs_null(void) {
  mu_assert_query(db, "select decStr(decAbs(null)) is null", "1");
  return 0;
}

static int sqlite_decimal_test_decadd_two_numbers(void) {
  mu_db_execute(db, "drop table if exists t;");
  mu_db_execute(db, "create table t(n blob);");
  mu_db_execute(db, "insert into t(n) values (dec('1.23'));");
  mu_assert_query(db, "select decStr(decAdd(t.n, t.n)) from t;", "2.46");
  mu_db_execute(db, "drop table t;");
  return 0;
}

static int sqlite_decimal_test_decadd_big_and_small_number(void) {
  mu_db_execute(db, "drop table if exists t;");
  mu_db_execute(db, "create table t(m blob, n blob);");
  mu_db_execute(db, "insert into t(m,n) values (dec('92233720368547758.07'), dec('0.01'));");
  mu_assert_query(db, "select decStr(decAdd(t.m, t.n)) from t;", "92233720368547758.08");
  mu_db_execute(db, "drop table t;");
  return 0;
}

static int sqlite_decimal_test_decadd_text_args(void) {
  mu_assert_query(db, "select decStr(decAdd('-0.0001', '1.9990', '1234567890.00005'))", "1234567891.99895");
  return 0;
}

static int sqlite_decimal_test_decadd_mixed_args(void) {
  mu_assert_query(db, "select decStr(decAdd('-0.0001', dec('1.9990'), '1234567890.00005'))", "1234567891.99895");
  return 0;
}


static int sqlite_decimal_test_decadd_empty_arg(void) {
  mu_assert_query(db, "select decStr(decAdd())", "0");
  return 0;
}

static int sqlite_decimal_test_decadd_null(void) {
  mu_assert_query(db, "select decStr(decAdd(null)) is null", "1");
  return 0;
}

static int sqlite_decimal_test_decadd_nan(void) {
  mu_assert_query(db, "select decStr(decAdd('NaN'))", "NaN");
  mu_assert_query(db, "select decStr(decAdd('NaN', 'NaN'))", "NaN");
  mu_assert_query(db, "select decStr(decAdd('1.0', 'NaN'))", "NaN");
  mu_assert_query(db, "select decStr(decAdd('NaN', '1.0'))", "NaN");
  mu_assert_query(db, "select decStr(decAdd('1.0', '2.0', '3.0', 'NaN'))", "NaN");
  return 0;
}

static int sqlite_decimal_test_decand(void) {
  mu_assert_query(db, "select decStr(decAnd('101', '110'))", "100");
  // decAnd() works with sequences of different lengths. The result has the
  // length of the shortest one (minus leading zeroes).
  mu_assert_query(db, "select decStr(decAnd('1010110110', '011'))", "10");
  mu_assert_query_fails(db, "select decAnd('1001', '012')", "Invalid operation");
  return 0;
}

static int sqlite_decimal_test_decavg_null(void) {
  // Nulls are ignored for the purpose of aggregation, so
  // the result is the same as the average of an empty set of values.
  mu_assert_query(db, "select decStr(decAvg(null))", "NaN");
  return 0;
}

static int sqlite_decimal_test_decclass(void) {
  mu_assert_query(db, "select decClass('-Inf')", "-Infinity");
  mu_assert_query(db, "select decClass('+Inf')", "+Infinity");
  mu_assert_query(db, "select decClass('NaN')", "NaN");
  mu_assert_query(db, "select decClass('-NaN')", "NaN");
  mu_assert_query(db, "select decClass('1.0')", "+Normal");
  mu_assert_query(db, "select decClass('-1.0')", "-Normal");
  mu_assert_query(db, "select decClass('0.0')", "+Zero");
  mu_assert_query(db, "select decClass('-0.0')", "-Zero");
  return 0;
}

static int sqlite_decimal_test_deccompare(void) {
  mu_assert_query(db, "select decCompare('1', '1.0')", "0");
  mu_assert_query(db, "select decCompare('1.0', '1')", "0");
  mu_assert_query(db, "select decCompare('-1.0', '1')", "-1");
  mu_assert_query(db, "select decCompare('1.0', '-1')", "1");
  mu_assert_query(db, "select decCompare('1.0', 'Inf')", "-1");
  mu_assert_query(db, "select decCompare('1.0', '-Inf')", "1");
  mu_assert_query_fails(db, "select decCompare('NaN', '1')", "Invalid operation");
  mu_assert_query_fails(db, "select decCompare('1', 'NaN')", "Invalid operation");
  mu_assert_query_fails(db, "select decCompare('NaN', 'NaN')", "Invalid operation");
  mu_assert_query(db, "select decCompare('03.2', '3.20') = 0", "1");
  mu_assert_query(db, "select decCompare('-03.2', '3.20') = -1", "1");
  mu_assert_query(db, "select decCompare('-1.2', '-3.20') = 1", "1");
  return 0;
}

static int sqlite_decimal_test_decdigits(void) {
  mu_skip_if(1, "Currently not implemented");
  mu_assert_query(db, "select decDigits(dec('0.0000'))", "1");
  mu_assert_query(db, "select decDigits(dec('9'))", "1");
  mu_assert_query(db, "select decDigits(dec('0.0012345'))", "5");
  mu_assert_query(db, "select decDigits(dec('0012345'))", "5");
  mu_assert_query(db, "select decDigits(dec('123450000'))", "9");
  mu_assert_query(db, "select decDigits(dec('Inf'))", "0");
  mu_assert_query(db, "select decDigits(dec('-Inf'))", "0");
  mu_assert_query(db, "select decDigits(dec('NaN'))", "0");
  return 0;
}

static int sqlite_decimal_test_decdivide(void) {
  mu_assert_query(db, "select decStr(decDiv('10.4', '2'))", "5.2");
  mu_assert_query(db, "select decStr(decDiv('10.4', '2.0'))", "5.2");
  mu_assert_query(db, "select decStr(decDiv('10.4', '2.000'))", "5.2");
  mu_assert_query(db, "select decStr(decDiv('10.400', '2'))", "5.2");
  mu_assert_query(db, "select decStr(decDiv('0', '1'))", "0");
  mu_assert_query(db, "select decStr(decDiv('0.00', '1'))", "0");
  mu_assert_query(db, "select decStr(decDiv('1', '0'))", "Division by zero");
  mu_assert_query(db, "select decStr(decDiv('-1', '0'))", "Division by zero");
  mu_assert_query(db, "select decStr(decDiv('0', '1.00'))", "0");
  mu_assert_query(db, "select decStr(decDiv('1.0', '1e-34'))", "1E+34");
  mu_assert_query(db,
      "select decStr(decDiv('1', '7'))",
      "0.142857142857142857142857142857142857143"
      );
  mu_assert_query(db,
      "select decStr(decQuantize(decDiv('1', '7'), '1.000000000000000000000000000000000'))",
      "0.142857142857142857142857142857143"
      );
  return 0;
}

static int sqlite_decimal_test_decdivideinteger(void) {
  mu_assert_query(db, "select decStr(decDivInt('10.4', '2'))", "5");
  // mu_assert_query(db, "select decGetExp(decDivInt('10.4', '2'))", "0"); // FIXME: decGetExp() not implemented
  mu_assert_query(db, "select decStr(decDivInt('10.4', '2.0'))", "5");
  mu_assert_query(db, "select decStr(decDivInt('10.4', '2.000'))", "5");
  mu_assert_query(db, "select decStr(decDivInt('10.400', '2'))", "5");
  mu_assert_query(db, "select decStr(decDivInt('0', '1'))", "0");
  mu_assert_query(db, "select decStr(decDivInt('0.00', '1'))", "0");
  mu_assert_query_fails(db, "select decStr(decDivInt('1', '0'))", "Division by zero");
  mu_assert_query_fails(db, "select decStr(decDivInt('-1', '0'))", "Division by zero");
  mu_assert_query(db, "select decStr(decDivInt('0', '1.00'))", "0");
  // mu_assert_query(db, "select decGetExp(decDivInt('0', '1.000'))", "0"); // FIXME: decGetExp() not implemented
  mu_assert_query(db, "select decStr(decDivInt('1.0', '1e-33'))", "1E+33");
  return 0;
}

static int sqlite_decimal_test_deceq(void) {
  mu_assert_query(db, "select decEq('1.0', '1.00')", "1");
  mu_assert_query(db, "select decEq('1', '0.0001e4')", "1");
  mu_assert_query(db, "select decEq('Inf', 'Inf')", "1");
  mu_assert_query(db, "select decEq('-Inf', '-Inf')", "1");
  mu_assert_query(db, "select decEq('1.999', '2.00')", "0");
  mu_assert_query(db, "select decEq('2.001', '2.00')", "0");
  mu_assert_query(db, "select decEq('Inf', '2.00')", "0");
  mu_assert_query(db, "select decEq('-Inf', 'Inf')", "0");
  mu_assert_query(db, "select decEq('NaN', 'NaN')", "0");
  mu_assert_query(db, "select decEq('NaN', '1.0')", "0");
  mu_assert_query(db, "select decEq('1.0', 'NaN')", "0");
  mu_assert_query(db, "select decEq('1e9999999990', 'Inf')", "1");
  mu_assert_query(db, "select decEq('-1e9999999990', '-Inf')", "1");
  return 0;
}

static int sqlite_decimal_test_direct_equality(void) {
  mu_assert_query(db, "select dec('1.0') = dec('1.00')", "1");
  mu_assert_query(db, "select dec('1') = dec('0.0001e4')", "1");
  mu_assert_query(db, "select dec('Inf') = dec('Inf')", "1");
  mu_assert_query(db, "select dec('-Inf') = dec('-Inf')", "1");
  mu_assert_query(db, "select dec('1.999') = dec('2.00')", "0");
  mu_assert_query(db, "select dec('2.001') = dec('2.00')", "0");
  mu_assert_query(db, "select dec('Inf') = dec('2.00')", "0");
  mu_assert_query(db, "select dec('-Inf') = dec('Inf')", "0");
  mu_assert_query(db, "select dec('NaN') = dec('NaN')", "1");
  // decInfinite has only one NaN
  mu_assert_query(db, "select dec('NaN') = dec('-NaN')", "1");
  mu_assert_query(db, "select dec('NaN') = dec('1.0')", "0");
  mu_assert_query(db, "select dec('1e9999999990') = dec('Inf')", "1");
  mu_assert_query(db, "select dec('-1e9999999990') = dec('-Inf')", "1");
  mu_assert_query(db,
      "select decDiv(dec(1),dec(7)) = dec('0.142857142857142857142857142857142857143')",
      "1"
      );
  return 0;
}

static int sqlite_decimal_test_decfma(void) {
  // decNumberFMA() invokes decCheckMath(), which enforces a few restrictions,
  // among which emax and -emin must be less than DEC_MAX_MATH (99999).
  // So, by default decFMA() raises an Invalid context error.
  mu_assert_query_fails(db, "select decStr(decFMA('3', '2.5', '4.67'))", "Invalid context");
  mu_db_execute(db, "update decContext set emin = -99999, emax = 99999");
  mu_assert_query(db, "select decStr(decFMA('3', '2.5', '4.67'))", "12.17");
  return 0;
}

static int sqlite_decimal_test_decfromint(void) {
  mu_assert_query(db, "select decStr(dec(12345))", "12345");
  mu_assert_query(db, "select decStr(dec(-12345))", "-12345");
  mu_assert_query(db, "select decStr(dec(0))", "0");
  mu_assert_query(db, "select decStr(dec(2147483647))", "2147483647");
  mu_assert_query(db, "select decStr(dec(2147483648))", "-2147483648"); // Wrap around, no error
  mu_assert_query(db, "select decStr(dec(-2147483648))", "-2147483648");
  mu_assert_query(db, "select decStr(dec(-2147483649))", "2147483647"); // Wrap around, no error
  return 0;
}

static int sqlite_decimal_test_decgetexponent(void) {
  mu_skip_if(1, "Currently not implemented");
  mu_assert_query(db, "select decGetExp(dec('-12.345'))", "-3");
  mu_assert_query(db, "select decGetExp(dec('1e17'))", "17");
  return 0;
}

static int sqlite_decimal_test_decgreatest(void) {
  mu_assert_query(db, "select decStr(decGreatest(dec('1.0'), dec('2.0')))", "2");
  mu_assert_query(db, "select decStr(decGreatest(dec('1.0'), dec('2.0'), dec('3.0')))", "3");
  mu_assert_query(db, "select decStr(decGreatest(dec('-1.0'), dec('-2.0'), dec('-3.0')))", "-1");
  mu_assert_query(db, "select decStr(decGreatest(dec('1.0'), dec('1.0')))", "1");
  mu_assert_query(db, "select decStr(decGreatest(null)) is null", "1");
  mu_assert_query(db, "select decStr(decGreatest('1', '1.0', null)) is null", "1");
  mu_assert_query(db, "select decStr(decGreatest('1', '-Inf'))", "1");
  mu_assert_query(db, "select decStr(decGreatest('1', '+Inf'))", "Infinity");
  mu_assert_query(db, "select decStr(decGreatest('-Inf', '+Inf'))", "Infinity");
  mu_assert_query(db, "select decStr(decGreatest('1', 'NaN'))", "1");
  mu_assert_query(db, "select decStr(decGreatest('1', 'NaN', 'NaN'))", "1");
  mu_assert_query(db, "select decStr(decGreatest('NaN', 'NaN', '1'))", "1");
  mu_assert_query(db, "select decStr(decGreatest('NaN', '1', 'NaN'))", "1");
  mu_assert_query(db, "select decStr(decGreatest('NaN', 'NaN'))", "NaN");
  mu_assert_query(db, "select decStr(decGreatest('NaN', 'NaN', 'NaN'))", "NaN");
  mu_assert_query(db, "select decStr(decGreatest('NaN', 'NaN', 'NaN', '1.0'))", "1");
  mu_assert_query(db, "select decStr(decGreatest('NaN', 'NaN', 'NaN', '1.0', NULL)) is null", "1");
  mu_assert_query(db, "select decStr(decGreatest('NaN', 'NaN', NULL, 'NaN', '1.0')) is null", "1");
  mu_assert_query(db, "select decStr(decGreatest(NULL, 'NaN', 'NaN', 'NaN', '1.0')) is null", "1");
  mu_assert_query(db, "select decStr(decGreatest('NaN', 'Inf'))", "Infinity");
  mu_assert_query(db, "select decStr(decGreatest('NaN', '-Inf'))", "-Infinity");
  mu_assert_query(db, "select decStr(decGreatest('NaN', '-Inf', 'Inf', 'NaN', '0.0'))", "Infinity");
  return 0;
}

static int sqlite_decimal_test_decinvert(void) {
  mu_assert_query(db,
      "select decStr(decInvert('1100101'))",
      "111111111111111111111111111111110011010"
      );
  mu_assert_query_fails(db, "select decInvert('012')", "Invalid operation");
  return 0;
}

static int sqlite_decimal_test_deciscanonical(void) {
  mu_assert_query(db, "select decIsCanonical('1.0')", "1");
  return 0;
}

static int sqlite_decimal_test_decisfinite(void) {
  mu_assert_query(db, "select decIsFinite('0.0')", "1");
  mu_assert_query(db, "select decIsFinite('Inf')", "0");
  mu_assert_query(db, "select decIsFinite('-Inf')", "0");
  mu_assert_query(db, "select decIsFinite('NaN')", "0");
  mu_assert_query(db, "select decIsFinite('-NaN')", "0");
  return 0;
}

static int sqlite_decimal_test_decisinfinite(void) {
  mu_assert_query(db, "select decIsInf('0.0')", "0");
  mu_assert_query(db, "select decIsInf('Inf')", "1");
  mu_assert_query(db, "select decIsInf('-Inf')", "1");
  mu_assert_query(db, "select decIsInf('NaN')", "0");
  mu_assert_query(db, "select decIsInf('-NaN')", "0");
  return 0;
}

static int sqlite_decimal_test_decisinteger(void) {
  mu_skip_if(1, "Currently not implemented");
  mu_assert_query(db, "select decIsInt('0')", "1");
  mu_assert_query(db, "select decIsInt('10')", "1");
  mu_assert_query(db, "select decIsInt('1.0')", "0");
  mu_assert_query(db, "select decIsInt('1e1')", "0");
  return 0;
}

static int sqlite_decimal_test_decisnan(void) {
  mu_assert_query(db, "select decIsNaN('0')", "0");
  mu_assert_query(db, "select decIsNaN('Inf')", "0");
  mu_assert_query(db, "select decIsNaN('-Inf')", "0");
  mu_assert_query(db, "select decIsNaN('NaN')", "1");
  mu_assert_query(db, "select decIsNaN('-NaN')", "1");
  return 0;
}

static int sqlite_decimal_test_decislogical(void) {
  mu_skip_if(1, "Currently not implemented");
  mu_assert_query(db, "select decIsLogical('011010111')", "1");
  mu_assert_query(db, "select decIsLogical('012')", "0");
  mu_assert_query(db, "select decIsLogical('-1')", "0");
  mu_assert_query(db, "select decIsLogical('1.00')", "0");
  mu_assert_query(db, "select decIsLogical('1e2')", "0");
  mu_assert_query(db, "select decIsLogical('Inf')", "0");
  mu_assert_query(db, "select decIsLogical('NaN')", "0");
  return 0;
}

static int sqlite_decimal_test_decisnegative(void) {
  mu_assert_query(db, "select decIsNeg('0.00')", "0");
  mu_assert_query(db, "select decIsNeg('-0.00')", "1");
  mu_assert_query(db, "select decIsNeg('-0.01')", "1");
  mu_assert_query(db, "select decIsNeg('0.01')", "0");
  mu_assert_query(db, "select decIsNeg('NaN')", "0");
  mu_assert_query(db, "select decIsNeg('Inf')", "0");
  mu_assert_query(db, "select decIsNeg('-Inf')", "1");
  // FIXME: this exhibits current inconsistent behaviour
  mu_assert_query(db, "select decIsNeg(dec('-NaN'))", "0");
  mu_assert_query(db, "select decIsNeg('-NaN')", "0");
  return 0;
}

static int sqlite_decimal_test_decisnormal(void) {
  mu_assert_query(db, "select decIsNormal('2')", "1");
  mu_assert_query(db, "select decIsNormal('0')", "0");
  mu_assert_query(db, "select decIsNormal('-0')", "0");
  mu_assert_query(db, "select decIsNormal('Inf')", "0");
  mu_assert_query(db, "select decIsNormal('-Inf')", "0");
  mu_assert_query(db, "select decIsNormal('NaN')", "0");
  mu_assert_query(db, "select decIsNormal('-NaN')", "0");
  return 0;
}

static int sqlite_decimal_test_decispositive(void) {
  mu_assert_query(db, "select decIsPos('0.00')", "0");
  mu_assert_query(db, "select decIsPos('-0.00')", "0");
  mu_assert_query(db, "select decIsPos('-0.01')", "0");
  mu_assert_query(db, "select decIsPos('0.01')", "1");
  mu_assert_query(db, "select decIsPos('NaN')", "0");
  mu_assert_query(db, "select decIsPos('-NaN')", "0");
  mu_assert_query(db, "select decIsPos('Inf')", "1");
  mu_assert_query(db, "select decIsPos('-Inf')", "0");
  return 0;
}

static int sqlite_decimal_test_decissigned(void) {
  mu_skip_if(1, "Currently not implemented");
  mu_assert_query(db, "select decIsSigned('0.00')", "0");
  mu_assert_query(db, "select decIsSigned('-0.00')", "1");
  mu_assert_query(db, "select decIsSigned('-0.01')", "1");
  mu_assert_query(db, "select decIsSigned('0.01')", "0");
  mu_assert_query(db, "select decIsSigned('NaN')", "0");
  mu_assert_query(db, "select decIsSigned('-NaN')", "1");
  mu_assert_query(db, "select decIsSigned('Inf')", "0");
  mu_assert_query(db, "select decIsSigned('-Inf')", "1");
  return 0;
}

static int sqlite_decimal_test_decissubnormal(void) {
  mu_assert_query(db, "select decIsSubnormal('2')", "0");
  mu_assert_query(db, "select decIsSubnormal('0')", "0");
  mu_assert_query(db, "select decIsSubnormal('Inf')", "0");
  mu_assert_query(db, "select decIsSubnormal('-Inf')", "0");
  mu_assert_query(db, "select decIsSubnormal('NaN')", "0");
  mu_skip_if(1, "Crashes!");
  mu_assert_query(db, "select decIsSubnormal('.1e-999999999')", "1");
  return 0;
}

static int sqlite_decimal_test_deciszero(void) {
  mu_assert_query(db, "select decIsZero('0')", "1");
  mu_assert_query(db, "select decIsZero('-0')", "1");
  mu_assert_query(db, "select decIsZero('1')", "0");
  mu_assert_query(db, "select decIsZero('-1')", "0");
  mu_assert_query(db, "select decIsZero('Inf')", "0");
  mu_assert_query(db, "select decIsZero('-Inf')", "0");
  mu_assert_query(db, "select decIsZero('NaN')", "0");
  mu_assert_query(db, "select decIsZero('-NaN')", "0");
  return 0;
}

static int sqlite_decimal_test_decleast(void) {
  mu_assert_query(db, "select decStr(decLeast(dec('1.0'), dec('2.0')))", "1");
  mu_assert_query(db, "select decStr(decLeast(dec('1.0'), dec('2.0'), dec('3.0')))", "1");
  mu_assert_query(db, "select decStr(decLeast(dec('-1.0'), dec('-2.0'), dec('-3.0')))", "-3");
  mu_assert_query(db, "select decStr(decLeast(dec('1.0'), dec('1.0')))", "1");
  mu_assert_query(db, "select decStr(decLeast(null)) is null", "1");
  mu_assert_query(db, "select decStr(decLeast('1', '1.0', null)) is null", "1");
  mu_assert_query(db, "select decStr(decLeast('1', '-Inf'))", "-Infinity");
  mu_assert_query(db, "select decStr(decLeast('1', '+Inf'))", "1");
  mu_assert_query(db, "select decStr(decLeast('-Inf', '+Inf'))", "-Infinity");
  mu_assert_query(db, "select decStr(decLeast('1', 'NaN'))", "1");
  mu_assert_query(db, "select decStr(decLeast('1', 'NaN', 'NaN'))", "1");
  mu_assert_query(db, "select decStr(decLeast('NaN', 'NaN', '1'))", "1");
  mu_assert_query(db, "select decStr(decLeast('NaN', '1', 'NaN'))", "1");
  mu_assert_query(db, "select decStr(decLeast('NaN', 'NaN'))", "NaN");
  mu_assert_query(db, "select decStr(decLeast('NaN', 'NaN', 'NaN'))", "NaN");
  mu_assert_query(db, "select decStr(decLeast('NaN', 'NaN', 'NaN', '1.0'))", "1");
  mu_assert_query(db, "select decStr(decLeast('NaN', 'NaN', 'NaN', '1.0', NULL)) is null", "1");
  mu_assert_query(db, "select decStr(decLeast('NaN', 'NaN', NULL, 'NaN', '1.0')) is null", "1");
  mu_assert_query(db, "select decStr(decLeast(NULL, 'NaN', 'NaN', 'NaN', '1.0')) is null", "1");
  mu_assert_query(db, "select decStr(decLeast('NaN', 'Inf'))", "Infinity");
  mu_assert_query(db, "select decStr(decLeast('NaN', '-Inf'))", "-Infinity");
  mu_assert_query(db, "select decStr(decLeast('NaN', 'Inf', '-Inf', 'NaN', '0.0'))", "-Infinity");
  return 0;
}

static int sqlite_decimal_test_dec_less_than(void) {
  mu_assert_query(db, "select decLt('1.0', '1.00')", "0");
  mu_assert_query(db, "select decLt('1.0', '1.00')", "0");
  mu_assert_query(db, "select decLt('1', '0.0001e4')", "0");
  mu_assert_query(db, "select decLt('Inf', 'Inf')", "0");
  mu_assert_query(db, "select decLt('-Inf', '-Inf')", "0");
  mu_assert_query(db, "select decLt('1.999', '2.00')", "1");
  mu_assert_query(db, "select decLt('2.001', '2.00')", "0");
  mu_assert_query(db, "select decLt('Inf', '2.00')", "0");
  mu_assert_query(db, "select decLt('-Inf', 'Inf')", "1");
  mu_assert_query(db, "select decLt('NaN', 'NaN')", "0");
  mu_assert_query(db, "select decLt('NaN', '1.0')", "0");
  // FIXME: inconsistent behaviour (cf. direct <)
  mu_assert_query(db, "select decLt('1.0', 'NaN')", "1");
  return 0;
}

static int sqlite_decimal_test_direct_less_than(void) {
  mu_assert_query(db, "select dec('1.0') < dec('1.00')", "0");
  mu_assert_query(db, "select dec('1.0') < dec('1.00')", "0");
  mu_assert_query(db, "select dec('1') < dec('0.0001e4')", "0");
  mu_assert_query(db, "select dec('Inf') < dec('Inf')", "0");
  mu_assert_query(db, "select dec('-Inf') < dec('-Inf')", "0");
  mu_assert_query(db, "select dec('1.999') < dec('2.00')", "1");
  mu_assert_query(db, "select dec('2.001') < dec('2.00')", "0");
  mu_assert_query(db, "select dec('Inf') < dec('2.00')", "0");
  mu_assert_query(db, "select dec('-Inf') < dec('Inf')", "1");
  mu_assert_query(db, "select dec('NaN') < dec('NaN')", "0");
  // decInfinite has only one NaN
  mu_assert_query(db, "select dec('-NaN') < dec('NaN')", "0");
  mu_assert_query(db, "select dec('NaN') < dec('1.0')", "0");
  // FIXME: inconsistent behaviour (cf. decLt())
  mu_assert_query(db, "select dec('1.0') < dec('NaN')", "1");
  return 0;
}

static int sqlite_decimal_test_dec_less_than_or_equal(void) {
  mu_assert_query(db, "select decLe('1.0', '1.00')", "1");
  mu_assert_query(db, "select decLe('1.0', '1.00')", "1");
  mu_assert_query(db, "select decLe('1', '0.0001e4')", "1");
  mu_assert_query(db, "select decLe('Inf', 'Inf')", "1");
  mu_assert_query(db, "select decLe('-Inf', '-Inf')", "1");
  mu_assert_query(db, "select decLe('1.999', '2.00')", "1");
  mu_assert_query(db, "select decLe('2.001', '2.00')", "0");
  mu_assert_query(db, "select decLe('Inf', '2.00')", "0");
  mu_assert_query(db, "select decLe('-Inf', 'Inf')", "1");
  // FIXME: inconsistent behaviour
  mu_assert_query(db, "select decLe('NaN', 'NaN')", "1");
  mu_assert_query(db, "select decLe('NaN', '1.0')", "0");
  // FIXME: inconsistent behaviour (cf. direct <=)
  mu_assert_query(db, "select decLe('1.0', 'NaN')", "1");
  return 0;
}

static int sqlite_decimal_test_direct_less_than_or_equal(void) {
  mu_assert_query(db, "select dec('1.0') <= dec('1.00')", "1");
  mu_assert_query(db, "select dec('1.0') <= dec('1.00')", "1");
  mu_assert_query(db, "select dec('1') <= dec('0.0001e4')", "1");
  mu_assert_query(db, "select dec('Inf') <= dec('Inf')", "1");
  mu_assert_query(db, "select dec('-Inf') <= dec('-Inf')", "1");
  mu_assert_query(db, "select dec('1.999') <= dec('2.00')", "1");
  mu_assert_query(db, "select dec('2.001') <= dec('2.00')", "0");
  mu_assert_query(db, "select dec('Inf') <= dec('2.00')", "0");
  mu_assert_query(db, "select dec('-Inf') <= dec('Inf')", "1");
  // FIXME: inconsistent behaviour
  mu_assert_query(db, "select dec('NaN') <= dec('NaN')", "1");
  mu_assert_query(db, "select dec('NaN') <= dec('1.0')", "0");
  // FIXME: inconsistent behaviour
  mu_assert_query(db, "select dec('1.0') <= dec('NaN')", "1");
  return 0;
}

static int sqlite_decimal_test_declogb(void) {
  mu_skip_if(1, "Currently not implemented");
  mu_assert_query(db, "select decStr(decLogB('1'))", "0");
  mu_assert_query(db, "select decStr(decLogB('-1'))", "0");
  mu_assert_query(db, "select decStr(decLogB('123'))", "2");
  mu_assert_query(db, "select decStr(decLogB('-123'))", "2");
  mu_assert_query(db, "select decStr(decLogB('1.23'))", "0");
  mu_assert_query(db, "select decStr(decLogB('12.3'))", "1");
  mu_assert_query(db, "select decStr(decLogB('0.123'))", "-1");
  mu_assert_query(db, "select decStr(decLogB('Inf'))", "Infinity");
  mu_assert_query(db, "select decStr(decLogB('-Inf'))", "Infinity");
  mu_assert_query(db, "select decStr(decLogB('NaN'))", "NaN");
  mu_assert_query(db, "select decStr(decLogB('-NaN'))", "NaN"); // decInfinite doesn't allow signed NaNs
  mu_assert_query_fails(db, "select decStr(decLogB('0.0'))", "Division by zero");
  return 0;
}

static int sqlite_decimal_test_decmaxmag(void) {
  mu_assert_query(db, "select decStr(decMaxMag(dec('1.0'), dec('2.0')))", "2");
  mu_assert_query(db, "select decStr(decMaxMag(dec('1.0'), dec('-2.0')))", "-2");
  mu_assert_query(db, "select decStr(decMaxMag(dec('-1.0'), dec('2.0')))", "2");
  mu_assert_query(db, "select decStr(decMaxMag(dec('-1.0'), dec('-2.0')))", "-2");
  mu_assert_query(db, "select decStr(decMaxMag(dec('-1.0'), dec('2.0'), dec('-3.0')))", "-3");
  mu_assert_query(db, "select decStr(decMaxMag(dec('-1.0'), dec('-2.0'), dec('-3.0')))", "-3");
  mu_assert_query(db, "select decStr(decMaxMag(dec('1.0'), dec('1.0')))", "1");
  mu_assert_query(db, "select decStr(decMaxMag(null)) is null", "1");
  mu_assert_query(db, "select decStr(decMaxMag('1', '1.0', null)) is null", "1");
  mu_assert_query(db, "select decStr(decMaxMag('-1', '-Inf'))", "-Infinity");
  mu_assert_query(db, "select decStr(decMaxMag('1', '-Inf'))", "-Infinity");
  mu_assert_query(db, "select decStr(decMaxMag('1', 'Inf'))", "Infinity");
  mu_assert_query(db, "select decStr(decMaxMag('1', '-Inf'))", "-Infinity");
  mu_assert_query(db, "select decStr(decMaxMag('1', 'NaN'))", "1");
  mu_assert_query(db, "select decStr(decMaxMag('-1', 'NaN', 'NaN'))", "-1");
  mu_assert_query(db, "select decStr(decMaxMag('NaN', 'NaN', '1'))", "1");
  mu_assert_query(db, "select decStr(decMaxMag('NaN', '-1', 'NaN'))", "-1");
  mu_assert_query(db, "select decStr(decMaxMag('NaN', 'NaN'))", "NaN");
  mu_assert_query(db, "select decStr(decMaxMag('NaN', 'NaN', 'NaN'))", "NaN");
  mu_assert_query(db, "select decStr(decMaxMag('NaN', 'NaN', 'NaN', '1.0'))", "1");
  mu_assert_query(db, "select decStr(decMaxMag('NaN', 'NaN', 'NaN', '-1.0', NULL)) is null", "1");
  mu_assert_query(db, "select decStr(decMaxMag('NaN', 'NaN', NULL, 'NaN', '1.0')) is null", "1");
  mu_assert_query(db, "select decStr(decMaxMag(NULL, 'NaN', 'NaN', 'NaN', '1.0')) is null", "1");
  mu_assert_query(db, "select decStr(decMaxMag('NaN', 'Inf'))", "Infinity");
  mu_assert_query(db, "select decStr(decMaxMag('NaN', '-Inf'))", "-Infinity");
  mu_assert_query(db, "select decStr(decMaxMag('NaN', '-Inf', 'Inf', 'NaN', '0.0'))", "Infinity");
  return 0;
}

static int sqlite_decimal_test_decminmag(void) {
  mu_assert_query(db, "select decStr(decMinMag(dec('1.0'), dec('2.0')))", "1");
  mu_assert_query(db, "select decStr(decMinMag(dec('1.0'), dec('-2.0')))", "1");
  mu_assert_query(db, "select decStr(decMinMag(dec('-1.0'), dec('2.0')))", "-1");
  mu_assert_query(db, "select decStr(decMinMag(dec('-1.0'), dec('-2.0')))", "-1");
  mu_assert_query(db, "select decStr(decMinMag(dec('-1.0'), dec('2.0'), dec('-3.0')))", "-1");
  mu_assert_query(db, "select decStr(decMinMag(dec('-1.0'), dec('-2.0'), dec('-3.0')))", "-1");
  mu_assert_query(db, "select decStr(decMinMag(dec('1.0'), dec('1.0')))", "1");
  mu_assert_query(db, "select decStr(decMinMag(null)) is null", "1");
  mu_assert_query(db, "select decStr(decMinMag('1', '1.0', null)) is null", "1");
  mu_assert_query(db, "select decStr(decMinMag('-1', '-Inf'))", "-1");
  mu_assert_query(db, "select decStr(decMinMag('1', '-Inf'))", "1");
  mu_assert_query(db, "select decStr(decMinMag('1', 'Inf'))", "1");
  mu_assert_query(db, "select decStr(decMinMag('1', '-Inf'))", "1");
  mu_assert_query(db, "select decStr(decMinMag('1', 'NaN'))", "1");
  mu_assert_query(db, "select decStr(decMinMag('-1', 'NaN', 'NaN'))", "-1");
  mu_assert_query(db, "select decStr(decMinMag('NaN', 'NaN', '1'))", "1");
  mu_assert_query(db, "select decStr(decMinMag('NaN', '-1', 'NaN'))", "-1");
  mu_assert_query(db, "select decStr(decMinMag('NaN', 'NaN'))", "NaN");
  mu_assert_query(db, "select decStr(decMinMag('NaN', 'NaN', 'NaN'))", "NaN");
  mu_assert_query(db, "select decStr(decMinMag('NaN', 'NaN', 'NaN', '1.0'))", "1");
  mu_assert_query(db, "select decStr(decMinMag('NaN', 'NaN', 'NaN', '-1.0', NULL)) is null", "1");
  mu_assert_query(db, "select decStr(decMinMag('NaN', 'NaN', NULL, 'NaN', '1.0')) is null", "1");
  mu_assert_query(db, "select decStr(decMinMag(NULL, 'NaN', 'NaN', 'NaN', '1.0')) is null", "1");
  mu_assert_query(db, "select decStr(decMinMag('NaN', 'Inf'))", "Infinity");
  mu_assert_query(db, "select decStr(decMinMag('NaN', '-Inf'))", "-Infinity");
  mu_assert_query(db, "select decStr(decMinMag('NaN', '-Inf', 'Inf', 'NaN', '0.0'))", "0");
  return 0;
}

static int sqlite_decimal_test_decmultiply(void) {
  mu_assert_query(db, "select decStr(decMul())", "1");
  mu_assert_query(db, "select decStr(decMul('1.00'))", "1");
  mu_assert_query(db, "select decStr(decMul('2.00', '3.000'))", "6");
  mu_assert_query(db, "select decStr(decMul('1.00', '3.00', '2.6543'))", "7.9629");
  mu_assert_query(db, "select decStr(decMul(null)) is null", "1");
  mu_assert_query(db, "select decStr(decMul('2.00', null)) is null", "1");
  mu_assert_query(db, "select decStr(decMul('2.00', 'Inf'))", "Infinity");
  mu_assert_query(db, "select decStr(decMul('-2.00', 'Inf'))", "-Infinity");
  mu_assert_query(db, "select decStr(decMul('2.00', '-Inf'))", "-Infinity");
  mu_assert_query(db, "select decStr(decMul('-2.00', '-Inf'))", "Infinity");
  mu_assert_query(db, "select decStr(decMul('NaN', '3.2'))", "NaN");
  mu_assert_query(db, "select decStr(decMul('NaN', '-Inf'))", "NaN");
  mu_assert_query(db, "select decStr(decMul('NaN', 'NaN'))", "NaN");
  return 0;
}

static int sqlite_decimal_test_decneg(void) {
  mu_assert_query(db, "select decStr(decNeg('1.23'))", "-1.23");
  mu_assert_query(db, "select decStr(decNeg('-1.23'))", "1.23");
  mu_assert_query(db, "select decStr(decNeg('0.0'))", "0");
  mu_assert_query(db, "select decStr(decNeg('-0.0'))", "0");
  mu_assert_query(db, "select decStr(decNeg('NaN'))", "NaN");
  mu_assert_query(db, "select decStr(decNeg('-NaN'))", "NaN"); // decInfinite doesn't allow signed NaNs
  mu_assert_query(db, "select decStr(decNeg('Inf'))", "-Infinity");
  mu_assert_query(db, "select decStr(decNeg('-Inf'))", "Infinity");
  return 0;
}

static int sqlite_decimal_test_decor(void) {
  mu_assert_query(db, "select decStr(decOr('010', '110'))", "110");
  // decOr() works with sequences of different lengths. The result has the
  // length of the longest one (minus leading zeroes).
  mu_assert_query(db, "select decStr(decOr('001010110110', '011'))", "1010110111");
  mu_assert_query_fails(db, "select decOr('1001', '012')", "Invalid operation");
  return 0;
}

static int sqlite_decimal_test_decplus(void) {
  mu_assert_query(db, "select decStr(decPlus('1.23'))", "1.23");
  mu_assert_query(db, "select decStr(decPlus('-1.23'))", "-1.23");
  mu_assert_query(db, "select decStr(decPlus('0.0'))", "0");
  mu_assert_query(db, "select decStr(decPlus('-0.0'))", "0");
  mu_assert_query(db, "select decStr(decPlus('NaN'))", "NaN");
  mu_assert_query(db, "select decStr(decPlus('-NaN'))", "NaN"); // decInfinite doesn't allow signed NaNs
  mu_assert_query(db, "select decStr(decPlus('Inf'))", "Infinity");
  mu_assert_query(db, "select decStr(decPlus('-Inf'))", "-Infinity");
  return 0;
}

static int sqlite_decimal_test_decquantize(void) {
  mu_db_execute(db, "update decContext set round = 'ROUND_HALF_EVEN'");
  mu_assert_query(db, "select decStr(decQuantize('1.234', '1.00'))", "1.23");
  mu_assert_query(db, "select decStr(decQuantize('1.236', '1.00'))", "1.24");
  mu_assert_query(db, "select decStr(decQuantize('1.235', '1.00'))", "1.24"); // round to even
  mu_assert_query(db, "select decStr(decQuantize('1.245', '1.00'))", "1.24"); // round to even
  mu_assert_query(db, "select decStr(decQuantize('1.235', '1'))", "1");
  mu_assert_query(db, "select decStr(decQuantize('1.2', '1.00'))", "1.2");
  return 0;
}

static int sqlite_decimal_test_decquantize_nulls(void) {
  mu_assert_query(db, "select decStr(decQuantize(null, '10.0')) is null", "1");
  mu_assert_query(db, "select decStr(decQuantize('1.23', null)) is null", "1");
  return 0;
}

static int sqlite_decimal_test_decreduce(void) {
  mu_assert_query(db, "select decStr(decReduce('1.234'))", "1.234");
  mu_assert_query(db, "select decStr(decReduce('1.234000'))", "1.234");
  mu_assert_query(db, "select decStr(decReduce('0.000100'))", "0.0001");
  mu_assert_query(db, "select decStr(decReduce('Inf'))", "Infinity");
  mu_assert_query(db, "select decStr(decReduce('-Inf'))", "-Infinity");
  mu_assert_query(db, "select decStr(decReduce('NaN'))", "NaN");
  return 0;
}

static int sqlite_decimal_test_decremainder(void) {
  mu_assert_query(db, "select decStr(decRemainder('10.4', '2'))", "0.4");
  // mu_assert_query(db, "select decGetExp(decRemainder('10.4', '2'))", "-1"); // FIXME: decGetExp() not implemented
  mu_assert_query(db, "select decStr(decRemainder('10.4', '2.0'))", "0.4");
  mu_assert_query(db, "select decStr(decRemainder('10.4', '2.000'))", "0.4");
  mu_assert_query(db, "select decStr(decRemainder('10.400', '2'))", "0.4");
  mu_assert_query(db, "select decStr(decRemainder('0', '1'))", "0");
  mu_assert_query(db, "select decStr(decRemainder('0.00', '1'))", "0");
  mu_assert_query_fails(db, "select decStr(decRemainder('1', '0'))", "Invalid operation");
  mu_assert_query_fails(db, "select decStr(decRemainder('-1', '0'))", "Invalid operation");
  mu_assert_query(db, "select decStr(decRemainder('0', '1.00'))", "0");
  // mu_assert_query(db, "select decGetExp(decRemainder('0', '1.000'))", "-3"); // FIXME: decGetExp() not implemented
  return 0;
}

static int sqlite_decimal_test_decrotate(void) {
  mu_assert_query(db, "select decStr(decRotate('1.2345', '2'))", "123.45");
  // The shift must be an integer
  mu_assert_query_fails(db, "select decStr(decRotate('1.2345', '1.2'))", "Invalid operation");
  // The shift must be finite
  mu_assert_query_fails(db, "select decStr(decRotate('1.2345', 'Inf'))", "Invalid operation");
  return 0;
}

static int sqlite_decimal_test_decsamequantum(void) {
  mu_assert_query(db, "select decSameQuantum('1.0', '10.0')", "1");
  mu_assert_query(db, "select decSameQuantum('1.0', '1.00')", "0");
  mu_assert_query(db, "select decSameQuantum('NaN', '1.00')", "0");
  mu_assert_query(db, "select decSameQuantum('Inf', '1.00')", "0");
  mu_assert_query(db, "select decSameQuantum('Inf', 'Inf')", "1");
  mu_assert_query(db, "select decSameQuantum('Inf', '-Inf')", "1");
  mu_assert_query(db, "select decSameQuantum('NaN', 'NaN')", "1");
  return 0;
}

static int sqlite_decimal_test_decscaleb(void) {
  mu_assert_query(db, "select decStr(decScaleB('1.23', '3'))", "1.23E+3");
  return 0;
}

static int sqlite_decimal_test_decshift(void) {
  mu_assert_query(db, "select decStr(decShift('1.2345', '2'))", "123.45");
  mu_assert_query(db, "select decStr(decShift('1.2345', '-2'))", "0.0123");
  mu_assert_query(db, "select decStr(decShift('123.45', '4'))", "1.2345E+6"); // FIXME?
  mu_assert_query(db, "select decStr(decShift('123.45', '-2'))", "1.23");
  mu_assert_query(db, "select decStr(decShift('0.0012345', '3'))", "1.2345");
  // The shift must be an integer
  mu_assert_query_fails(db, "select decStr(decShift('1.2345', '1.2'))", "Invalid operation");
  // The shift must be finite
  mu_assert_query_fails(db, "select decStr(decShift('1.2345', 'Inf'))", "Invalid operation");
  return 0;
}

static int sqlite_decimal_test_decstr(void) {
  mu_assert_query(db, "select decStr(dec('1e35'))", "1E+35");
  mu_assert_query(db, "select decStr(dec('1e-35'))", "1E-35");
  return 0;
}

static int sqlite_decimal_test_decstr_null(void) {
  mu_assert_query(db, "select decStr(null) is null", "1");
  return 0;
}

static int sqlite_decimal_test_decstr_NaN(void) {
  mu_assert_query(db, "select decStr('NaN')", "NaN");
  mu_assert_query(db, "select decStr('nan')", "NaN");
  mu_assert_query(db, "select decStr('NAN')", "NaN");
  return 0;
}

static int sqlite_decimal_test_decsubtract(void) {
  mu_assert_query(db, "select decStr(decSub('1.0', '10.00'))", "-9");
  mu_assert_query(db, "select decStr(decSub('1.0', '1.00'))", "0");
  mu_assert_query(db, "select decStr(decSub('NaN', '1.00'))", "NaN");
  mu_assert_query(db, "select decStr(decSub('Inf', '1.00'))", "Infinity");
  mu_assert_query(db, "select decStr(decSub('Inf', 'Inf'))", "Invalid operation");
  mu_assert_query(db, "select decStr(decSub('Inf', '-Inf'))", "Infinity");
  mu_assert_query(db, "select decStr(decSub('NaN', 'NaN'))", "NaN");
  return 0;
}

static int sqlite_decimal_test_decsum_null(void) {
  // Nulls are ignored for the purpose of aggregation, so
  // the result is the same as the sum of an empty set of values.
  mu_assert_query(db, "select decStr(decSum(null))", "0");
  return 0;
}

static int sqlite_decimal_test_dectoint32(void) {
  mu_assert_query(db, "select decToInt32('0')", "0");
  mu_assert_query(db, "select decToInt32('123')", "123");
  // decToInt32() uses decNumberToInt32(), which sets DEC_Invalid_operation if
  // its argument does not have an exponent of 0 (i.e., it's an integer), it is
  // NaN or Infinity or if it is out-of-range
  mu_assert_query_fails(db, "select decToInt32('123.5')", "Invalid operation");
  mu_assert_query_fails(db, "select decToInt32('122.5')", "Invalid operation");
  mu_assert_query(db, "select decToInt32('-123')", "-123");
  mu_assert_query_fails(db, "select decToInt32('-123.5')", "Invalid operation");
  mu_assert_query_fails(db, "select decToInt32('-122.5')", "Invalid operation");
  mu_assert_query_fails(db, "select decToInt32('Inf')", "Invalid operation");
  mu_assert_query_fails(db, "select decToInt32('-Inf')", "Invalid operation");
  mu_assert_query_fails(db, "select decToInt32('NaN')", "Invalid operation");
  mu_assert_query(db, "select decToInt32(dec('2147483647'))", "2147483647");
  mu_assert_query(db, "select decToInt32(dec(2147483648))", "-2147483648"); // Wraps around, no error
  mu_assert_query(db, "select decToInt32(dec(-2147483648))", "-2147483648");
  mu_assert_query(db, "select decToInt32(dec(-2147483649))", "2147483647"); // Wraps around, no error
  return 0;
}

static int sqlite_decimal_test_dectointegral(void) {
  mu_assert_query(db, "select decStr(decToIntegral('0'))", "0");
  mu_assert_query(db, "select decStr(decToIntegral('1.51'))", "2");
  mu_assert_query(db, "select decStr(decToIntegral('1.49'))", "1");
  return 0;
}

static int sqlite_decimal_test_decxor(void) {
  mu_assert_query(db, "select decStr(decXor('010', '110'))", "100");
  // decOr() works with sequences of different lengths. The result has the
  // length of the longest one (minus leading zeroes).
  mu_assert_query(db, "select decStr(decXor('001010110110', '011'))", "1010110101");
  mu_assert_query_fails(db, "select decXor('1001', '012')", "Invalid operation");
  return 0;
}

static int sqlite_decimal_test_rounding_modes(void) {
  mu_db_execute(db, "update decContext set round = 'ROUND_CEILING'");
  mu_assert_query(db, "select decStr(decQuantize('1.005', '1.00'))", "1.01");
  mu_assert_query(db, "select decStr(decQuantize('1.015', '1.00'))", "1.02");
  mu_assert_query(db, "select decStr(decQuantize('1.005', '1.0'))", "1.1");
  mu_assert_query(db, "select decStr(decQuantize('1.005', '1'))", "2");
  mu_assert_query(db, "select decStr(decQuantize('-1.005', '1.00'))", "-1");
  mu_assert_query(db, "select decStr(decQuantize('-1.015', '1.00'))", "-1.01");
  mu_assert_query(db, "select decStr(decQuantize('-1.005', '1.0'))", "-1");
  mu_assert_query(db, "select decStr(decQuantize('-1.005', '1'))", "-1");

  mu_db_execute(db, "update decContext set round = 'ROUND_UP'");
  mu_assert_query(db, "select decStr(decQuantize('1.005', '1.00'))", "1.01");
  mu_assert_query(db, "select decStr(decQuantize('1.015', '1.00'))", "1.02");
  mu_assert_query(db, "select decStr(decQuantize('1.005', '1.0'))", "1.1");
  mu_assert_query(db, "select decStr(decQuantize('1.005', '1'))", "2");
  mu_assert_query(db, "select decStr(decQuantize('-1.005', '1.00'))", "-1.01");
  mu_assert_query(db, "select decStr(decQuantize('-1.015', '1.00'))", "-1.02");
  mu_assert_query(db, "select decStr(decQuantize('-1.005', '1.0'))", "-1.1");
  mu_assert_query(db, "select decStr(decQuantize('-1.005', '1'))", "-2");

  mu_db_execute(db, "update decContext set round = 'ROUND_05UP'");
  mu_assert_query(db, "select decStr(decQuantize('1.005', '1.00'))", "1.01");
  mu_assert_query(db, "select decStr(decQuantize('1.015', '1.00'))", "1.01");
  mu_assert_query(db, "select decStr(decQuantize('1.005', '1.0'))", "1.1");
  mu_assert_query(db, "select decStr(decQuantize('1.005', '1'))", "1");
  mu_assert_query(db, "select decStr(decQuantize('-1.005', '1.00'))", "-1.01");
  mu_assert_query(db, "select decStr(decQuantize('-1.015', '1.00'))", "-1.01");
  mu_assert_query(db, "select decStr(decQuantize('-1.005', '1.0'))", "-1.1");
  mu_assert_query(db, "select decStr(decQuantize('-1.005', '1'))", "-1");

  mu_db_execute(db, "update decContext set round = 'ROUND_HALF_UP'");
  mu_assert_query(db, "select decStr(decQuantize('1.005', '1.00'))", "1.01");
  mu_assert_query(db, "select decStr(decQuantize('1.015', '1.00'))", "1.02");
  mu_assert_query(db, "select decStr(decQuantize('1.005', '1.0'))", "1");
  mu_assert_query(db, "select decStr(decQuantize('1.005', '1'))", "1");
  mu_assert_query(db, "select decStr(decQuantize('-1.005', '1.00'))", "-1.01");
  mu_assert_query(db, "select decStr(decQuantize('-1.015', '1.00'))", "-1.02");
  mu_assert_query(db, "select decStr(decQuantize('-1.005', '1.0'))", "-1");
  mu_assert_query(db, "select decStr(decQuantize('-1.005', '1'))", "-1");

  mu_db_execute(db, "update decContext set round = 'ROUND_HALF_EVEN'");
  mu_assert_query(db, "select decStr(decQuantize('1.005', '1.00'))", "1");
  mu_assert_query(db, "select decStr(decQuantize('1.015', '1.00'))", "1.02");
  mu_assert_query(db, "select decStr(decQuantize('1.005', '1.0'))", "1");
  mu_assert_query(db, "select decStr(decQuantize('1.005', '1'))", "1");
  mu_assert_query(db, "select decStr(decQuantize('-1.005', '1.00'))", "-1");
  mu_assert_query(db, "select decStr(decQuantize('-1.015', '1.00'))", "-1.02");
  mu_assert_query(db, "select decStr(decQuantize('-1.005', '1.0'))", "-1");
  mu_assert_query(db, "select decStr(decQuantize('-1.005', '1'))", "-1");

  mu_db_execute(db, "update decContext set round = 'ROUND_HALF_DOWN'");
  mu_assert_query(db, "select decStr(decQuantize('1.005', '1.00'))", "1");
  mu_assert_query(db, "select decStr(decQuantize('1.015', '1.00'))", "1.01");
  mu_assert_query(db, "select decStr(decQuantize('1.005', '1.0'))", "1");
  mu_assert_query(db, "select decStr(decQuantize('1.005', '1'))", "1");
  mu_assert_query(db, "select decStr(decQuantize('-1.005', '1.00'))", "-1");
  mu_assert_query(db, "select decStr(decQuantize('-1.015', '1.00'))", "-1.01");
  mu_assert_query(db, "select decStr(decQuantize('-1.005', '1.0'))", "-1");
  mu_assert_query(db, "select decStr(decQuantize('-1.005', '1'))", "-1");

  mu_db_execute(db, "update decContext set round = 'ROUND_FLOOR'");
  mu_assert_query(db, "select decStr(decQuantize('1.005', '1.00'))", "1");
  mu_assert_query(db, "select decStr(decQuantize('1.015', '1.00'))", "1.01");
  mu_assert_query(db, "select decStr(decQuantize('1.005', '1.0'))", "1");
  mu_assert_query(db, "select decStr(decQuantize('1.005', '1'))", "1");
  mu_assert_query(db, "select decStr(decQuantize('-1.005', '1.00'))", "-1.01");
  mu_assert_query(db, "select decStr(decQuantize('-1.015', '1.00'))", "-1.02");
  mu_assert_query(db, "select decStr(decQuantize('-1.005', '1.0'))", "-1.1");
  mu_assert_query(db, "select decStr(decQuantize('-1.005', '1'))", "-2");

  // Back to default
  mu_db_execute(db, "update decContext set round = 'DEFAULT'");
  mu_assert_query(db, "select round from decContext", "ROUND_HALF_UP");
  return 0;
}

#pragma mark Test vtables

static int sqlite_decimal_test_context_delete_fails(void) {
  mu_assert_query_fails(db, "delete from decContext",
                        "Deleting from decContext is not allowed");
  return 0;
}

static int sqlite_decimal_test_context_insert_fails(void) {
  mu_assert_query_fails(db, "insert into decContext values (0,0,0,0)",
                        "Inserting into decContext is not allowed");
  return 0;
}

static int sqlite_decimal_test_context_max_exp(void) {
  mu_assert_query(db, "select emax = 999999999 from decContext", "1");
  return 0;
}

static int
sqlite_decimal_test_context_min_exp(void) {
  mu_assert_query(db, "select emin = -999999999 from decContext", "1");
  return 0;
}

static int sqlite_decimal_test_context_precision(void) {
  mu_assert_query(db, "select prec = 39 from decContext", "1");
  return 0;
}

static int sqlite_decimal_test_context_rounding_default(void) {
  // When the decimal context is initialized to DEC_INIT_BASE, the defaults for
  // the ANSI X3.274 arithmetic subset are used. In particular, the round field
  // of the context is set to ROUND_HALF_UP.
  mu_assert_query(db, "select round from decContext", "ROUND_HALF_UP");
  return 0;
}

static int sqlite_decimal_test_context_set_rounding(void) {
  mu_db_execute(db, "update decContext set round = 'ROUND_UP'");
  mu_assert_query(db, "select round from decContext", "ROUND_UP");
  mu_db_execute(db, "update decContext set round = 'DEFAULT'");
  mu_assert_query(db, "select round from decContext", "ROUND_HALF_UP");
  return 0;
}

static int sqlite_decimal_test_context_update_set_null_fails() {
  mu_assert_query_fails(db, "update decContext set round = null", "Value cannot be NULL");
  mu_assert_query_fails(db, "update decContext set prec = null", "Value cannot be NULL");
  return 0;
}

static int sqlite_decimal_test_status(void) {
  mu_assert_query(db, "select count(*) from decStatus", "0");
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

  mu_db_execute(db, "delete from decStatus where flag = 'Overflow'");
  mu_db_execute(db, "update decStatus set flag = 'Overflow' where flag = 'Out of memory'");
  mu_db_execute(db, "update decStatus set flag = 'Overflow' where flag = 'Subnormal'");
  mu_assert_query(db, "select count(*) from decStatus", "11");

  return 0;
}

static int sqlite_decimal_test_status_inexact_flag(void) {
  mu_assert_query(db, "select decStr(decQuantize(decDiv('1','7'), '1.0000'))", "0.1429");
  mu_assert_query(db, "select flag from decStatus", "Inexact result");
  return 0;
}

static int sqlite_decimal_test_status_insert_null_fails(void) {
  mu_assert_query_fails(db, "insert into decStatus values (null)", "Value cannot be NULL");
  return 0;
}

static int sqlite_decimal_test_traps_default(void) {
  mu_assert_query(db, "select count(*) from decTraps", "7");
  mu_assert_query(db, "select flag from decTraps where flag = 'Conversion syntax'", "Conversion syntax");
  mu_assert_query(db, "select flag from decTraps where flag = 'Division by zero'", "Division by zero");
  mu_assert_query(db, "select flag from decTraps where flag = 'Division undefined'", "Division undefined");
  mu_assert_query(db, "select flag from decTraps where flag = 'Division impossible'", "Division impossible");
  mu_assert_query(db, "select flag from decTraps where flag = 'Out of memory'", "Out of memory");
  mu_assert_query(db, "select flag from decTraps where flag = 'Invalid context'", "Invalid context");
  mu_assert_query(db, "select flag from decTraps where flag = 'Invalid operation'", "Invalid operation");
  return 0;
}

static int sqlite_decimal_test_traps_division_impossible_flag(void) {
  mu_assert_query(db, "select flag from decTraps where flag = 'Division impossible'", "Division impossible");
  mu_assert_query_fails(db, "select decStr(decDivInt('1e6143','0.0000001'))", "Division impossible");
  mu_assert_query(db, "select count(*) from decStatus", "0");
  return 0;
}

static int sqlite_decimal_test_traps_insert_null_fails(void) {
  mu_assert_query_fails(db, "insert into decTraps values (null)", "Value cannot be NULL");
  return 0;
}


#pragma mark Test runner

static void sqlite_test_context_setup() {
  // Reset context
  mu_db_execute(db, "update decContext set "
      "prec = 39,"
      "emin = -999999999,"
      "emax = 999999999,"
      "round = 'DEFAULT'"
      );
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

static void sqlite_decimal_func_tests(void) {
  mu_setup = sqlite_test_context_setup;
  mu_teardown = mu_noop;

  mu_test(sqlite_decimal_test_decbytes);
  mu_test(sqlite_decimal_test_decbits);
  mu_test(sqlite_decimal_test_dec_NaN);
  mu_test(sqlite_decimal_test_dec_Inf);
  mu_test(sqlite_decimal_test_dec_neg);
  mu_test(sqlite_decimal_test_dec_parse_error);
  mu_test(sqlite_decimal_test_decabs);
  mu_test(sqlite_decimal_test_decabs_null);
  mu_test(sqlite_decimal_test_decadd_two_numbers);
  mu_test(sqlite_decimal_test_decadd_big_and_small_number);
  mu_test(sqlite_decimal_test_decadd_text_args);
  mu_test(sqlite_decimal_test_decadd_mixed_args);
  mu_test(sqlite_decimal_test_decadd_empty_arg);
  mu_test(sqlite_decimal_test_decadd_null);
  mu_test(sqlite_decimal_test_decadd_nan);
  mu_test(sqlite_decimal_test_decand);
  mu_test(sqlite_decimal_test_decavg_null);
  mu_test(sqlite_decimal_test_decclass);
  mu_test(sqlite_decimal_test_deccompare);
  mu_test(sqlite_decimal_test_decdigits);
  mu_test(sqlite_decimal_test_decdivide);
  mu_test(sqlite_decimal_test_decdivideinteger);
  mu_test(sqlite_decimal_test_deceq);
  mu_test(sqlite_decimal_test_direct_equality);
  mu_test(sqlite_decimal_test_decfma);
  mu_test(sqlite_decimal_test_decfromint);
  mu_test(sqlite_decimal_test_decgetexponent);
  mu_test(sqlite_decimal_test_decgreatest);
  mu_test(sqlite_decimal_test_decinvert);
  mu_test(sqlite_decimal_test_deciscanonical);
  mu_test(sqlite_decimal_test_decisfinite);
  mu_test(sqlite_decimal_test_decisinfinite);
  mu_test(sqlite_decimal_test_decisinteger);
  mu_test(sqlite_decimal_test_decislogical);
  mu_test(sqlite_decimal_test_decisnegative);
  mu_test(sqlite_decimal_test_decisnormal);
  mu_test(sqlite_decimal_test_decispositive);
  mu_test(sqlite_decimal_test_decissigned);
  mu_test(sqlite_decimal_test_decissubnormal);
  mu_test(sqlite_decimal_test_deciszero);
  mu_test(sqlite_decimal_test_decisnan);
  mu_test(sqlite_decimal_test_decleast);
  mu_test(sqlite_decimal_test_dec_less_than);
  mu_test(sqlite_decimal_test_direct_less_than);
  mu_test(sqlite_decimal_test_dec_less_than_or_equal);
  mu_test(sqlite_decimal_test_direct_less_than_or_equal);
  mu_test(sqlite_decimal_test_declogb);
  mu_test(sqlite_decimal_test_decmaxmag);
  mu_test(sqlite_decimal_test_decminmag);
  mu_test(sqlite_decimal_test_decmultiply);
  mu_test(sqlite_decimal_test_decneg);
  mu_test(sqlite_decimal_test_decor);
  mu_test(sqlite_decimal_test_decplus);
  mu_test(sqlite_decimal_test_decquantize);
  mu_test(sqlite_decimal_test_decquantize_nulls);
  mu_test(sqlite_decimal_test_decreduce);
  mu_test(sqlite_decimal_test_decremainder);
  mu_test(sqlite_decimal_test_decrotate);
  mu_test(sqlite_decimal_test_decsamequantum);
  mu_test(sqlite_decimal_test_decscaleb);
  mu_test(sqlite_decimal_test_decshift);
  mu_test(sqlite_decimal_test_decstr);
  mu_test(sqlite_decimal_test_decstr_null);
  mu_test(sqlite_decimal_test_decstr_NaN);
  mu_test(sqlite_decimal_test_decsubtract);
  mu_test(sqlite_decimal_test_decsum_null);
  mu_test(sqlite_decimal_test_dectoint32);
  mu_test(sqlite_decimal_test_dectointegral);
  mu_test(sqlite_decimal_test_decxor);
  mu_test(sqlite_decimal_test_rounding_modes);
}

static void sqlite_decimal_vtab_tests(void) {
  mu_setup = sqlite_test_context_setup;
  mu_teardown = mu_noop;

  mu_test(sqlite_decimal_test_context_delete_fails);
  mu_test(sqlite_decimal_test_context_insert_fails);
  mu_test(sqlite_decimal_test_context_max_exp);
  mu_test(sqlite_decimal_test_context_min_exp);
  mu_test(sqlite_decimal_test_context_precision);
  mu_test(sqlite_decimal_test_context_rounding_default);
  mu_test(sqlite_decimal_test_context_update_set_null_fails);
  mu_test(sqlite_decimal_test_context_set_rounding);
  mu_test(sqlite_decimal_test_status);
  mu_test(sqlite_decimal_test_status_inexact_flag);
  mu_test(sqlite_decimal_test_status_insert_null_fails);
  mu_test(sqlite_decimal_test_traps_default);
  mu_test(sqlite_decimal_test_traps_division_impossible_flag);
  mu_test(sqlite_decimal_test_traps_insert_null_fails);
}

