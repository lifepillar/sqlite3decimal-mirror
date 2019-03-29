#include <signal.h>
#include <string.h>
#include "decNumber/decQuad.h"
#include "impl_decimal.h"

#if !defined(DECLITEND)
/**
 * \brief Tells decNumber whether the platform is little-endian or big-endian.
 *
 * \see decNumber's documentation for the details.
 */
#define DECLITEND 1
#endif

#pragma mark Helper functions

/**
 * \brief Returns the \a i-th bit of a decQuad number.
 *
 * Bit `0` is the MSB, no matter what the endianness is.
 */
static char bit(size_t i, decQuad const* decnum) {
#if DECLITEND
  return '0' + ((decnum->bytes[DECQUAD_Bytes - 1 - i / 8] >> (7 - i % 8)) & 0x01);
#else
  return '0' + ((decnum->bytes[i / 8] >> (7 - i % 8)) & 0x01);
#endif
}

/**
 * \brief Copies \a count bytes from \a src to \a dest in reverse order.
 */
#if !DECLITEND
static void reverse_bytes(size_t count, uint8_t dest[count], uint8_t const src[count]) {
  for (size_t i = 0; i < count; ++i) {
    dest[i] = src[count - 1 - i];
  }
}
#endif

/**
 * \brief Extracts a decimal from a SQLite3 text field.
 *
 * \return \a result
 *
 * If bad syntax is detected, the result is an error if the Traps virtual table
 * contains `Conversion syntax`; otherwise, the result is a a quiet `NaN` and
 * `Conversion syntax` is inserted into the Status table.
 *
 * \note This function assumes that the provided sqlite3_value is of type
 *       `SQLITE_TEXT`.
 */
static decQuad* decQuadFromSQLite3Text(decQuad* result, sqlite3_value* value, decContext* decCtx) {
  char const* const string_num = (char*)sqlite3_value_text(value);
  decQuadFromString(result, string_num, decCtx);
  return result;
}

/**
 * \brief Extracts a decimal from a SQLite3 blob field.
 *
 * \return \a result
 *
 * Decimals are stored as a sequence of DECQUAD_Bytes bytes in little-endian
 * order. If the architecture is big-endian, their order must be reversed when
 * they are read from the database. This function takes care of the endiannes
 * of the platform.
 *
 * If the blob does not have the correct format, the `DEC_Conversion_syntax`
 * flag is set and the result is a quiet `NaN` or an error depending on whether
 * the error is trapped.
 *
 * \note This function assumes that the provided sqlite3_value is of type
 *       `SQLITE_BLOB`.
 */
static decQuad* decQuadFromSQLite3Blob(decQuad* result, sqlite3_value* value, decContext* decCtx) {
  if (sqlite3_value_bytes(value) != DECQUAD_Bytes) {
    decContextSetStatusQuiet(decCtx, DEC_Conversion_syntax);
    decQuadFromString(result, "NaN", decCtx);
    return result;
  }
#if DECLITEND
  memcpy(result, sqlite3_value_blob(value), DECQUAD_Bytes);
#else
 reverse_bytes(DECQUAD_Bytes, result->bytes, sqlite3_value_blob(value));
#endif
  return result;
}

/**
 * \brief Creates a decimal from a SQLite3 integer.
 *
 * \return \a result
 *
 * \note This function assumes that the provided sqlite3_value is of type
 *       `SQLITE_INTEGER`.
 */
static decQuad* decQuadFromSQLite3Int(decQuad* result, sqlite3_value* value) {
  int32_t n = (int32_t)sqlite3_value_int(value);
  decQuadFromInt32(result, n);
  return result;
}

/**
 * \brief A wrapper around `sqlite3_result_blob()` taking into account the
 *        endianness of the platform.
 */
static void decQuadToSQLite3Blob(sqlite3_context* context, decQuad const* decnum) {
#if DECLITEND
  // Little-endian
  sqlite3_result_blob(context, decnum->bytes, DECQUAD_Bytes, SQLITE_TRANSIENT);
#else
  // Big-endian
  uint8_t buf[DECQUAD_Bytes];
  reverse_bytes(DECQUAD_Bytes, &buf, decnum→bytes);
  sqlite3_result_blob(context, &buf, DECQUAD_Bytes, SQLITE_TRANSIENT);
#endif
}

/**
 * \brief Checks decNumber's context status and interrupts the current SQL
 *        operation with an error if any of the mask bit is set.
 *
 * \return `1` if there is no error; `0` otherwise.
 */
static int checkStatus(sqlite3_context* sqlCtx, decContext* decCtx, uint32_t mask) {
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
 * \brief Initializes a decQuad number from a SQLite3 integer, blob, or string.
 *
 * If an error occurs, an error message is set through sqlite3_result_error().
 *
 * \return `1` upon success; `0` if an error occurs.
 *
 */
static int decode(decQuad* decnum, decContext* decCtx, sqlite3_value* value, sqlite3_context* sqlCtx) {
  switch (sqlite3_value_type(value)) {
    case SQLITE_BLOB:
      decQuadFromSQLite3Blob(decnum, value, decCtx);
      break;
    case SQLITE_TEXT:
      decQuadFromSQLite3Text(decnum, value, decCtx);
      break;
    case SQLITE_INTEGER:
      decQuadFromSQLite3Int(decnum, value);
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
 * This handler should never be called. If it is, it just logs a message to
 * `stderr` and returns control to the point where the signal was raised.
 */
static void signalHandler(int signo) {
  signal(SIGFPE, signalHandler); // Re-enable
  fprintf(stderr, "[Decimal] Unexpectedly raised signal: %d\n", signo);
}

void* decimalInitSystem() {
  if (signal(SIGFPE, signalHandler) == SIG_ERR) {
    // Setting signal handling disabled?
  }
  return decimalContextCreate();
}

void* decimalContextCreate() {
  decContext* context = sqlite3_malloc(sizeof(decContext));
  if (context != 0) {
    decContextDefault(context, DEC_INIT_DECQUAD);
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
  if (new_prec != ((decContext*)decCtx)->digits) {
    *zErrMsg = sqlite3_mprintf("Precision cannot be changed");
    return SQLITE_ERROR;
  }
  return SQLITE_OK;
}

int decimalSetMaxExp(void* decCtx, int new_exp, char** zErrMsg) {
  if (new_exp != ((decContext*)decCtx)->emax) {
    *zErrMsg = sqlite3_mprintf("Exponent cannot be changed");
    return SQLITE_ERROR;
  }
  return SQLITE_OK;
}

int decimalSetMinExp(void* decCtx, int new_exp, char** zErrMsg) {
  if (new_exp != ((decContext*)decCtx)->emin) {
    *zErrMsg = sqlite3_mprintf("Exponent cannot be changed");
    return SQLITE_ERROR;
  }
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
 */
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
    return DEC_ROUND_HALF_EVEN;
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

// Of the following flags, DEC_Clamped, DEC_Rounded and DEC_Subnormal are never
// set by decQuad.
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
    if (decContextTestStatus(decCtx, extended_flags[i].flag)) {
      --k;
    }
    if (k == 0) {
      return extended_flags[i].name;
    }
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
    if (((decContext*)decCtx)->traps & extended_flags[i].flag) {
      --k;
    }
    if (k == 0) {
      return extended_flags[i].name;
    }
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
  strcat(version, decQuadVersion());
  strcat(version, ")");
  sqlite3_result_text(context, version, -1, SQLITE_TRANSIENT);
}

#pragma mark . → Dec

void decimalCreate(sqlite3_context* context, sqlite3_value* value) {
  decQuad decnum;
  decContext* decCtx = sqlite3_user_data(context);
  if (decode(&decnum, decCtx, value, context)) {
    decQuadToSQLite3Blob(context, &decnum);
  }
}

#pragma mark Dec → Dec

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

SQLITE_DECIMAL_OP1(Abs,        decQuadAbs)
SQLITE_DECIMAL_OP1(NextDown,   decQuadNextMinus)
SQLITE_DECIMAL_OP1(NextUp,     decQuadNextPlus)
SQLITE_DECIMAL_OP1(Invert,     decQuadInvert)
SQLITE_DECIMAL_OP1(LogB,       decQuadLogB)
SQLITE_DECIMAL_OP1(Minus,      decQuadMinus)
SQLITE_DECIMAL_OP1(Plus,       decQuadPlus)
SQLITE_DECIMAL_OP1(Reduce,     decQuadReduce)
SQLITE_DECIMAL_OP1(ToIntegral, decQuadToIntegralExact)

#pragma mark Dec × Dec → Dec

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

#pragma mark Dec → Text

// See: IEEE Standard for Floating Point Arithmetic (IEEE754), 2008, p. 22.
void decimalBits(sqlite3_context* context, sqlite3_value* value) {
  decQuad decnum;
  decContext* decCtx = sqlite3_user_data(context);

  if (!decode(&decnum, decCtx, value, context))
      return;

  size_t k = 8 * DECQUAD_Bytes; // Storage in bits
  size_t w = (k / 16 + 9) - 5; // (Size of the combination field) - 5
  size_t t = 15 * k / 16 - 10; // Trailing significand field width in bits
  char bits[k + 3 + (t / 10 - 1) + 1]; // t/10 - 1 spaces between declets

  bits[0] = bit(0, &decnum); // Sign bit
  bits[1] = ' ';
  // Combination field (bits 1-17)
  size_t m = 2;
  for (size_t i = 1; i <= 5; i++) { // bits 1-5
    bits[m] = bit(i, &decnum);
    m++;
  }
  bits[m] = ' ';
  m++;
  for (size_t i = 6; i <= 5 + w; i++) { // bits 6-(w+5)
    bits[m] = bit(i, &decnum);
    m++;
  }
  bits[m] = ' ';
  m++;
  // Significand
  for (size_t i = w + 6; i < k; i += 10) { // Group remaining bits in declets
    for (size_t j = 0; j < 10; j++) {
      bits[m] = bit(i + j, &decnum);
      m++;
    }
    bits[m] = ' ';
    m++;
  }
  bits[m-1] = '\0';
  sqlite3_result_text(context, bits, -1, SQLITE_TRANSIENT);
}

void decimalBytes(sqlite3_context* context, sqlite3_value* value) {
  decQuad decnum;
  decContext* decCtx = sqlite3_user_data(context);

  if (!decode(&decnum, decCtx, value, context))
      return;

  char hexes[3 * DECQUAD_Bytes + 1]; // 3 characters per byte
  for (size_t i = 0; i < DECQUAD_Bytes; i++) {
#if DECLITEND
    sprintf(&hexes[i * 3], "%02x ", decnum.bytes[DECQUAD_Bytes - 1 - i]);
#else
    sprintf(&hexes[i * 3], "%02x ", decnum.bytes[i]);
#endif
  }
  hexes[3 * DECQUAD_Bytes - 1] = '\0';
  sqlite3_result_text(context, hexes, -1, SQLITE_TRANSIENT);
}

void decimalGetCoefficient(sqlite3_context* context, sqlite3_value* value) {
  decQuad decnum;
  decContext* decCtx = sqlite3_user_data(context);
  if (decode(&decnum, decCtx, value, context)) {
    // bcd_coeff is an array of DECQUAD_Pmax elements, one digit in each byte
    // (BCD8 encoding); the first (most significant) digit is ignored if the
    // result will be a NaN; all are ignored if the result is finite.
    // All bytes must be in the range 0-9 (decNumber's mmanual, p. 56).
    uint8_t bcd_coeff[DECQUAD_Pmax];
    // No error is possible from decQuadGetCoefficient(), and no status is set
    decQuadGetCoefficient(&decnum, &bcd_coeff[0]);
    char digits[DECQUAD_Pmax + 1];
    for (size_t i = 0; i < DECQUAD_Pmax; i++)
      sprintf(&digits[i], "%01d", bcd_coeff[i]);
    digits[DECQUAD_Pmax] = '\0';
    sqlite3_result_text(context, digits, -1, SQLITE_TRANSIENT);
  }
}

void decimalToString(sqlite3_context* context, sqlite3_value* value) {
  decQuad decnum;
  decContext* decCtx = sqlite3_user_data(context);
  if (decode(&decnum, decCtx, value, context)) {
    // Converting a decNumber to a string requires a buffer
    // DECQUAD_String + 14 bytes long.
    char s[DECQUAD_String + 14];
    // No error is possible from decQuadToString(), and no status is set
    decQuadToString(&decnum, s);
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
  decQuad decnum;
  decContext* decCtx = sqlite3_user_data(context);
  if (decode(&decnum, decCtx, value, context))
    sqlite3_result_text(context, decQuadClassString(&decnum), -1, SQLITE_STATIC);
}

#pragma mark Dec → Bool

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
SQLITE_DECIMAL_INT1(IsInteger,   decQuadIsInteger)
SQLITE_DECIMAL_INT1(IsLogical,   decQuadIsLogical)
SQLITE_DECIMAL_INT1(IsNaN,       decQuadIsNaN)
SQLITE_DECIMAL_INT1(IsNegative,  decQuadIsNegative)
SQLITE_DECIMAL_INT1(IsNormal,    decQuadIsNormal)
SQLITE_DECIMAL_INT1(IsPositive,  decQuadIsPositive)
SQLITE_DECIMAL_INT1(IsSigned,    decQuadIsSigned)
SQLITE_DECIMAL_INT1(IsSubnormal, decQuadIsSubnormal)
SQLITE_DECIMAL_INT1(IsZero,      decQuadIsZero)

#pragma mark Dec → Int

/**
 * \brief Returns the number of significant digits in a given decimal.
 *
 * If the argument is zero or an infinite, `1` is returned.
 * If the argument is `NaN`, then the number of digits in the payload is
 * returned.
 */
SQLITE_DECIMAL_INT1(Digits,      decQuadDigits)
SQLITE_DECIMAL_INT1(GetExponent, decQuadGetExponent)

void decimalToInt32(sqlite3_context* context, sqlite3_value* value) {
  decQuad decnum;
  decContext* decCtx = sqlite3_user_data(context);
  if (decode(&decnum, decCtx, value, context)) {
    // If rounding removes non-zero digits then the DEC_Inexact flag is set
    int result = decQuadToInt32Exact(&decnum, decCtx, decCtx->round);
    if (checkStatus(context, decCtx, decCtx->traps))
      sqlite3_result_int(context, result);
  }
}

#pragma mark Dec × Dec → Int

#define SQLITE_DECIMAL_CMP(fun, cmp)                                                    \
  void decimal ## fun(sqlite3_context* context, sqlite3_value* v1, sqlite3_value* v2) { \
    decQuad x;                                                                          \
    decQuad y;                                                                          \
    decContext* decCtx = sqlite3_user_data(context);                                    \
    if (decode(&x, decCtx, v1, context) && decode(&y, decCtx, v2, context)) {           \
      decQuad result;                                                                   \
      decQuadCompare(&result, &x, &y, decCtx);                                          \
      sqlite3_result_int(context, cmp(&result));                                        \
    }                                                                                   \
  }                                                                                     \

#define decQuadIsNegativeOrZero(d) (decQuadIsNegative(d) || decQuadIsZero(d))

SQLITE_DECIMAL_CMP(Equal,              decQuadIsZero)
SQLITE_DECIMAL_CMP(GreaterThan,        decQuadIsPositive)
SQLITE_DECIMAL_CMP(GreaterThanOrEqual, !decQuadIsNegativeOrZero)
SQLITE_DECIMAL_CMP(LessThan,           decQuadIsNegative)
SQLITE_DECIMAL_CMP(LessThanOrEqual,    decQuadIsNegativeOrZero)
SQLITE_DECIMAL_CMP(NotEqual,           !decQuadIsZero)

void decimalCompare(sqlite3_context* context, sqlite3_value* v1, sqlite3_value* v2) {
  decQuad x;
  decQuad y;
  decContext* decCtx = sqlite3_user_data(context);
  if (decode(&x, decCtx, v1, context) && decode(&y, decCtx, v2, context)) {
    decQuad result;
    // If an argument is a NaN, the DEC_Invalid_operation flag is set.
    decQuadCompareSignal(&result, &x, &y, decCtx);
    if (checkStatus(context, decCtx, decCtx->traps))
      sqlite3_result_int(context, decQuadToInt32Exact(&result, decCtx, DEC_ROUND_UP)); // Rounding mode does not matter
  }
}

void decimalSameQuantum(sqlite3_context* context, sqlite3_value* v1, sqlite3_value* v2) {
  decQuad x;
  decQuad y;
  decContext* decCtx = sqlite3_user_data(context);
  if (decode(&x, decCtx, v1, context) && decode(&y, decCtx, v2, context)) {
    int result = decQuadSameQuantum(&x, &y);
    sqlite3_result_int(context, result);
  }
}

#pragma mark Dec × Dec × Dec → Dec

/**
 * \brief Calculates the fused multiply-add `v1 × v2 + v3`.
 */
void decimalFMA(sqlite3_context* context, sqlite3_value* v1, sqlite3_value* v2, sqlite3_value* v3) {
  decQuad x;
  decQuad y;
  decQuad z;
  decContext* decCtx = sqlite3_user_data(context);
  if (decode(&x, decCtx, v1, context) && decode(&y, decCtx, v2, context) && decode(&z, decCtx, v3, context)) {
    decQuad result;
    decQuadFMA(&result, &x, &y, &z, decCtx);
    if (checkStatus(context, decCtx, decCtx->traps))
      decQuadToSQLite3Blob(context, &result);
  }
}

#pragma mark Dec × ⋯ × Dec → Dec

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
  (void)decCtx;
  uint8_t mantissa[DECQUAD_Pmax];
  for (size_t i = 0; i < DECQUAD_Pmax; i++)
    mantissa[i] = 0x09;
  // See IEEE Std 754-2008 §3.3 Sets of floating-point data
  decQuadFromBCD(decnum, DECQUAD_Emax - DECQUAD_Pmax + 1, mantissa, 0);
}

static void decimalMinDefault(decQuad* decnum, decContext* decCtx) {
  (void)decCtx;
  uint8_t mantissa[DECQUAD_Pmax];
  for (size_t i = 0; i < DECQUAD_Pmax; i++)
    mantissa[i] = 0x09;
  decQuadFromBCD(decnum, DECQUAD_Emax - DECQUAD_Pmax + 1, mantissa, DECFLOAT_Sign);
}

static void decimalMultiplyDefault(decQuad* decnum, decContext* decCtx) {
  decQuadFromString(decnum, "1", decCtx);
}

SQLITE_DECIMAL_OPn(Add,      decQuadAdd,      decimalAddDefault)
SQLITE_DECIMAL_OPn(Max,      decQuadMax,      decimalMaxDefault)
SQLITE_DECIMAL_OPn(MaxMag,   decQuadMaxMag,   decimalMaxDefault)
SQLITE_DECIMAL_OPn(Min,      decQuadMin,      decimalMinDefault)
SQLITE_DECIMAL_OPn(MinMag,   decQuadMinMag,   decimalMinDefault)
SQLITE_DECIMAL_OPn(Multiply, decQuadMultiply, decimalMultiplyDefault)

/**
 * \brief Holds the cumulative sum computed by decSum().
 */
typedef struct AggregateData AggregateData;

struct AggregateData {
  decQuad value;
  decContext* decCtx;
  uint32_t count;
};

#pragma mark Aggregate functions

#define SQLITE_DECIMAL_AGGR_STEP(fun, aggr)                                                          \
  void decimal ## fun ## Step(sqlite3_context* context, int argc, sqlite3_value** argv) {            \
    (void)argc;                                                                                      \
    AggregateData* data = (AggregateData*)sqlite3_aggregate_context(context, sizeof(AggregateData)); \
                                                                                                     \
    if (data == 0) {                                                                                 \
      sqlite3_result_error_nomem(context);                                                           \
      return;                                                                                        \
    }                                                                                                \
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

#define SQLITE_DECIMAL_AGGR_FINAL(fun, defaultValue, finalize)                                       \
  void decimal ## fun ## Final(sqlite3_context* context) {                                           \
    AggregateData* data = (AggregateData*)sqlite3_aggregate_context(context, sizeof(AggregateData)); \
                                                                                                     \
    if (data == 0) {                                                                                 \
      sqlite3_result_error_nomem(context);                                                           \
      return;                                                                                        \
    }                                                                                                \
                                                                                                     \
    if (data->count == 0) {                                                                          \
      data->decCtx = sqlite3_user_data(context);                                                     \
      defaultValue(data);                                                                            \
    }                                                                                                \
                                                                                                     \
    finalize(data);                                                                                  \
                                                                                                     \
    if (checkStatus(context, data->decCtx, data->decCtx->traps))                                     \
      decQuadToSQLite3Blob(context, &(data->value));                                                 \
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

SQLITE_DECIMAL_NOT_IMPL1(Exp)
SQLITE_DECIMAL_NOT_IMPL1(Ln)
SQLITE_DECIMAL_NOT_IMPL1(Log10)
SQLITE_DECIMAL_NOT_IMPL1(ToInt64)
SQLITE_DECIMAL_NOT_IMPL1(Sqrt)
SQLITE_DECIMAL_NOT_IMPL1(Trim)
SQLITE_DECIMAL_NOT_IMPL2(Power)

