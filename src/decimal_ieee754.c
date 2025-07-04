/**
 * \file      decimal_ieee754.c
 * \author    Lifepillar
 * \copyright Copyright (c) 2025 Lifepillar.
 *            This program is free software; you can redistribute it and/or
 *            modify it under the terms of the Simplified BSD License (also
 *            known as the "2-Clause License" or "FreeBSD License".)
 * \copyright This program is distributed in the hope that it will be useful,
 *            but without any warranty; without even the implied warranty of
 *            merchantability or fitness for a particular purpose.
 *
 * \brief     An implementation based on IEEE 754 decimal128 format.
 */

#include <signal.h>
#include <string.h>

#include "sqlite3ext.h"
#include "decimal.h"
#include "decNumber/decQuad.h"
#include "decNumber/decimal128.h"

SQLITE_EXTENSION_INIT3


#pragma mark Helper functions

#if DEBUG
/**
 ** @brief Returns the i-th bit of a `decQuad` number.
 **
 ** Bit 0 is the MSB, no matter what the endianness is.
 **/
static char bit(size_t i, decQuad const* decnum) {
#if HAVE_LITTLE_ENDIAN
  return '0' + ((decnum->bytes[DECQUAD_Bytes - 1 - i / 8] >> (7 - i % 8)) & 0x01);
#else
  return '0' + ((decnum->bytes[i / 8] >> (7 - i % 8)) & 0x01);
#endif
}
#endif

/**
 ** @brief Copies `count` bytes from `src` to `dest` in reverse order.
 **/
#if !HAVE_LITTLE_ENDIAN
static void
reverse_bytes(size_t count, uint8_t dest[count], uint8_t const src[count]) {
  for (size_t i = 0; i < count; ++i) {
    dest[i] = src[count - 1 - i];
  }
}
#endif

/**
 * \brief Encodes a decimal value.
 *
 * \param context SQLite3 context
 * \param decnum A decimal value to encode
 */
static inline void decQuadToSQLite3Blob(sqlite3_context* context, decQuad* decnum) {
#if HAVE_LITTLE_ENDIAN
  sqlite3_result_blob(context, decnum->bytes, DECQUAD_Bytes, SQLITE_TRANSIENT);
#else // Big-endian
  uint8_t buf[DECQUAD_Bytes];
  reverse_bytes(DECQUAD_Bytes, &buf, decnum->bytes);
  sqlite3_result_blob(context, &buf, DECQUAD_Bytes, SQLITE_TRANSIENT);
#endif
}

/**
 * \brief Builds a decimal from a blob field.
 *
 * Decimals are stored using the IEEE decimal128 format. If the blob does not
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
 * \see decQuad.h
 **/
static inline void decQuadFromSQLite3Blob(
    decQuad*         result,
    sqlite3_value*   value,
    [[maybe_unused]] decContext* decCtx
    ) {
  if (sqlite3_value_bytes(value) == DECQUAD_Bytes) {
    uint8_t const* bytes = sqlite3_value_blob(value);
#if HAVE_LITTLE_ENDIAN
    memcpy(&result->bytes, bytes, DECQUAD_Bytes);
#else
    reverse_bytes(DECQUAD_Bytes, result->bytes, bytes);
#endif
  }
  else {
    decContextSetStatusQuiet(decCtx, DEC_Conversion_syntax);
    decQuadFromString(result, "NaN", decCtx);
  }
}


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
static inline void decQuadFromSQLite3Text(
    decQuad*       result,
    sqlite3_value* value,
    decContext*    decCtx
    ) {
  char const* const string_num = (char*)sqlite3_value_text(value);

  decQuadFromString(result, string_num, decCtx);  // TODO: check for status flags
}

/**
 * \brief Creates a decimal from an integer.
 *
 * \param result The output decimal
 * \param value A value of type `SQLITE_INTEGER`
 *
 * \return \a result
 **/
static inline void decQuadFromSQLite3Integer(
    decQuad*       result,
    sqlite3_value* value
    ) {
  int32_t n = sqlite3_value_int(value);

  decQuadFromInt32(result, n); // Result is exact; no error is possible
}

/**
 * \brief Checks decNumber's context status and interrupts the current SQL
 *        operation with an error if any of the mask bit is set.
 *
 * \param sqlCtx SQLite3's context
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
static int decode(decQuad* decnum, decContext* decCtx, sqlite3_value* value, sqlite3_context* sqlCtx) {
  switch (sqlite3_value_type(value)) {
    case SQLITE_BLOB:
      decQuadFromSQLite3Blob(decnum, value, decCtx);
      break;
    case SQLITE_TEXT:
      decQuadFromSQLite3Text(decnum, value, decCtx);
      break;
    case SQLITE_INTEGER:
      decQuadFromSQLite3Integer(decnum, value);
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
 * \see decNumber's manual, pp. 14-15 and p. 23.
 */
static void signalHandler([[maybe_unused]] int signo) {
  signal(SIGFPE, signalHandler); // Re-enable
}

void* decimalInitSystem(void) {
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
  decContextDefault(context, DEC_INIT_DECQUAD);
  context->traps |= (DEC_Conversion_syntax   |
                     DEC_Division_by_zero    |
                     DEC_Division_undefined  |
                     DEC_Division_impossible |
                     DEC_Insufficient_storage|
                     DEC_Invalid_context     |
                     DEC_Invalid_operation   );
  return context;
}

void* decimalContextCreate(void) {
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

int decimalSetPrecision([[maybe_unused]] void* decCtx, int new_prec, char** zErrMsg) {
  if (new_prec != DECQUAD_Pmax) {
    *zErrMsg = sqlite3_mprintf("The only allowed precision is %d\n", DECQUAD_Pmax);
    return SQLITE_ERROR;
  }

  return SQLITE_OK;
}

int decimalSetMaxExp([[maybe_unused]] void* decCtx, int new_exp, char** zErrMsg) {
  if (new_exp != DECQUAD_Emax) {
    *zErrMsg = sqlite3_mprintf("The only valid maximum exponent is %d\n", DECQUAD_Emax);
    return SQLITE_ERROR;
  }

  return SQLITE_OK;
}

int decimalSetMinExp([[maybe_unused]] void* decCtx, int new_exp, char** zErrMsg) {
  if (new_exp != DECQUAD_Emin) {
    *zErrMsg = sqlite3_mprintf("The only valid minimum exponent is %d\n", DECQUAD_Emin);
    return SQLITE_ERROR;
  }

  return SQLITE_OK;
}

/**
 * \brief Supported rounding modes.
 *
 * The supported rounding modes are:
 *
 * - `ROUND_CEILING`: round towards +inf.
 * - `ROUND_DOWN`: round towards 0.
 * - `ROUND_FLOOR`: round towards -inf.
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
    return DEC_ROUND_HALF_EVEN; // Default for IEEE 754 decimal128

  for (size_t i = 0; i < DEC_ROUND_MAX; i++) {
    // FIXME: this doesn't check for trailing garbage (e.g., `DEC_ROUND_UPPITY_BOO` is valid)
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
    if (decContextTestStatus(decCtx, extended_flags[i].flag))
      --k;

    if (k == 0)
      return extended_flags[i].name;
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
    if (((decContext*)decCtx)->traps & extended_flags[i].flag)
      --k;

    if (k == 0)
      return extended_flags[i].name;
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
  // decNumberVersion() returns at most 19 characters (see DECNUMBER macro)
  char version[sizeof(SQLITE_DECIMAL_VERSION) + 19 + 4] = SQLITE_DECIMAL_VERSION;
  strcat(version, " (");
  strcat(version, decQuadVersion());
  strcat(version, ")");
  sqlite3_result_text(context, version, -1, SQLITE_TRANSIENT);
}


#pragma mark . -> Dec

void decimalCreate(sqlite3_context* context, sqlite3_value* value) {
  decQuad decnum;
  decContext* decCtx = sqlite3_user_data(context);

  if (decode(&decnum, decCtx, value, context)) {
    decQuadToSQLite3Blob(context, &decnum);
  }
}


#pragma mark Dec -> Dec

#define SQLITE_DECIMAL_OP1(fun, op)                                     \
  void decimal ## fun(sqlite3_context* context, sqlite3_value* value) { \
    decQuad decnum;                                                     \
    decContext* decCtx = sqlite3_user_data(context);                    \
    if (decode(&decnum, decCtx, value, context)) {                      \
      decQuad result;                                                   \
      op(&result, &decnum, decCtx);                                     \
      if (checkStatus(context, decCtx, decCtx->traps)) {                \
        decQuadToSQLite3Blob(context, &result);                         \
      }                                                                 \
    }                                                                   \
  }

#define SQLITE_DECIMAL_OP1_DECNUMBER(fun, op)                           \
  void decimal ## fun(sqlite3_context* context, sqlite3_value* value) { \
    decQuad d;                                                          \
    decContext* decCtx = sqlite3_user_data(context);                    \
    if (decode(&d, decCtx, value, context)) {                           \
      decNumber a;                                                      \
      decQuadToNumber(&d, &a);                                          \
      op(&a, &a, decCtx);                                               \
      decQuadFromNumber(&d, &a, decCtx);                                \
                                                                        \
      if (checkStatus(context, decCtx, decCtx->traps)) {                \
        decQuadToSQLite3Blob(context, &d);                              \
      }                                                                 \
    }                                                                   \
  }

SQLITE_DECIMAL_OP1(Abs,             decQuadAbs)
SQLITE_DECIMAL_OP1(NextDown,        decQuadNextMinus)
SQLITE_DECIMAL_OP1(NextUp,          decQuadNextPlus)
SQLITE_DECIMAL_OP1(Invert,          decQuadInvert)
SQLITE_DECIMAL_OP1(Minus,           decQuadMinus)
SQLITE_DECIMAL_OP1(Plus,            decQuadPlus)
SQLITE_DECIMAL_OP1(Reduce,          decQuadReduce)
SQLITE_DECIMAL_OP1(ToIntegral,      decQuadToIntegralExact)

SQLITE_DECIMAL_OP1_DECNUMBER(Exp,   decNumberExp)
SQLITE_DECIMAL_OP1_DECNUMBER(Ln,    decNumberLn)
SQLITE_DECIMAL_OP1_DECNUMBER(LogB,  decNumberLogB)
SQLITE_DECIMAL_OP1_DECNUMBER(Log10, decNumberLog10)
SQLITE_DECIMAL_OP1_DECNUMBER(Sqrt,  decNumberSquareRoot)

void decimalTrim(sqlite3_context* context, sqlite3_value* value) {
  decQuad d;
  decContext* decCtx = sqlite3_user_data(context);

  if (decode(&d, decCtx, value, context)) {
    decNumber a;
    decQuadToNumber(&d, &a);
    decNumberTrim(&a);
    decQuadFromNumber(&d, &a, decCtx);

    if (checkStatus(context, decCtx, decCtx->traps)) {
      decQuadToSQLite3Blob(context, &d);
    }
  }
}

void decimalIsInteger(sqlite3_context* context, sqlite3_value* value) {
  decQuad d;
  decContext* decCtx = sqlite3_user_data(context);

  if (decode(&d, decCtx, value, context)) {
    uint32_t result = decQuadIsInteger(&d);
    sqlite3_result_int(context, result);
  }
}

void decimalIsLogical(sqlite3_context* context, sqlite3_value* value) {
  decQuad d;
  decContext* decCtx = sqlite3_user_data(context);

  if (decode(&d, decCtx, value, context)) {
    uint32_t result = decQuadIsLogical(&d);
    sqlite3_result_int(context, result);
  }
}

#pragma mark Dec x Dec -> Dec

#define SQLITE_DECIMAL_OP2(fun, op)                                                             \
  void decimal ## fun(sqlite3_context* context, sqlite3_value* value1, sqlite3_value* value2) { \
    decQuad d1;                                                                                 \
    decQuad d2;                                                                                 \
    decContext* decCtx = sqlite3_user_data(context);                                            \
    if (decode(&d1, decCtx, value1, context) && decode(&d2, decCtx, value2, context)) {         \
      decQuad result;                                                                           \
      op(&result, &d1, &d2, decCtx);                                                            \
      if (checkStatus(context, decCtx, decCtx->traps)) {                                        \
        decQuadToSQLite3Blob(context, &result);                                                 \
      }                                                                                         \
    }                                                                                           \
  }

SQLITE_DECIMAL_OP2(And,           decQuadAnd)
SQLITE_DECIMAL_OP2(Divide,        decQuadDivide)
SQLITE_DECIMAL_OP2(DivideInteger, decQuadDivideInteger)
SQLITE_DECIMAL_OP2(Or,            decQuadOr)
SQLITE_DECIMAL_OP2(Quantize,      decQuadQuantize)
SQLITE_DECIMAL_OP2(Remainder,     decQuadRemainder)
SQLITE_DECIMAL_OP2(Rotate,        decQuadRotate)
SQLITE_DECIMAL_OP2(ScaleB,        decQuadScaleB)
SQLITE_DECIMAL_OP2(Shift,         decQuadShift)
SQLITE_DECIMAL_OP2(Subtract,      decQuadSubtract)
SQLITE_DECIMAL_OP2(Xor,           decQuadXor)

void decimalPower(sqlite3_context* context, sqlite3_value* value1, sqlite3_value* value2) {
  decQuad d1;
  decQuad d2;
  decContext* decCtx = sqlite3_user_data(context);

  if (decode(&d1, decCtx, value1, context) && decode(&d2, decCtx, value2, context)) {
    decNumber a;
    decNumber b;
    decQuadToNumber(&d1, &a);
    decQuadToNumber(&d2, &b);
    decNumberPower(&a, &a, &b, decCtx);
    decQuadFromNumber(&d1, &a, decCtx);

    if (checkStatus(context, decCtx, decCtx->traps)) {
      decQuadToSQLite3Blob(context, &d1);
    }
  }
}

#pragma mark Dec -> Text

void decimalToString(sqlite3_context* context, sqlite3_value* value) {
  decQuad decnum;
  decContext* decCtx = sqlite3_user_data(context);

  if (decode(&decnum, decCtx, value, context)) {
    char s[DECQUAD_String + 14];

    decQuadToString(&decnum, s);
    sqlite3_result_text(context, s, -1, SQLITE_TRANSIENT);
  }
}

void decimalBytes(sqlite3_context* context, sqlite3_value* value) {
  switch (sqlite3_value_type(value)) {
    case SQLITE_BLOB:
      break;
    case SQLITE_TEXT:
    case SQLITE_INTEGER:
    default:
      sqlite3_result_error(context, "Only blob arguments are accepted", -1);
      return;
  }

  // Adapted from decFloatShow() in decCommon.c
  uint8_t const* const bytes = sqlite3_value_blob(value);
  char hexbuf[DECQUAD_Bytes * 2 + DECQUAD_Bytes / 4 + 1]; // NB blank after every fourth
                                                          // char buff[DECQUAD_String];                              // For value in decimal
  size_t i;
  size_t j = 0;

  for (i = 0; i < DECQUAD_Bytes; ++i) {
#if HAVE_LITTLE_ENDIAN
    sprintf(&hexbuf[j], "%02x", bytes[DECQUAD_Bytes - 1 - i]);
#else
    sprintf(&hexbuf[j], "%02x", bytes[i]);
#endif
    j += 2;
    // the next line adds blank (and terminator) after final pair, too
    if ((i + 1) % 4 == 0) {
      strcpy(&hexbuf[j], " ");
      ++j;
    }
  }
  hexbuf[--j] = '\0';
  // decQuadToString(df, buff);
  // printf(">%s> %s [big-endian]  %s\n", tag, hexbuf, buff);
  sqlite3_result_text(context, hexbuf, -1, SQLITE_TRANSIENT);
}

// See: IEEE Standard for Floating Point Arithmetic (IEEE754), 2008, p. 22.
#if DEBUG
void decimalBits(sqlite3_context* context, sqlite3_value* value) {
  decQuad decnum;
  decContext* decCtx = sqlite3_user_data(context);

  if (!decode(&decnum, decCtx, value, context)) {
      sqlite3_result_error(context, "Malformed decimal blob", -1);
      return;
  }

  size_t const k = 8 * DECQUAD_Bytes;  // Storage in bits
  size_t const w = (k / 16 + 9) - 5;   // (Size of the combination field) - 5
  size_t const t = 15 * k / 16 - 10;   // Trailing significand field width in bits
  char bits[k + 3 + (t / 10 - 1) + 1]; // t/10 - 1 spaces between declets

  bits[0] = bit(0, &decnum); // Sign bit
  bits[1] = ' ';

  // Combination field (bits 1-17)
  size_t m = 2;

  for (size_t i = 1; i <= 5; ++i) { // bits 1-5
    bits[m] = bit(i, &decnum);
    ++m;
  }

  bits[m] = ' ';
  ++m;

  for (size_t i = 6; i <= 5 + w; ++i) { // bits 6-(w+5)
    bits[m] = bit(i, &decnum);
    ++m;
  }
  bits[m] = ' ';
  ++m;

  // Significand
  for (size_t i = w + 6; i < k; i += 10) { // Group remaining bits in declets
    for (size_t j = 0; j < 10; ++j) {
      bits[m] = bit(i + j, &decnum);
      ++m;
    }
    bits[m] = ' ';
    ++m;
  }

  bits[m - 1] = '\0';

  sqlite3_result_text(context, bits, -1, SQLITE_TRANSIENT);
}
#endif

/**
 * \brief Returns the class of a decimal as text.
 *
 * The possible values are:
 *
 * - `+Normal`: positive normal.
 * - `-Normal`: negative normal.
 * - `+Zero`: positive zero.
 * - `-Zero`: negative zero.
 * - `+Infinity`: +inf.
 * - `-Infinity`: -inf.
 * - `+Subnormal`: positive subnormal.
 * - `-Subnormal`: negative subnormal.
 * - `NaN`: NaN.
 */
void decimalClass(sqlite3_context* context, sqlite3_value* value) {
  decQuad decnum;
  decContext* decCtx = sqlite3_user_data(context);
  if (decode(&decnum, decCtx, value, context))
    sqlite3_result_text(context, decQuadClassString(&decnum), -1, SQLITE_STATIC);
}


#pragma mark Dec -> Bool

#define SQLITE_DECIMAL_INT1(fun, op)                                    \
  void decimal ## fun(sqlite3_context* context, sqlite3_value* value) { \
    decQuad decnum;                                                     \
    decContext* decCtx = sqlite3_user_data(context);                    \
    if (decode(&decnum, decCtx, value, context)) {                      \
      int result = op(&decnum);                                         \
      sqlite3_result_int(context, result);                              \
    }                                                                   \
  }

SQLITE_DECIMAL_INT1(IsCanonical, decQuadIsCanonical)
SQLITE_DECIMAL_INT1(IsFinite,    decQuadIsFinite)
SQLITE_DECIMAL_INT1(IsInfinite,  decQuadIsInfinite)
SQLITE_DECIMAL_INT1(IsNaN,       decQuadIsNaN)
SQLITE_DECIMAL_INT1(IsNegative,  decQuadIsNegative)
SQLITE_DECIMAL_INT1(IsNormal,    decQuadIsNormal)
SQLITE_DECIMAL_INT1(IsPositive,  decQuadIsPositive)
SQLITE_DECIMAL_INT1(IsSigned,    decQuadIsSigned)
SQLITE_DECIMAL_INT1(IsSubnormal, decQuadIsSubnormal)
SQLITE_DECIMAL_INT1(IsZero,      decQuadIsZero)


#pragma mark Dec x Dec -> Bool

  void decimalEqual(sqlite3_context* context, sqlite3_value* v1, sqlite3_value* v2) {
    decQuad x;
    decQuad y;
    decQuad cmp;
    decContext* decCtx = sqlite3_user_data(context);

    if (decode(&x, decCtx, v1, context) && decode(&y, decCtx, v2, context)) {
      uint32_t result = decQuadIsZero(decQuadCompare(&cmp, &x, &y, decCtx));
      sqlite3_result_int(context, result);
    }
  }

void decimalNotEqual(sqlite3_context* context, sqlite3_value* v1, sqlite3_value* v2) {
  decQuad x;
  decQuad y;
  decQuad cmp;
  decContext* decCtx = sqlite3_user_data(context);

  if (decode(&x, decCtx, v1, context) && decode(&y, decCtx, v2, context)) {
    uint32_t result = !decQuadIsZero(decQuadCompare(&cmp, &x, &y, decCtx));
    sqlite3_result_int(context, result);
  }
}

void decimalLessThan(sqlite3_context* context, sqlite3_value* v1, sqlite3_value* v2) {
  decQuad x;
  decQuad y;
  decQuad cmp;
  decContext* decCtx = sqlite3_user_data(context);

  if (decode(&x, decCtx, v1, context) && decode(&y, decCtx, v2, context)) {
    uint32_t result = decQuadIsNegative(decQuadCompare(&cmp, &x, &y, decCtx));
    sqlite3_result_int(context, result);
  }
}

void decimalLessThanOrEqual(sqlite3_context* context, sqlite3_value* v1, sqlite3_value* v2) {
  decQuad x;
  decQuad y;
  decQuad cmp;
  decContext* decCtx = sqlite3_user_data(context);

  if (decode(&x, decCtx, v1, context) && decode(&y, decCtx, v2, context)) {
    decQuadCompare(&cmp, &x, &y, decCtx);
    uint32_t result = !(decQuadIsNaN(&cmp) || decQuadIsPositive(&cmp));
    sqlite3_result_int(context, result);
  }
}

void decimalGreaterThan(sqlite3_context* context, sqlite3_value* v1, sqlite3_value* v2) {
  decQuad x;
  decQuad y;
  decQuad cmp;
  decContext* decCtx = sqlite3_user_data(context);

  if (decode(&x, decCtx, v1, context) && decode(&y, decCtx, v2, context)) {
    uint32_t result = decQuadIsPositive(decQuadCompare(&cmp, &x, &y, decCtx));
    sqlite3_result_int(context, result);
  }
}

void decimalGreaterThanOrEqual(sqlite3_context* context, sqlite3_value* v1, sqlite3_value* v2) {
  decQuad x;
  decQuad y;
  decQuad cmp;
  decContext* decCtx = sqlite3_user_data(context);

  if (decode(&x, decCtx, v1, context) && decode(&y, decCtx, v2, context)) {
    decQuadCompare(&cmp, &x, &y, decCtx);
    uint32_t result = !(decQuadIsNaN(&cmp) || decQuadIsNegative(&cmp));
    sqlite3_result_int(context, result);
  }
}


#pragma mark Dec -> Int

void decimalToInt32(sqlite3_context* context, sqlite3_value* value) {
  decQuad decnum;
  decContext* decCtx = sqlite3_user_data(context);

  if (decode(&decnum, decCtx, value, context)) {
    int result = decQuadToInt32Exact(&decnum, decCtx, decCtx->round);

    if (checkStatus(context, decCtx, decCtx->traps))
      sqlite3_result_int(context, result);
  }
}

void decimalGetExponent(sqlite3_context* context, sqlite3_value* value) {
  decQuad decnum;
  decContext* decCtx = sqlite3_user_data(context);

  if (decode(&decnum, decCtx, value, context)) {
    int32_t exponent = decQuadGetExponent(&decnum);
    sqlite3_result_int(context, exponent);
  }
}


#pragma mark Dec x Dec -> Dec

void decimalCompare(sqlite3_context* context, sqlite3_value* v1, sqlite3_value* v2) {
  decQuad x;
  decQuad y;
  decContext* decCtx = sqlite3_user_data(context);

  if (decode(&x, decCtx, v1, context) && decode(&y, decCtx, v2, context)) {
    decQuad result;
    decQuadCompare(&result, &x, &y, decCtx);
    decQuadToSQLite3Blob(context, &result);
  }
}


#pragma mark Dec x Dec -> Int

void decimalSameQuantum(sqlite3_context* context, sqlite3_value* v1, sqlite3_value* v2) {
  decQuad x;
  decQuad y;
  decContext* decCtx = sqlite3_user_data(context);

  if (decode(&x, decCtx, v1, context) && decode(&y, decCtx, v2, context)) {
    uint32_t result = decQuadSameQuantum(&x, &y);
    sqlite3_result_int(context, result);
  }
}


#pragma mark Dec x Dec x Dec -> Dec

void decimalFMA(sqlite3_context* context, sqlite3_value* v1, sqlite3_value* v2, sqlite3_value* v3) {
  decQuad x;
  decQuad y;
  decQuad z;
  decContext* decCtx = sqlite3_user_data(context);

  if (decode(&x, decCtx, v1, context) && decode(&y, decCtx, v2, context) && decode(&z, decCtx, v3, context)) {
    decQuad result;
    decQuadFMA(&result, &x, &y, &z, decCtx);

    if (checkStatus(context, decCtx, decCtx->traps)) {
      decQuadToSQLite3Blob(context, &result);
    }
  }
}


#pragma mark Dec x ... x Dec -> Dec

#define SQLITE_DECIMAL_OPn(fun, op, defaultValue)                                 \
  void decimal ## fun(sqlite3_context* context, int argc, sqlite3_value** argv) { \
    decContext* decCtx = sqlite3_user_data(context);                              \
    decQuad result;                                                               \
    if (argc == 0) {                                                              \
      defaultValue(&result, decCtx);                                              \
      decQuadToSQLite3Blob(context, &result);                                     \
      return;                                                                     \
    }                                                                             \
    if (decode(&result, decCtx, argv[0], context)) {                              \
      for (int i = 1; i < argc; i++) {                                            \
        decQuad decnum;                                                           \
        if (decode(&decnum, decCtx, argv[i], context)) {                          \
          op(&result, &result, &decnum, decCtx);                                  \
        }                                                                         \
        else {                                                                    \
          return;                                                                 \
        }                                                                         \
      }                                                                           \
      if (checkStatus(context, decCtx, decCtx->traps)) {                          \
        decQuadToSQLite3Blob(context, &result);                                   \
      }                                                                           \
    }                                                                             \
  }

static void decimalAddDefault(decQuad* decnum, decContext* decCtx) {
  (void)decCtx;
  decQuadZero(decnum);
}

static void decimalMaxDefault(decQuad* decnum, decContext* decCtx) {
  decQuadFromString(decnum, "inf", decCtx);
}

static void decimalMinDefault(decQuad* decnum, decContext* decCtx) {
  decQuadFromString(decnum, "-inf", decCtx);
}

static void decimalMultiplyDefault(decQuad* decnum, decContext* decCtx) {
  decQuadFromString(decnum, "1", decCtx);
}

  SQLITE_DECIMAL_OPn(Add,      decQuadAdd,      decimalAddDefault)
  SQLITE_DECIMAL_OPn(Max,      decQuadMax,      decimalMaxDefault)
  SQLITE_DECIMAL_OPn(MaxMag,   decQuadMaxMag,   decimalMaxDefault)
  SQLITE_DECIMAL_OPn(Min,      decQuadMin,      decimalMinDefault)
  SQLITE_DECIMAL_OPn(MinMag,   decQuadMinMag,   decimalAddDefault)
SQLITE_DECIMAL_OPn(Multiply, decQuadMultiply, decimalMultiplyDefault)


#pragma mark Aggregate functions

  /**
   * \brief Holds the cumulative sum computed by decSum().
   */
  typedef struct AggregateData AggregateData;

  struct AggregateData {
    decQuad value;
    decContext* decCtx;
    uint32_t count;
  };

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
      decQuad value;                                                                                 \
      if (decode(&value, data->decCtx, argv[0], context)) {                                          \
        aggr(&(data->value), &(data->value), &value, data->decCtx);                                  \
        data->count++;                                                                               \
      }                                                                                              \
    }                                                                                                \
  }

  SQLITE_DECIMAL_AGGR_STEP(Sum, decQuadAdd)
  SQLITE_DECIMAL_AGGR_STEP(Min, decQuadMin)
  SQLITE_DECIMAL_AGGR_STEP(Max, decQuadMax)
SQLITE_DECIMAL_AGGR_STEP(Avg, decQuadAdd)


#define SQLITE_DECIMAL_AGGR_FINAL(fun, defaultValue, finalize)                                         \
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
      decQuadToSQLite3Blob(context, &(data->value));                                                   \
    }

  static void sumAggrDefault(AggregateData* data) {
    decQuadZero(&(data->value));
  }

static void maxAggrDefault(AggregateData* data) {
  decimalMaxDefault(&(data->value), data->decCtx);
}

static void minAggrDefault(AggregateData* data) {
  decimalMinDefault(&(data->value), data->decCtx);
}

static void avgAggrDefault(AggregateData* data) {
  decQuadFromString(&(data->value), "NaN", data->decCtx);
}

static void avgAggrFinOp(AggregateData* data) {
  decQuad count;
  decQuadFromUInt32(&count, data->count);
  decQuadDivide(&(data->value), &(data->value), &count, data->decCtx);
}

static void aggrNoOp(AggregateData* data) {
  (void)data;
}

SQLITE_DECIMAL_AGGR_FINAL(Sum, sumAggrDefault, aggrNoOp)
SQLITE_DECIMAL_AGGR_FINAL(Max, maxAggrDefault, aggrNoOp)
SQLITE_DECIMAL_AGGR_FINAL(Min, minAggrDefault, aggrNoOp)
SQLITE_DECIMAL_AGGR_FINAL(Avg, avgAggrDefault, avgAggrFinOp)


#pragma mark Not implemented

#define SQLITE_DECIMAL_NOT_IMPL1(fun)                                     \
    void decimal ## fun(sqlite3_context* context, sqlite3_value* value) { \
      (void)context;                                                      \
      (void)value;                                                        \
      sqlite3_result_error(context, "Operation not implemented", -1);     \
      return;                                                             \
    }

SQLITE_DECIMAL_NOT_IMPL1(GetCoefficient)
SQLITE_DECIMAL_NOT_IMPL1(ToInt64)

