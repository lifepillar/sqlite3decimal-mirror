/**
 * \file impl_decinfinite.c
 * \author    Lifepillar
 * \copyright Copyright (c) 2019 Lifepillar.
 *            This program is free software; you can redistribute it and/or
 *            modify it under the terms of the Simplified BSD License (also
 *            known as the "2-Clause License" or "FreeBSD License".)
 * \copyright This program is distributed in the hope that it will be useful,
 *            but without any warranty; without even the implied warranty of
 *            merchantability or fitness for a particular purpose.
 *
 * \brief Implementation of decimal arithmetic using decNumber and the
 *        decimalInfinite encoding.
 *
 * \todo Optimize comparison functions
 * \todo Use signal handling mechanism instead of checkStatus()?
 * \todo Allow choosing allocation strategy (static vs dynamic) for local variables.
 * \todo Better error reporting for functions using decCheckMath() (currently,
 *       they report Invalid context)
 * \todo Reset context status after each successful operation?
 * \todo Fix semantics inconsistencies! For instance: decIsNeg('-NaN') is true,
 *       but decIsNeg(dec('-NaN')) is false.
 */
#include <signal.h>
#include <string.h>
#include "decInfinite.h"
#include "decNumber/decPacked.h" // This must be included *after* decInfinite.h
#include "impl_decimal.h"

#pragma mark Helper functions

/**
 * \brief Builds a decimal from a text field.
 *
 * \param result The output decimal
 * \param value A value of type `SQLITE_TEXT`
 * \param decCtx decNumber's context
 *
 * \return \a result
 *
 * If bad syntax is detected, the result is an error if the decTraps virtual
 * table contains `Conversion syntax`; otherwise, the result is a a quiet
 * `NaN` and `Conversion syntax` is inserted into the `decStatus` table.
 **/
static decNumber* decNumberFromSQLite3Text(decNumber* result, sqlite3_value* value, decContext* decCtx) {
  char const* const string_num = (char*)sqlite3_value_text(value);
  decNumberFromString(result, string_num, decCtx);
  return result;
}

/**
 * \brief Builds a decimal from a blob field.
 *
 * Decimals are stored using the decimalInfinite format. If the blob does not
 * have the correct format, the `DEC_Conversion_syntax` flag is set and the
 * result is a quiet `NaN` or an error depending on whether the error is
 * trapped.
 *
 * \param result The output decimal
 * \param value A value of type `SQLITE_BLOB`
 * \param decCtx decNumber's context
 *
 * \return \a result
 *
 * \see decInfinite.h
 **/
static decNumber* decNumberFromSQLite3Blob(decNumber* result, sqlite3_value* value, decContext* decCtx) {
  int length = sqlite3_value_bytes(value);
  uint8_t const* bytes = sqlite3_value_blob(value);
  if (decInfiniteToNumber(length, bytes, result)) return result;
  decContextSetStatusQuiet(decCtx, DEC_Conversion_syntax);
  return result;
}

/**
 * \brief Creates a decimal from an integer.
 *
 * \param result The output decimal
 * \param value A value of type `SQLITE_INTEGER`
 *
 * \return \a result
 **/
static decNumber* decNumberFromSQLite3Integer(decNumber* result, sqlite3_value* value) {
  int32_t n = sqlite3_value_int(value);
  decNumberFromInt32(result, n);
  return result;
}

/**
 * \brief Encodes a decimal value.
 *
 * \param context SQLite3 context
 * \param decnum A decimal value to encode
 *
 * \note This function **modifies** the input \a decnum. If you need to use the
 *       value after calling this function, **make a copy first**.
 */
static void decNumberToSQLite3Blob(sqlite3_context* context, decNumber* decnum) {
  uint8_t* bytes = sqlite3_malloc(DECINF_MAXSIZE * sizeof(uint8_t));
  if (bytes) {
    size_t length = decInfiniteFromNumber(DECINF_MAXSIZE, bytes, decnum);
    sqlite3_result_blob(context, bytes, length, SQLITE_TRANSIENT); // FIXME: Use SQLITE_STATIC to avoid copy?
    sqlite3_free(bytes);
  }
  else
    sqlite3_result_error_nomem(context);
}

/**
 * \brief Checks decNumber's context status and interrupts the current SQL
 *        operation with an error if any of the mask bit is set.
 *
 * \param sqlCtx SQLIte3's context
 * \param decContext decNumber's context
 * \param mask decNumber's status bits
 *
 * \return `1` if there is no error; `0` otherwise.
 *
 * \see decNumber's manual, p. 29.
 **/
static int checkStatus(sqlite3_context* sqlCtx, decContext* decCtx, uint32_t mask) {
  // TODO: use the signal handling mechanism instead of this?
  if (decContextTestStatus(decCtx, mask)) {
    decContext errCtx; // FIXME: reimplement without copying the whole context
    decimalContextCopy(&errCtx, decCtx);
    decContextClearStatus(&errCtx, ~mask);
    sqlite3_result_error(sqlCtx, decContextStatusToString(&errCtx), -1);
    // Clear the flag(s) that generated the error
    decContextClearStatus(decCtx, mask);
    return 0;
  }
  return 1;
}

/**
 * \brief Initializes a decimal from a SQLite3 integer, blob, or string.
 *
 * If an error occurs, an error message is set through sqlite3_result_error().
 *
 * \param decnum The output decimal
 * \param decCtx decNumber's context
 * \param value A SQLite3 value
 * \param sqlCtx SQLite3's context
 *
 * \return `1` upon success; `0` if an error occurs.
 **/
static int decode(decNumber* decnum, decContext* decCtx, sqlite3_value* value, sqlite3_context* sqlCtx) {
  switch (sqlite3_value_type(value)) {
    case SQLITE_BLOB:
      decNumberFromSQLite3Blob(decnum, value, decCtx);
      break;
    case SQLITE_TEXT:
      decNumberFromSQLite3Text(decnum, value, decCtx);
      break;
    case SQLITE_INTEGER:
      decNumberFromSQLite3Integer(decnum, value);
      break;
    default:
      sqlite3_result_error(sqlCtx, "Cannot create decimal from the given type", -1);
      return 0;
  }
  return checkStatus(sqlCtx, decCtx, decCtx->traps);
}

#pragma mark Context functions

/**
 * \brief Handles `SIGFPE` signals.
 *
 * Quoting from decNumber's manual, p.14:
 *
 * > When one of the decNumber functions sets a bit in the context status, the
 * > bit is compared with the corresponding bit in the traps field. If that bit
 * > is set (is 1) then a C Floating-Point Exception signal (SIGFPE) is raised.
 * > At that point, a signal handler function (previously identified to the
 * > C runtime) is called.

 * This function is such a handler. This doesn't do much: it just returns
 * control to the point where the signal was raised.
 *
 * \see decNumber's manual, pp. 14–15 and p. 23.
 */
static void signalHandler(int signo) {
  (void)signo;
  signal(SIGFPE, signalHandler); // Re-enable
}

void* decimalInitSystem() {
  if (signal(SIGFPE, signalHandler) == SIG_ERR) {
    // Maybe, setting signal handling is disabled?
    // TODO: should we do something here?
  }
  // TODO: Check endianness (with decContextTestEndian())?
  return decimalContextCreate();
}

/**
 * \brief Initializes or resets a decimal context to its default.
 *
 * \param context A non-null pointer to a decima context
 *
 * \return \a context
 */
static decContext* initDefaultContext(decContext* context) {
  decContextDefault(context, DEC_INIT_BASE);
  context->digits = DECNUMDIGITS;
  context->traps |= (DEC_Conversion_syntax   |
                     DEC_Division_by_zero    |
                     DEC_Division_undefined  |
                     DEC_Division_impossible |
                     DEC_Insufficient_storage|
                     DEC_Invalid_context     |
                     DEC_Invalid_operation   );
  return context;
}

void* decimalContextCreate() {
  decContext* context = sqlite3_malloc(sizeof(decContext));
  initDefaultContext(context);
  return context;
}

void decimalContextCopy(void* target, void* source) {
  *(decContext*)target = *(decContext*)source;
}

void decimalContextDestroy(void* context) {
  if (context) sqlite3_free((decContext*)context);
}

#pragma mark Helper functions for context virtual table

int decimalPrecision(void* decCtx) {
  return ((decContext*)decCtx)->digits;
}

int decimalMaxExp(void* decCtx) {
  return ((decContext*)decCtx)->emax;
}

int decimalMinExp(void* decCtx) {
  return ((decContext*)decCtx)->emin;
}

int decimalSetPrecision(void* decCtx, int new_prec, char** zErrMsg) {
  if (new_prec <= 0) {
    *zErrMsg = sqlite3_mprintf("Precision must be positive");
    return SQLITE_ERROR;
  }
  else if (new_prec > DECNUMDIGITS) {
    *zErrMsg = sqlite3_mprintf("Maximum allowed precision is %d\n", DECNUMDIGITS);
    return SQLITE_ERROR;
  }
  else if (new_prec % 3 != 0) {
    *zErrMsg = sqlite3_mprintf("Precision must be a multiple of three");
    return SQLITE_ERROR;
  }
  ((decContext*)decCtx)->digits = new_prec;
  return SQLITE_OK;
}

int decimalSetMaxExp(void* decCtx, int new_exp, char** zErrMsg) {
  if (new_exp <= 0 || new_exp > 999999999) {
    *zErrMsg = sqlite3_mprintf("Exponent value out of range");
    return SQLITE_ERROR;
  }
  ((decContext*)decCtx)->emax = new_exp;
  return SQLITE_OK;
}

int decimalSetMinExp(void* decCtx, int new_exp, char** zErrMsg) {
  if (new_exp >= 0 || new_exp < -999999999) {
    *zErrMsg = sqlite3_mprintf("Exponent value out of range");
    return SQLITE_ERROR;
  }
  ((decContext*)decCtx)->emin = new_exp;
  return SQLITE_OK;
}

/**
 * \brief Supported rounding modes.
 *
 * The supported rounding modes are:
 *
 * - `ROUND_CEILING`: round towards +∞.
 * - `ROUND_DOWN`: round towards 0.
 * - `ROUND_FLOOR`: round towards -∞.
 * - `ROUND_HALF_DOWN`: round to nearest; if equidistant, round down.
 * - `ROUND_HALF_EVEN`: round to nearest; if equidistant, round so that the
 *                       final digit is even.
 * - `ROUND_HALF_UP`: round to nearest; if equidistant, round up.
 * - `ROUND_UP`: round away from 0.
 * - `ROUND_05UP`: the same as `ROUND_UP`, except that rounding up only
 *                  occurs if the digit to be rounded is 0 or 5 and after.
 **/
static const struct {
  enum rounding value;
  char const* name;
} rounding_mode[DEC_ROUND_MAX] = {
  {DEC_ROUND_CEILING,   "ROUND_CEILING"},
  {DEC_ROUND_UP,        "ROUND_UP"},
  {DEC_ROUND_HALF_UP,   "ROUND_HALF_UP"},
  {DEC_ROUND_HALF_EVEN, "ROUND_HALF_EVEN"},
  {DEC_ROUND_HALF_DOWN, "ROUND_HALF_DOWN"},
  {DEC_ROUND_DOWN,      "ROUND_DOWN"},
  {DEC_ROUND_FLOOR,     "ROUND_FLOOR"},
  {DEC_ROUND_05UP,      "ROUND_05UP"},
};

static char const* roundingModeToString(enum rounding round) {
  return rounding_mode[round].name;
}

static enum rounding roundingModeToEnum(char const* round) {
  if (strncmp(round, "DEFAULT", strlen("DEFAULT")) == 0)
    return DEC_ROUND_HALF_UP; // ANSI X3.274
  for (size_t i = 0; i < DEC_ROUND_MAX; i++) {
    if (strncmp(round, rounding_mode[i].name, strlen(rounding_mode[i].name)) == 0)
      return rounding_mode[i].value;
  }
  return DEC_ROUND_MAX;
}

char const* decimalRoundingMode(void* decCtx) {
  enum rounding mode = decContextGetRounding(decCtx);
  return roundingModeToString(mode);
}

int decimalSetRoundingMode(void* decCtx, char const* new_mode, char** zErrMsg) {
  enum rounding mode = roundingModeToEnum(new_mode);
  if (mode == DEC_ROUND_MAX) {
    *zErrMsg = sqlite3_mprintf("Invalid rounding mode");
    return SQLITE_ERROR;
  }
  decContextSetRounding(decCtx, mode);
  return SQLITE_OK;
}

#pragma mark Helper functions for status and traps virtual tables

#define FLAGS_MAX 13

/**
 * \brief decNumber's flags.
 *
 * Of the following flags, DEC_Clamped, DEC_Rounded and DEC_Subnormal are never
 * set by decNumber.
 */
static const struct {
  uint32_t flag;
  char const* name;
} extended_flags[FLAGS_MAX] = {
  {DEC_Clamped,              "Clamped"},
  {DEC_Conversion_syntax,    "Conversion syntax"},
  {DEC_Division_by_zero,     "Division by zero"},
  {DEC_Division_impossible,  "Division impossible"},
  {DEC_Division_undefined,   "Division undefined"},
  {DEC_Inexact,              "Inexact result"},
  {DEC_Insufficient_storage, "Out of memory"},
  {DEC_Invalid_context,      "Invalid context"},
  {DEC_Invalid_operation,    "Invalid operation"},
  {DEC_Overflow,             "Overflow"},
  {DEC_Rounded,              "Rounded result"},
  {DEC_Subnormal,            "Subnormal"},
  {DEC_Underflow,            "Underflow"},
};

static uint32_t flag_string_to_int(char const* flag) {
  for (size_t i = 0; i < FLAGS_MAX; ++i) {
    if (strncmp(flag, extended_flags[i].name, strlen(extended_flags[i].name)) == 0)
      return extended_flags[i].flag;
  }
  return FLAGS_MAX;
}

int decimalClearFlag(void* decCtx, char const* flag, char** zErrMsg) {
  uint32_t status = flag_string_to_int(flag);
  if (status == FLAGS_MAX) {
    *zErrMsg = sqlite3_mprintf("Invalid flag: %s", flag);
    return SQLITE_ERROR;
  }
  decContextClearStatus(decCtx, status);
  return SQLITE_OK;
}

int decimalSetFlag(void* decCtx, char const* flag, char** zErrMsg) {
  uint32_t status = flag_string_to_int(flag);
  if (status == FLAGS_MAX) {
    *zErrMsg = sqlite3_mprintf("Invalid flag: %s", flag);
    return SQLITE_ERROR;
  }
  decContextSetStatusQuiet(decCtx, status);
  return SQLITE_OK;
}

char const* decimalGetFlag(void* decCtx, size_t n) {
  size_t k = n + 1;
  for (size_t i = 0; i < FLAGS_MAX; ++i) {
    if (decContextTestStatus(decCtx, extended_flags[i].flag)) --k;
    if (k == 0) return extended_flags[i].name;
  }
  return 0;
}

int decimalClearTrap(void* decCtx, char const* flag, char** zErrMsg) {
  uint32_t status = flag_string_to_int(flag);
  if (status == FLAGS_MAX) {
    *zErrMsg = sqlite3_mprintf("Invalid flag: %s", flag);
    return SQLITE_ERROR;
  }
  ((decContext*)decCtx)->traps &= ~status;
  return SQLITE_OK;
}

int decimalSetTrap(void* decCtx, char const* flag, char** zErrMsg) {
  uint32_t status = flag_string_to_int(flag);
  if (status == FLAGS_MAX) {
    *zErrMsg = sqlite3_mprintf("Invalid flag: %s", flag);
    return SQLITE_ERROR;
  }
  ((decContext*)decCtx)->traps |= status;
  return SQLITE_OK;
}

char const* decimalGetTrap(void* decCtx, size_t n) {
  size_t k = n + 1;
  for (size_t i = 0; i < FLAGS_MAX; ++i) {
    if (((decContext*)decCtx)->traps & extended_flags[i].flag) --k;
    if (k == 0) return extended_flags[i].name;
  }
  return 0;
}

#pragma mark Nullary functions

void decimalClearStatus(sqlite3_context* context) {
  decContext* sharedCtx = sqlite3_user_data(context);
  decContextZeroStatus(sharedCtx);
}

void decimalStatus(sqlite3_context* context) {
  decContext* sharedCtx = sqlite3_user_data(context);
  uint32_t status = decContextGetStatus(sharedCtx);
  char* res = sqlite3_mprintf("%08x", status);
  sqlite3_result_text(context, res, -1, SQLITE_TRANSIENT);
  sqlite3_free(res);
}

void decimalVersion(sqlite3_context* context) {
  // decNumberVersion() returns at most 16 characters (decNumber manual, p. 47)
  char version[sizeof(SQLITE_DECIMAL_VERSION) + 16 + 4] = SQLITE_DECIMAL_VERSION;
  strcat(version, " (");
  strcat(version, decNumberVersion());
  strcat(version, ")");
  sqlite3_result_text(context, version, -1, SQLITE_TRANSIENT);
}

#pragma mark . → Dec

void decimalCreate(sqlite3_context* context, sqlite3_value* value) {
  decNumber decnum;
  decContext* decCtx = sqlite3_user_data(context);
  if (decode(&decnum, decCtx, value, context))
    decNumberToSQLite3Blob(context, &decnum);
}

#pragma mark Dec → Dec

#define SQLITE_DECIMAL_OP1(fun, op)                                     \
  void decimal ## fun(sqlite3_context* context, sqlite3_value* value) { \
    decNumber decnum;                                                   \
    decContext* decCtx = sqlite3_user_data(context);                    \
    if (decode(&decnum, decCtx, value, context)) {                      \
      decNumber result;                                                 \
      op(&result, &decnum, decCtx);                                     \
      if (checkStatus(context, decCtx, decCtx->traps)) {                \
        decNumberToSQLite3Blob(context, &result);                       \
      }                                                                 \
    }                                                                   \
  }

SQLITE_DECIMAL_OP1(Abs,        decNumberAbs)
// calls decCheckMath()
SQLITE_DECIMAL_OP1(Exp,        decNumberExp)
SQLITE_DECIMAL_OP1(NextDown,   decNumberNextMinus)
SQLITE_DECIMAL_OP1(NextUp,     decNumberNextPlus)
SQLITE_DECIMAL_OP1(Invert,     decNumberInvert)
// calls decCheckMath()
SQLITE_DECIMAL_OP1(Ln,         decNumberLn)
// calls decCheckMath()
SQLITE_DECIMAL_OP1(Log10,      decNumberLog10)
SQLITE_DECIMAL_OP1(LogB,       decNumberLogB)
SQLITE_DECIMAL_OP1(Minus,      decNumberMinus)
SQLITE_DECIMAL_OP1(Plus,       decNumberPlus)
SQLITE_DECIMAL_OP1(Reduce,     decNumberReduce)
SQLITE_DECIMAL_OP1(Sqrt,       decNumberSquareRoot)
SQLITE_DECIMAL_OP1(ToIntegral, decNumberToIntegralValue)

#pragma mark Dec × Dec → Dec

#define SQLITE_DECIMAL_OP2(fun, op)                                                             \
  void decimal ## fun(sqlite3_context* context, sqlite3_value* value1, sqlite3_value* value2) { \
    decNumber d1;                                                                               \
    decNumber d2;                                                                               \
    decContext* decCtx = sqlite3_user_data(context);                                            \
    if (decode(&d1, decCtx, value1, context) && decode(&d2, decCtx, value2, context)) {         \
      decNumber result;                                                                         \
      op(&result, &d1, &d2, decCtx);                                                            \
      if (checkStatus(context, decCtx, decCtx->traps)) {                                        \
        decNumberToSQLite3Blob(context, &result);                                               \
      }                                                                                         \
    }                                                                                           \
  }

SQLITE_DECIMAL_OP2(And,           decNumberAnd)
SQLITE_DECIMAL_OP2(Divide,        decNumberDivide)
SQLITE_DECIMAL_OP2(DivideInteger, decNumberDivideInteger)
SQLITE_DECIMAL_OP2(Or,            decNumberOr)
// calls decCheckMath()
SQLITE_DECIMAL_OP2(Power,         decNumberPower)
SQLITE_DECIMAL_OP2(Quantize,      decNumberQuantize)
SQLITE_DECIMAL_OP2(Remainder,     decNumberRemainder)
SQLITE_DECIMAL_OP2(Rotate,        decNumberRotate)
SQLITE_DECIMAL_OP2(ScaleB,        decNumberScaleB)
SQLITE_DECIMAL_OP2(Shift,         decNumberShift)
SQLITE_DECIMAL_OP2(Subtract,      decNumberSubtract)
SQLITE_DECIMAL_OP2(Xor,           decNumberXor)

#pragma mark Dec → Text

void decimalBytes(sqlite3_context* context, sqlite3_value* value) {
  switch (sqlite3_value_type(value)) {
    case SQLITE_BLOB:
      break;
    case SQLITE_TEXT:
    case SQLITE_INTEGER:
    default:
      sqlite3_result_error(context, "Currently, only blob arguments are accepted", -1);
      return;
  }
  char* const hexes = sqlite3_malloc(3 * DECINF_MAXSIZE);
  if (hexes) {
    size_t length = sqlite3_value_bytes(value);
    if (length > DECINF_MAXSIZE) {
      sqlite3_result_error(context, "Encoding too long", -1);
      return;
    }
    uint8_t const* const bytes = sqlite3_value_blob(value);
    decInfiniteToBytes(length, bytes, hexes);
    sqlite3_result_text(context, hexes, -1, SQLITE_TRANSIENT);
    sqlite3_free(hexes);
  }
  else
    sqlite3_result_error_nomem(context);
}

void decimalBits(sqlite3_context* context, sqlite3_value* value) {
  switch (sqlite3_value_type(value)) {
    case SQLITE_BLOB:
      break;
    case SQLITE_TEXT:
    case SQLITE_INTEGER:
    default:
      sqlite3_result_error(context, "Currently, only blob arguments are accepted", -1);
      return;
  }
  char* const bits = sqlite3_malloc(9 * DECINF_MAXSIZE + 1); // Loose upper bound
  if (bits) {
    size_t length = sqlite3_value_bytes(value);
    if (length > DECINF_MAXSIZE) {
      sqlite3_result_error(context, "Encoding too long", -1);
      return;
    }
    uint8_t const* const bytes = sqlite3_value_blob(value);
    decInfiniteToBits(length, bytes, bits);
    sqlite3_result_text(context, bits, -1, SQLITE_TRANSIENT);
    sqlite3_free(bits);
  }
  else
    sqlite3_result_error_nomem(context);
}

// FIXME
void decimalGetCoefficient(sqlite3_context* context, sqlite3_value* value) {
  decNumber decnum;
  decContext* decCtx = sqlite3_user_data(context);
  if (decode(&decnum, decCtx, value, context)) {
    // bcd_coeff is an array of DECNUMDIGITS elements, one digit in each byte
    // (BCD8 encoding); the first (most significant) digit is ignored if the
    // result will be a NaN; all are ignored if the result is finite.
    // All bytes must be in the range 0-9 (decNumber's mmanual, p. 56).
    uint8_t bcd_coeff[DECNUMDIGITS];
    // No error is possible from decNumberGetCoefficient(), and no status is set
    //decNumberGetCoefficient(&decnum, &bcd_coeff[0]);
    char digits[DECNUMDIGITS + 1];
    for (size_t i = 0; i < DECNUMDIGITS; ++i)
      sprintf(&digits[i], "%01d", bcd_coeff[i]);
    digits[DECNUMDIGITS] = '\0';
    sqlite3_result_text(context, digits, -1, SQLITE_TRANSIENT);
  }
}

void decimalToString(sqlite3_context* context, sqlite3_value* value) {
  decNumber decnum;
  decContext* decCtx = sqlite3_user_data(context);
  if (decode(&decnum, decCtx, value, context)) {
    // Converting a decNumber to a string requires a buffer
    // DECQUAD_String + 14 bytes long.
    char s[DECNUMDIGITS + 14];
    // No error is possible from decNumberToString() and decNumberTrim(), and
    // no status is set
    decNumberToString(decNumberTrim(&decnum), s);
    sqlite3_result_text(context, s, -1, SQLITE_TRANSIENT);
  }
}

/**
 * \brief Returns the class of a decimal as text.
 *
 * The possible values are:
 *
 * - `+Normal`: positive normal.
 * - `-Normal`: negative normal.
 * - `+Zero`: positive zero.
 * - `-Zero`: negative zero.
 * - `+Infinity`: +∞.
 * - `-Infinity`: -∞.
 * - `+Subnormal`: positive subnormal.
 * - `-Subnormal`: negative subnormal.
 * - `NaN`: NaN.
 */
void decimalClass(sqlite3_context* context, sqlite3_value* value) {
  decNumber decnum;
  decContext* decCtx = sqlite3_user_data(context);
  if (decode(&decnum, decCtx, value, context))
    sqlite3_result_text(context, decNumberClassToString(decNumberClass(&decnum, decCtx)), -1, SQLITE_STATIC);
}

#pragma mark Dec → Bool

#define SQLITE_DECIMAL_INT1(fun, op)                                    \
  void decimal ## fun(sqlite3_context* context, sqlite3_value* value) { \
    decNumber decnum;                                                   \
    decContext* decCtx = sqlite3_user_data(context);                    \
    if (decode(&decnum, decCtx, value, context)) {                      \
      int result = op(&decnum);                                         \
      sqlite3_result_int(context, result);                              \
    }                                                                   \
  }

#define decNumberIsPositive(d) \
  (!decNumberIsNegative(d) && !decNumberIsZero(d) && !decNumberIsNaN(d))

SQLITE_DECIMAL_INT1(IsCanonical, decNumberIsCanonical)
SQLITE_DECIMAL_INT1(IsFinite,    decNumberIsFinite)
SQLITE_DECIMAL_INT1(IsInfinite,  decNumberIsInfinite)
//SQLITE_DECIMAL_INT1(IsInteger,   decNumberIsInteger)
//SQLITE_DECIMAL_INT1(IsLogical,   decNumberIsLogical)
SQLITE_DECIMAL_INT1(IsNaN,       decNumberIsNaN)
SQLITE_DECIMAL_INT1(IsNegative,  decNumberIsNegative)
SQLITE_DECIMAL_INT1(IsPositive,  decNumberIsPositive)
//SQLITE_DECIMAL_INT1(IsSigned,    decNumberIsSigned)
SQLITE_DECIMAL_INT1(IsZero,      decNumberIsZero)

#pragma mark Dec × Dec → Bool

#define SQLITE_DECIMAL_INT1_CTX(fun, op)                                \
  void decimal ## fun(sqlite3_context* context, sqlite3_value* value) { \
    decNumber decnum;                                                   \
    decContext* decCtx = sqlite3_user_data(context);                    \
    if (decode(&decnum, decCtx, value, context)) {                      \
      int result = op(&decnum, decCtx);                                 \
      sqlite3_result_int(context, result);                              \
    }                                                                   \
  }

SQLITE_DECIMAL_INT1_CTX(IsNormal,          decNumberIsNormal)
SQLITE_DECIMAL_INT1_CTX(IsSubnormal, decNumberIsSubnormal)

#pragma mark Dec → Int

void decimalToInt32(sqlite3_context* context, sqlite3_value* value) {
  decNumber decnum;
  decContext* decCtx = sqlite3_user_data(context);
  if (decode(&decnum, decCtx, value, context)) {
    // Need to trim trailing zeroes to make the exponent equal to zero (if the
    // decimal is integer).
    int result = decNumberToInt32(decNumberTrim(&decnum), decCtx);
    if (checkStatus(context, decCtx, decCtx->traps))
      sqlite3_result_int(context, result);
  }
}

#pragma mark Dec × Dec → Int

#define SQLITE_DECIMAL_CMP(fun, cmp)                                                    \
  void decimal ## fun(sqlite3_context* context, sqlite3_value* v1, sqlite3_value* v2) { \
    decNumber x;                                                                        \
    decNumber y;                                                                        \
    decContext* decCtx = sqlite3_user_data(context);                                    \
    if (decode(&x, decCtx, v1, context) && decode(&y, decCtx, v2, context)) {           \
      decNumber result;                                                                 \
      decNumberCompare(&result, &x, &y, decCtx);                                        \
      sqlite3_result_int(context, cmp(&result));                                        \
    }                                                                                   \
  }                                                                                     \

#define decNumberIsNegativeOrZero(d) (decNumberIsNegative(d) || decNumberIsZero(d))

SQLITE_DECIMAL_CMP(Equal,              decNumberIsZero)
SQLITE_DECIMAL_CMP(GreaterThan,        decNumberIsPositive)
SQLITE_DECIMAL_CMP(GreaterThanOrEqual, !decNumberIsNegativeOrZero)
SQLITE_DECIMAL_CMP(LessThan,           decNumberIsNegative)
SQLITE_DECIMAL_CMP(LessThanOrEqual,    decNumberIsNegativeOrZero)
SQLITE_DECIMAL_CMP(NotEqual,           !decNumberIsZero)

void decimalCompare(sqlite3_context* context, sqlite3_value* v1, sqlite3_value* v2) {
  decNumber x;
  decNumber y;
  decContext* decCtx = sqlite3_user_data(context);
  if (decode(&x, decCtx, v1, context) && decode(&y, decCtx, v2, context)) {
    decNumber result;
    // If an argument is a NaN, the DEC_Invalid_operation flag is set.
    decNumberCompareSignal(&result, &x, &y, decCtx);
    if (checkStatus(context, decCtx, decCtx->traps))
     sqlite3_result_int(context, decNumberToInt32(&result, decCtx));
  }
}

void decimalSameQuantum(sqlite3_context* context, sqlite3_value* v1, sqlite3_value* v2) {
  decNumber x;
  decNumber y;
  decContext* decCtx = sqlite3_user_data(context);
  if (decode(&x, decCtx, v1, context) && decode(&y, decCtx, v2, context)) {
    decNumber result;
    decNumberSameQuantum(&result, &x, &y);
    sqlite3_result_int(context, decNumberToUInt32(&result, decCtx));
  }
}


#pragma mark Dec × Dec × Dec → Dec

/**
 * \brief Calculates the fused multiply-add `v1 × v2 + v3`.
 */
void decimalFMA(sqlite3_context* context, sqlite3_value* v1, sqlite3_value* v2, sqlite3_value* v3) {
  decNumber x;
  decNumber y;
  decNumber z;
  decContext* decCtx = sqlite3_user_data(context);
  if (decode(&x, decCtx, v1, context) && decode(&y, decCtx, v2, context) && decode(&z, decCtx, v3, context)) {
    decNumber result;
    decNumberFMA(&result, &x, &y, &z, decCtx);
    if (checkStatus(context, decCtx, decCtx->traps))
      decNumberToSQLite3Blob(context, &result);
  }
}

#pragma mark Dec × ⋯ × Dec → Dec

#define SQLITE_DECIMAL_OPn(fun, op, defaultValue)                                 \
  void decimal ## fun(sqlite3_context* context, int argc, sqlite3_value** argv) { \
    decContext* decCtx = sqlite3_user_data(context);                              \
    decNumber result;                                                             \
    if (argc == 0) {                                                              \
      defaultValue(&result, decCtx);                                              \
      decNumberToSQLite3Blob(context, &result);                                   \
      return;                                                                     \
    }                                                                             \
    if (decode(&result, decCtx, argv[0], context)) {                              \
      for (int i = 1; i < argc; i++) {                                            \
        decNumber decnum;                                                         \
        if (decode(&decnum, decCtx, argv[i], context)) {                          \
          op(&result, &result, &decnum, decCtx);                                  \
        }                                                                         \
        else {                                                                    \
          return;                                                                 \
        }                                                                         \
      }                                                                           \
      if (checkStatus(context, decCtx, decCtx->traps)) {                          \
        decNumberToSQLite3Blob(context, &result);                                 \
      }                                                                           \
    }                                                                             \
  }

static void decimalAddDefault(decNumber* decnum, decContext* decCtx) {
  (void)decCtx;
  decNumberZero(decnum);
}

static void decimalMaxDefault(decNumber* decnum, decContext* decCtx) {
  (void)decCtx;
  uint8_t mantissa[DECNUMDIGITS];
  for (size_t i = 0; i < DECNUMDIGITS; ++i)
    mantissa[i] = 0x99;
  decPackedToNumber(&mantissa[0], DECNUMDIGITS, &(decCtx->emax), decnum);
}

static void decimalMinDefault(decNumber* decnum, decContext* decCtx) {
  (void)decCtx;
  uint8_t mantissa[DECNUMDIGITS];
  for (size_t i = 0; i < DECNUMDIGITS; i++)
    mantissa[i] = 0x99;
  decPackedToNumber(&mantissa[0], DECNUMDIGITS, &(decCtx->emin), decnum);
}

static void decimalMultiplyDefault(decNumber* decnum, decContext* decCtx) {
  decNumberFromString(decnum, "1", decCtx);
}

SQLITE_DECIMAL_OPn(Add,      decNumberAdd,      decimalAddDefault)
SQLITE_DECIMAL_OPn(Max,      decNumberMax,      decimalMaxDefault)
SQLITE_DECIMAL_OPn(MaxMag,   decNumberMaxMag,   decimalMaxDefault)
SQLITE_DECIMAL_OPn(Min,      decNumberMin,      decimalMinDefault)
SQLITE_DECIMAL_OPn(MinMag,   decNumberMinMag,   decimalMinDefault)
SQLITE_DECIMAL_OPn(Multiply, decNumberMultiply, decimalMultiplyDefault)

/**
 * \brief Holds the cumulative sum computed by decSum().
 */
typedef struct AggregateData AggregateData;

struct AggregateData {
  decNumber value;
  decContext* decCtx;
  uint32_t count;
};

#pragma mark Aggregate functions

#define SQLITE_DECIMAL_AGGR_STEP(fun, aggr)                                                          \
  void decimal ## fun ## Step(sqlite3_context* context, int argc, sqlite3_value** argv) {            \
    (void)argc;                                                                                      \
    AggregateData* data = (AggregateData*)sqlite3_aggregate_context(context, sizeof(AggregateData)); \
                                                                                                     \
    if (data == 0)                                                                                   \
    return;                                                                                          \
                                                                                                     \
    if (data->count == 0) {                                                                          \
      data->decCtx = sqlite3_user_data(context);                                                     \
      if (decode(&(data->value), data->decCtx, argv[0], context)) {                                  \
        data->count++;                                                                               \
      }                                                                                              \
    }                                                                                                \
    else {                                                                                           \
      decNumber value;                                                                               \
      if (decode(&value, data->decCtx, argv[0], context)) {                                          \
        aggr(&(data->value), &(data->value), &value, data->decCtx);                                  \
        data->count++;                                                                               \
      }                                                                                              \
    }                                                                                                \
  }

SQLITE_DECIMAL_AGGR_STEP(Sum, decNumberAdd)
SQLITE_DECIMAL_AGGR_STEP(Min, decNumberMin)
SQLITE_DECIMAL_AGGR_STEP(Max, decNumberMax)
SQLITE_DECIMAL_AGGR_STEP(Avg, decNumberAdd)

#define SQLITE_DECIMAL_AGGR_FINAL(fun, defaultValue, finalize)                                       \
  void decimal ## fun ## Final(sqlite3_context* context) {                                           \
    AggregateData* data = (AggregateData*)sqlite3_aggregate_context(context, sizeof(AggregateData)); \
                                                                                                     \
    if (data == 0) return;                                                                           \
                                                                                                     \
    if (data->count == 0) {                                                                          \
      data->decCtx = sqlite3_user_data(context);                                                     \
      defaultValue(data);                                                                            \
    }                                                                                                \
                                                                                                     \
    finalize(data);                                                                                  \
                                                                                                     \
    if (checkStatus(context, data->decCtx, data->decCtx->traps))                                     \
    decNumberToSQLite3Blob(context, &(data->value));                                                 \
  }

static void sumAggrDefault(AggregateData* data) {
  decNumberZero(&(data->value));
}

static void maxAggrDefault(AggregateData* data) {
  decimalMaxDefault(&(data->value), data->decCtx);
}

static void minAggrDefault(AggregateData* data) {
  decimalMinDefault(&(data->value), data->decCtx);
}

static void avgAggrDefault(AggregateData* data) {
  decNumberFromString(&(data->value), "NaN", data->decCtx);
}

static void avgAggrFinOp(AggregateData* data) {
  decNumber count;
  decNumberFromUInt32(&count, data->count);
  decNumberDivide(&(data->value), &(data->value), &count, data->decCtx);
}

static void aggrNoOp(AggregateData* data) {
  (void)data;
}

SQLITE_DECIMAL_AGGR_FINAL(Sum, sumAggrDefault, aggrNoOp)
SQLITE_DECIMAL_AGGR_FINAL(Max, maxAggrDefault, aggrNoOp)
SQLITE_DECIMAL_AGGR_FINAL(Min, minAggrDefault, aggrNoOp)
SQLITE_DECIMAL_AGGR_FINAL(Avg, avgAggrDefault, avgAggrFinOp)

#pragma mark Not implemented

#define SQLITE_DECIMAL_NOT_IMPL1(fun)                                   \
  void decimal ## fun(sqlite3_context* context, sqlite3_value* value) { \
    (void)context;                                                      \
    (void)value;                                                        \
    sqlite3_result_error(context, "Operation not implemented", -1);     \
    return;                                                             \
  }

#define SQLITE_DECIMAL_NOT_IMPL2(fun)                                                   \
  void decimal ## fun(sqlite3_context* context, sqlite3_value* v1, sqlite3_value* v2) { \
    (void)context;                                                                      \
    (void)v1;                                                                           \
    (void)v2;                                                                           \
    sqlite3_result_error(context, "Operation not implemented", -1);                     \
    return;                                                                             \
  }

SQLITE_DECIMAL_NOT_IMPL1(Digits)
SQLITE_DECIMAL_NOT_IMPL1(GetExponent)
// Not implemented by decNumber
SQLITE_DECIMAL_NOT_IMPL1(IsInteger)
// Not implemented by decNumber
SQLITE_DECIMAL_NOT_IMPL1(IsLogical)
// Not implemented by decNumber
SQLITE_DECIMAL_NOT_IMPL1(IsSigned)
// Not implemented by decNumber
SQLITE_DECIMAL_NOT_IMPL1(ToInt64)
// Unnecessary
SQLITE_DECIMAL_NOT_IMPL1(Trim)

