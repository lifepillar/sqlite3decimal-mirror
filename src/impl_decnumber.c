#include <signal.h>
#include <string.h>

//#ifndef DECNUMDIGITS
/**
 ** @brief TODO
 **/
#define DECNUMDIGITS 69
//#endif

#include "decNumber/decNumber.h"
#include "decNumber/decPacked.h"
#include "impl_decimal.h"

#pragma mark Helper functions

#if !defined(DECLITEND)
/**
 ** @brief Tells decNumber whether the platform is little-endian or big-endian.
 **
 ** @see decNumber's documentation for the details.
 **/
#define DECLITEND 1
#endif

#pragma mark Helper functions

/**
 ** @brief Copies `count` bytes from `src` to `dest` in reverse order.
 **/
#if !DECLITEND
static void
reverse_bytes(size_t count, uint8_t dest[count], uint8_t const src[count]) {
  for (size_t i = 0; i < count; i++) {
    dest[i] = src[count - 1 - i];
  }
}
#endif

/**
 ** @brief Extracts a number from a SQLite3 text field.
 **
 ** @return `result`
 **
 ** If bad syntax is detected, the result is an error if the `decTraps` virtual
 ** table contains `Conversion syntax`; otherwise, the result is a a quiet
 ** `NaN` and `Conversion syntax` is inserted into the `decStatus` table.
 **
 ** @note This function assumes that the provided `sqlite3_value` is of type `SQLITE_TEXT`.
 **/
static decNumber*
decNumberFromSQLite3Text(decNumber* result, sqlite3_value* value, decContext* decCtx) {
  char const* const string_num = (char*)sqlite3_value_text(value);
  decNumberFromString(result, string_num, decCtx);
  return result;
}

/**
 ** @brief Extracts a number from a SQLite3 blob field.
 **
 ** @return `result`
 **
 ** Decimals are stored as a variable-length sequence of in little-endian
 ** order. If the architecture is big-endian, their order must be reversed when
 ** they are read from the database. This function takes care of the endianness
 ** of the platform.
 **
 ** If the blob does not have the correct format, the `DEC_Conversion_syntax`
 ** flag is set and the result is a quiet `NaN` or an error depending on whether
 ** the error is trapped.
 **
 ** @note This function assumes that the provided `sqlite3_value` is of type `SQLITE_BLOB`.
 **/
static decNumber*
decNumberFromSQLite3Blob(decNumber* result, sqlite3_value* value, decContext* decCtx) {
  (void)decCtx;
  int32_t length = sqlite3_value_bytes(value);
  uint8_t const* bytes = sqlite3_value_blob(value);
  int32_t* scale = (int32_t*)bytes;
#if DECLITEND
  decPackedToNumber(bytes + sizeof(int32_t), length - sizeof(int32_t), scale, result);
#else
  // FIXME: reverse scale and unpack
#endif
  return result;
}

/**
 ** @brief Creates a decimal from a SQLite3 integer.
 **
 ** @return `result`
 **
 ** @note This function assumes that the provided `sqlite3_value` is of type `SQLITE_INTEGER`.
 **/
static decNumber*
decNumberFromSQLite3Int(decNumber* result, sqlite3_value* value) {
  int32_t n = (int32_t)sqlite3_value_int(value);
  decNumberFromInt32(result, n);
  return result;
}

/**
 ** @brief A wrapper around `sqlite3_result_blob()` to store a number.
 **/
static void
decNumberToSQLite3Blob(sqlite3_context* context, decNumber const* decnum) {
  int32_t scale;
  int32_t length = sizeof(scale) + (decnum->digits + 2) / 2;
  uint8_t bytes[length]; // TODO: allocate dynamically?
  // FIXME: little-endian only
  decPackedFromNumber(&bytes[0] + sizeof(scale), length - sizeof(scale), (int32_t*)&bytes[0], decnum);
  sqlite3_result_blob(context, bytes, length, SQLITE_TRANSIENT);
}

/**
 ** @brief Checks decNumber's context status and interrupts the current SQL
 **        operation with an error if any of the mask bit is set.
 **
 ** @return `1` if there is no error; `0` otherwise.
 **/
static int
checkStatus(sqlite3_context* sqlCtx, decContext* decCtx, uint32_t mask) {
  if (decContextTestStatus(decCtx, mask)) {
    decContext errCtx;
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
 ** @brief Initializes a `decNumber` number from a SQLite3 integer, blob, or string.
 **
 ** @return `1` upon success; `0` if an error occurs.
 **
 ** If an error occurs, an error message is set through `sqlite3_result_error()`.
 **/
static int
decode(decNumber* decnum, decContext* decCtx, sqlite3_value* value, sqlite3_context* sqlCtx) {
  switch (sqlite3_value_type(value)) {
    case SQLITE_BLOB:
      decNumberFromSQLite3Blob(decnum, value, decCtx);
      break;
    case SQLITE_TEXT:
      decNumberFromSQLite3Text(decnum, value, decCtx);
      break;
    case SQLITE_INTEGER:
      decNumberFromSQLite3Int(decnum, value);
      break;
    default:
      sqlite3_result_error(sqlCtx, "Cannot create decimal from the given type", -1);
      return 0;
  }
  return checkStatus(sqlCtx, decCtx, decCtx->traps);
}

#pragma mark Context functions

/**
 ** @brief Handles `SIGFPE` signals.
 **
 ** This handler should never be called. If it is, it just logs a message to
 ** `stderr` and returns control to the point the signal was raised.
 **/
static void
signalHandler(int signo) {
  signal(SIGFPE, signalHandler); // Re-enable
  fprintf(stderr, "[Decimal] Unexpectedly raised signal: %d\n", signo);
}

void*
decimalInitSystem() {
  if (signal(SIGFPE, signalHandler) == SIG_ERR) {
    // Setting signal handling disabled?
  }
  return decimalContextCreate();
}

void*
decimalContextCreate() {
  decContext* context = sqlite3_malloc(sizeof(decContext));
  if (context != 0) {
    decContextDefault(context, DEC_INIT_BASE);
    context->digits = DECNUMDIGITS;
    context->traps |= (DEC_Conversion_syntax   |
                       DEC_Division_by_zero    |
                       DEC_Division_undefined  |
                       DEC_Division_impossible |
                       DEC_Insufficient_storage|
                       DEC_Invalid_context     |
                       DEC_Invalid_operation   );
  }
  return context;
}

void
decimalContextCopy(void* target, void* source) {
  *(decContext*)target = *(decContext*)source;
}

void
decimalContextDestroy(void* context) {
  if (context) {
    sqlite3_free((decContext*)context);
  }
}

#pragma mark Helper functions for context virtual table

int
decimalPrecision(void* decCtx) {
  return ((decContext*)decCtx)->digits;
}

int
decimalMaxExp(void* decCtx) {
  return ((decContext*)decCtx)->emax;
}

int
decimalMinExp(void* decCtx) {
  return ((decContext*)decCtx)->emin;
}

int
decimalSetPrecision(void* decCtx, int new_prec, char** zErrMsg) {
  if (new_prec != ((decContext*)decCtx)->digits) {
    *zErrMsg = sqlite3_mprintf("Precision cannot be changed");
    return SQLITE_ERROR;
  }
  return SQLITE_OK;
}

int
decimalSetMaxExp(void* decCtx, int new_exp, char** zErrMsg) {
  if (new_exp != ((decContext*)decCtx)->emax) {
    *zErrMsg = sqlite3_mprintf("Exponent cannot be changed");
    return SQLITE_ERROR;
  }
  return SQLITE_OK;
}

int
decimalSetMinExp(void* decCtx, int new_exp, char** zErrMsg) {
  if (new_exp != ((decContext*)decCtx)->emin) {
    *zErrMsg = sqlite3_mprintf("Exponent cannot be changed");
    return SQLITE_ERROR;
  }
  return SQLITE_OK;
}

/**
 ** @brief Supported rounding modes.
 **
 ** The supported rounding modes are:
 **
 ** - `ROUND_CEILING`: round towards +∞.
 ** - `ROUND_DOWN`: round towards 0.
 ** - `ROUND_FLOOR`: round towards -∞.
 ** - `ROUND_HALF_DOWN`: round to nearest; if equidistant, round down.
 ** - `ROUND_HALF_EVEN`: round to nearest; if equidistant, round so that the
 **                       final digit is even.
 ** - `ROUND_HALF_UP`: round to nearest; if equidistant, round up.
 ** - `ROUND_UP`: round away from 0.
 ** - `ROUND_05UP`: the same as `ROUND_UP`, except that rounding up only
 **                  occurs if the digit to be rounded is 0 or 5 and after.
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

static char const*
roundingModeToString(enum rounding round) {
  return rounding_mode[round].name;
}

static enum rounding
roundingModeToEnum(char const* round) {
  if (strncmp(round, "DEFAULT", strlen("DEFAULT")) == 0)
    return DEC_ROUND_HALF_EVEN;
  for (size_t i = 0; i < DEC_ROUND_MAX; i++) {
    if (strncmp(round, rounding_mode[i].name, strlen(rounding_mode[i].name)) == 0)
      return rounding_mode[i].value;
  }
  return DEC_ROUND_MAX;
}

char const*
decimalRoundingMode(void* decCtx) {
  enum rounding mode = decContextGetRounding(decCtx);
  return roundingModeToString(mode);
}

int
decimalSetRoundingMode(void* decCtx, char const* new_mode, char** zErrMsg) {
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

// Of the following flags, DEC_Clamped, DEC_Rounded and DEC_Subnormal are never
// set by decNumber.
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

static uint32_t
flag_string_to_int(char const* flag) {
  for (size_t i = 0; i < FLAGS_MAX; i++) {
    if (strncmp(flag, extended_flags[i].name, strlen(extended_flags[i].name)) == 0)
      return extended_flags[i].flag;
  }
  return FLAGS_MAX;
}

int
decimalClearFlag(void* decCtx, char const* flag, char** zErrMsg) {
  uint32_t status = flag_string_to_int(flag);
  if (status == FLAGS_MAX) {
    *zErrMsg = sqlite3_mprintf("Invalid flag: %s", flag);
    return SQLITE_ERROR;
  }
  decContextClearStatus(decCtx, status);
  return SQLITE_OK;
}

int
decimalSetFlag(void* decCtx, char const* flag, char** zErrMsg) {
  uint32_t status = flag_string_to_int(flag);
  if (status == FLAGS_MAX) {
    *zErrMsg = sqlite3_mprintf("Invalid flag: %s", flag);
    return SQLITE_ERROR;
  }
  decContextSetStatusQuiet(decCtx, status);
  return SQLITE_OK;
}

char const*
decimalGetFlag(void* decCtx, size_t n) {
  size_t k = n + 1;
  for (size_t i = 0; i < FLAGS_MAX; i++) {
    if (decContextTestStatus(decCtx, extended_flags[i].flag)) {
      k--;
    }
    if (k == 0) {
      return extended_flags[i].name;
    }
  }
  return 0;
}

int
decimalClearTrap(void* decCtx, char const* flag, char** zErrMsg) {
  uint32_t status = flag_string_to_int(flag);
  if (status == FLAGS_MAX) {
    *zErrMsg = sqlite3_mprintf("Invalid flag: %s", flag);
    return SQLITE_ERROR;
  }
  ((decContext*)decCtx)->traps &= ~status;
  return SQLITE_OK;
}

int
decimalSetTrap(void* decCtx, char const* flag, char** zErrMsg) {
  uint32_t status = flag_string_to_int(flag);
  if (status == FLAGS_MAX) {
    *zErrMsg = sqlite3_mprintf("Invalid flag: %s", flag);
    return SQLITE_ERROR;
  }
  ((decContext*)decCtx)->traps |= status;
  return SQLITE_OK;
}

char const*
decimalGetTrap(void* decCtx, size_t n) {
  size_t k = n + 1;
  for (size_t i = 0; i < FLAGS_MAX; i++) {
    if (((decContext*)decCtx)->traps & extended_flags[i].flag) {
      k--;
    }
    if (k == 0) {
      return extended_flags[i].name;
    }
  }
  return 0;
}

#pragma mark Nullary functions

void
decimalClearStatus(sqlite3_context* context) {
  decContext* sharedCtx = sqlite3_user_data(context);
  decContextZeroStatus(sharedCtx);
}

void
decimalStatus(sqlite3_context* context) {
  decContext* sharedCtx = sqlite3_user_data(context);
  uint32_t status = decContextGetStatus(sharedCtx);
  char* res = sqlite3_mprintf("%08x", status);
  sqlite3_result_text(context, res, -1, SQLITE_TRANSIENT);
  sqlite3_free(res);
}

void
decimalVersion(sqlite3_context* context) {
  // decNumberVersion() returns at most 16 characters (decNumber manual, p. 47)
  char version[sizeof(SQLITE_DECIMAL_VERSION) + 16 + 4] = SQLITE_DECIMAL_VERSION;
  strcat(version, " (");
  strcat(version, decNumberVersion());
  strcat(version, ")");
  sqlite3_result_text(context, version, -1, SQLITE_TRANSIENT);
}

#pragma mark . → Dec

void
decimalCreate(sqlite3_context* context, sqlite3_value* value) {
  decNumber decnum;
  decContext* decCtx = sqlite3_user_data(context);
  if (decode(&decnum, decCtx, value, context)) {
    decNumberToSQLite3Blob(context, &decnum);
  }
}

#pragma mark Dec → Dec

#define SQLITE_DECIMAL_OP1(fun, op)                                          \
void decimal ## fun(sqlite3_context* context, sqlite3_value* value) { \
decNumber decnum;                                                     \
decContext* decCtx = sqlite3_user_data(context);                    \
if (decode(&decnum, decCtx, value, context)) {                      \
decNumber result;                                                   \
op(&result, &decnum, decCtx);                                     \
if (checkStatus(context, decCtx, decCtx->traps)) {                \
decNumberToSQLite3Blob(context, &result);                         \
}                                                                 \
}                                                                   \
}

SQLITE_DECIMAL_OP1(Abs,        decNumberAbs)
SQLITE_DECIMAL_OP1(NextDown,   decNumberNextMinus)
SQLITE_DECIMAL_OP1(NextUp,     decNumberNextPlus)
SQLITE_DECIMAL_OP1(Invert,     decNumberInvert)
SQLITE_DECIMAL_OP1(LogB,       decNumberLogB)
SQLITE_DECIMAL_OP1(Minus,      decNumberMinus)
SQLITE_DECIMAL_OP1(Plus,       decNumberPlus)
SQLITE_DECIMAL_OP1(Reduce,     decNumberReduce)
SQLITE_DECIMAL_OP1(ToIntegral, decNumberToIntegralExact)

#pragma mark Dec × Dec → Dec

#define SQLITE_DECIMAL_OP2(fun, op)                                                                  \
void decimal ## fun(sqlite3_context* context, sqlite3_value* value1, sqlite3_value* value2) { \
decNumber d1;                                                                                 \
decNumber d2;                                                                                 \
decContext* decCtx = sqlite3_user_data(context);                                            \
if (decode(&d1, decCtx, value1, context) && decode(&d2, decCtx, value2, context)) {         \
decNumber result;                                                                           \
op(&result, &d1, &d2, decCtx);                                                            \
if (checkStatus(context, decCtx, decCtx->traps)) {                                        \
decNumberToSQLite3Blob(context, &result);                                                 \
}                                                                                         \
}                                                                                           \
}

SQLITE_DECIMAL_OP2(And,           decNumberAnd)
SQLITE_DECIMAL_OP2(Divide,        decNumberDivide)
SQLITE_DECIMAL_OP2(DivideInteger, decNumberDivideInteger)
SQLITE_DECIMAL_OP2(Or,            decNumberOr)
SQLITE_DECIMAL_OP2(Quantize,      decNumberQuantize)
SQLITE_DECIMAL_OP2(Remainder,     decNumberRemainder)
SQLITE_DECIMAL_OP2(Rotate,        decNumberRotate)
SQLITE_DECIMAL_OP2(ScaleB,        decNumberScaleB)
SQLITE_DECIMAL_OP2(Shift,         decNumberShift)
SQLITE_DECIMAL_OP2(Subtract,      decNumberSubtract)
SQLITE_DECIMAL_OP2(Xor,           decNumberXor)

#pragma mark Dec → Text


void
decimalBytes(sqlite3_context* context, sqlite3_value* value) {
  decNumber decnum;
  decContext* decCtx = sqlite3_user_data(context);

  if (!decode(&decnum, decCtx, value, context))
    return;

  char hexes[3 * decnum.digits + 1]; // 3 characters per byte
  for (int32_t i = 0; i < decnum.digits; i++) {
#if DECLITEND
    sprintf(&hexes[i * 3], "%02x ", decnum.lsu[decnum.digits - 1 - i]);
#else
    sprintf(&hexes[i * 3], "%02x ", decnum.lsu[i]);
#endif
  }
  hexes[3 * DECNUMDIGITS - 1] = '\0';
  sqlite3_result_text(context, hexes, -1, SQLITE_TRANSIENT);
}

void
decimalGetCoefficient(sqlite3_context* context, sqlite3_value* value) {
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
    for (size_t i = 0; i < DECNUMDIGITS; i++)
      sprintf(&digits[i], "%01d", bcd_coeff[i]);
    digits[DECNUMDIGITS] = '\0';
    sqlite3_result_text(context, digits, -1, SQLITE_TRANSIENT);
  }
}

void
decimalToString(sqlite3_context* context, sqlite3_value* value) {
  decNumber decnum;
  decContext* decCtx = sqlite3_user_data(context);
  if (decode(&decnum, decCtx, value, context)) {
    // Converting a decNumber to a string requires a buffer
    // DECQUAD_String + 14 bytes long.
    char s[DECNUMDIGITS + 14];
    // No error is possible from decNumberToString(), and no status is set
    decNumberToString(&decnum, s);
    sqlite3_result_text(context, s, -1, SQLITE_TRANSIENT);
  }
}

/**
 ** @brief Returns the class of a decimal as text.
 **
 ** The possible values are:
 **
 ** - `+Normal`: positive normal.
 ** - `-Normal`: negative normal.
 ** - `+Zero`: positive zero.
 ** - `-Zero`: negative zero.
 ** - `+Infinity`: +∞.
 ** - `-Infinity`: -∞.
 ** - `+Subnormal`: positive subnormal.
 ** - `-Subnormal`: negative subnormal.
 ** - `NaN`: NaN.
 **/
void
decimalClass(sqlite3_context* context, sqlite3_value* value) {
  decNumber decnum;
  decContext* decCtx = sqlite3_user_data(context);
  if (decode(&decnum, decCtx, value, context))
    sqlite3_result_text(context, decNumberClassToString(decNumberClass(&decnum, decCtx)), -1, SQLITE_STATIC);
}

#pragma mark Dec → Bool

#define SQLITE_DECIMAL_INT1(fun, op)                                         \
void decimal ## fun(sqlite3_context* context, sqlite3_value* value) { \
decNumber decnum;                                                     \
decContext* decCtx = sqlite3_user_data(context);                    \
if (decode(&decnum, decCtx, value, context)) {                      \
int result = op(&decnum);                                         \
sqlite3_result_int(context, result);                              \
}                                                                   \
}

SQLITE_DECIMAL_INT1(IsCanonical, decNumberIsCanonical)
SQLITE_DECIMAL_INT1(IsFinite,    decNumberIsFinite)
SQLITE_DECIMAL_INT1(IsInfinite,  decNumberIsInfinite)
//SQLITE_DECIMAL_INT1(IsInteger,   decNumberIsInteger)
//SQLITE_DECIMAL_INT1(IsLogical,   decNumberIsLogical)
SQLITE_DECIMAL_INT1(IsNaN,       decNumberIsNaN)
SQLITE_DECIMAL_INT1(IsNegative,  decNumberIsNegative)
//SQLITE_DECIMAL_INT2(IsNormal,          decNumberIsNormal)
//SQLITE_DECIMAL_INT1(IsPositive,  decNumberIsPositive)
//SQLITE_DECIMAL_INT1(IsSigned,    decNumberIsSigned)
//SQLITE_DECIMAL_INT1(IsSubnormal, decNumberIsSubnormal)
SQLITE_DECIMAL_INT1(IsZero,      decNumberIsZero)

#pragma mark Dec → Int

/**
 ** @brief Returns the number of significant digits in a given decimal.
 **
 ** If the argument is zero or an infinite, `1` is returned.
 ** If the argument is `NaN`, then the number of digits in the payload is
 ** returned.
 **/
//SQLITE_DECIMAL_INT1(Digits,      decNumberDigits)
//SQLITE_DECIMAL_INT1(GetExponent, decNumberGetExponent)

void
decimalToInt32(sqlite3_context* context, sqlite3_value* value) {
  decNumber decnum;
  decContext* decCtx = sqlite3_user_data(context);
  if (decode(&decnum, decCtx, value, context)) {
    // If rounding removes non-zero digits then the DEC_Inexact flag is set
    //int result = decNumberToInt32Exact(&decnum, decCtx, decCtx->round);
    int result = 0; // FIXME
    if (checkStatus(context, decCtx, decCtx->traps))
      sqlite3_result_int(context, result);
  }
}

#pragma mark Dec × Dec → Int

#define SQLITE_DECIMAL_CMP(fun, cmp)                                                         \
void decimal ## fun(sqlite3_context* context, sqlite3_value* v1, sqlite3_value* v2) { \
decNumber x;                                                                          \
decNumber y;                                                                          \
decContext* decCtx = sqlite3_user_data(context);                                    \
if (decode(&x, decCtx, v1, context) && decode(&y, decCtx, v2, context)) {           \
decNumber result;                                                                   \
decNumberCompare(&result, &x, &y, decCtx);                                          \
sqlite3_result_int(context, cmp(&result));                                        \
}                                                                                   \
}                                                                                     \

#define decNumberIsNegativeOrZero(d) (decNumberIsNegative(d) || decNumberIsZero(d))

SQLITE_DECIMAL_CMP(Equal,              decNumberIsZero)
//SQLITE_DECIMAL_CMP(GreaterThan,        decNumberIsPositive)
SQLITE_DECIMAL_CMP(GreaterThanOrEqual, !decNumberIsNegativeOrZero)
SQLITE_DECIMAL_CMP(LessThan,           decNumberIsNegative)
SQLITE_DECIMAL_CMP(LessThanOrEqual,    decNumberIsNegativeOrZero)
SQLITE_DECIMAL_CMP(NotEqual,           !decNumberIsZero)

void
decimalCompare(sqlite3_context* context, sqlite3_value* v1, sqlite3_value* v2) {
  decNumber x;
  decNumber y;
  decContext* decCtx = sqlite3_user_data(context);
  if (decode(&x, decCtx, v1, context) && decode(&y, decCtx, v2, context)) {
    decNumber result;
    // If an argument is a NaN, the DEC_Invalid_operation flag is set.
    decNumberCompareSignal(&result, &x, &y, decCtx);
    if (checkStatus(context, decCtx, decCtx->traps))
      // FIXME: incorrect
     // sqlite3_result_int(context, decNumberToInt32Exact(&result, decCtx, DEC_ROUND_UP)); // Rounding mode does not matter
      sqlite3_result_int(context, 0);
  }
}

void
decimalSameQuantum(sqlite3_context* context, sqlite3_value* v1, sqlite3_value* v2) {
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
 ** @brief Calculates the fused multiply-add `v1 × v2 + v3`.
 **/
void
decimalFMA(sqlite3_context* context, sqlite3_value* v1, sqlite3_value* v2, sqlite3_value* v3) {
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

#define SQLITE_DECIMAL_OPn(fun, op, defaultValue)                                    \
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

static void
decimalAddDefault(decNumber* decnum, decContext* decCtx) {
  (void)decCtx;
  decNumberZero(decnum);
}

static void
decimalMaxDefault(decNumber* decnum, decContext* decCtx) {
  (void)decCtx;
  uint8_t mantissa[DECNUMDIGITS];
  for (size_t i = 0; i < DECNUMDIGITS; i++)
    mantissa[i] = 0x99;
  decPackedToNumber(&mantissa[0], DECNUMDIGITS, &(decCtx->emax), decnum);
}

static void
decimalMinDefault(decNumber* decnum, decContext* decCtx) {
  (void)decCtx;
  uint8_t mantissa[DECNUMDIGITS];
  for (size_t i = 0; i < DECNUMDIGITS; i++)
    mantissa[i] = 0x99;
  decPackedToNumber(&mantissa[0], DECNUMDIGITS, &(decCtx->emin), decnum);
}

static void
decimalMultiplyDefault(decNumber* decnum, decContext* decCtx) {
  decNumberFromString(decnum, "1", decCtx);
}

SQLITE_DECIMAL_OPn(Add,      decNumberAdd,      decimalAddDefault)
SQLITE_DECIMAL_OPn(Max,      decNumberMax,      decimalMaxDefault)
SQLITE_DECIMAL_OPn(MaxMag,   decNumberMaxMag,   decimalMaxDefault)
SQLITE_DECIMAL_OPn(Min,      decNumberMin,      decimalMinDefault)
SQLITE_DECIMAL_OPn(MinMag,   decNumberMinMag,   decimalMinDefault)
SQLITE_DECIMAL_OPn(Multiply, decNumberMultiply, decimalMultiplyDefault)

/**
 ** @brief Holds the cumulative sum computed by `decSum()`.
 **/
typedef struct AggregateData AggregateData;

struct AggregateData {
  decNumber value;
  decContext* decCtx;
  uint32_t count;
};

#pragma mark Aggregate functions

#define SQLITE_DECIMAL_AGGR_STEP(fun, aggr)                                                             \
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
decNumber value;                                                                                 \
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

#define SQLITE_DECIMAL_AGGR_FINAL(fun, defaultValue, finalize)                                          \
void decimal ## fun ## Final(sqlite3_context* context) {                                           \
AggregateData* data = (AggregateData*)sqlite3_aggregate_context(context, sizeof(AggregateData)); \
\
if (data == 0)                                                                                   \
return;                                                                                        \
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

static void
sumAggrDefault(AggregateData* data) {
  decNumberZero(&(data->value));
}

static void
maxAggrDefault(AggregateData* data) {
  decimalMaxDefault(&(data->value), data->decCtx);
}

static void
minAggrDefault(AggregateData* data) {
  decimalMinDefault(&(data->value), data->decCtx);
}

static void
avgAggrDefault(AggregateData* data) {
  decNumberFromString(&(data->value), "NaN", data->decCtx);
}

static void
avgAggrFinOp(AggregateData* data) {
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

#define SQLITE_DECIMAL_NOT_IMPL1(fun)                                      \
void decimal ## fun(sqlite3_context* context, sqlite3_value* value) { \
(void)context;                                                      \
(void)value;                                                        \
sqlite3_result_error(context, "Operation not implemented", -1);     \
return;                                                             \
}

#define SQLITE_DECIMAL_NOT_IMPL2(fun)                                                      \
void decimal ## fun(sqlite3_context* context, sqlite3_value* v1, sqlite3_value* v2) { \
(void)context;                                                                      \
(void)v1;                                                                           \
(void)v2;                                                                           \
sqlite3_result_error(context, "Operation not implemented", -1);                     \
return;                                                                             \
}

SQLITE_DECIMAL_NOT_IMPL1(Bits)
SQLITE_DECIMAL_NOT_IMPL1(Digits)
SQLITE_DECIMAL_NOT_IMPL1(Exp)
SQLITE_DECIMAL_NOT_IMPL1(GetExponent)
SQLITE_DECIMAL_NOT_IMPL1(IsInteger)
SQLITE_DECIMAL_NOT_IMPL1(IsLogical)
SQLITE_DECIMAL_NOT_IMPL1(IsNormal)
SQLITE_DECIMAL_NOT_IMPL1(IsPositive)
SQLITE_DECIMAL_NOT_IMPL1(IsSigned)
SQLITE_DECIMAL_NOT_IMPL1(IsSubnormal)
SQLITE_DECIMAL_NOT_IMPL1(Ln)
SQLITE_DECIMAL_NOT_IMPL1(Log10)
SQLITE_DECIMAL_NOT_IMPL1(ToInt64)
SQLITE_DECIMAL_NOT_IMPL1(Sqrt)
SQLITE_DECIMAL_NOT_IMPL1(Trim)
SQLITE_DECIMAL_NOT_IMPL2(GreaterThan)
SQLITE_DECIMAL_NOT_IMPL2(Power)


