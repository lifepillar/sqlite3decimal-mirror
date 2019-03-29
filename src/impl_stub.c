#include <signal.h>
#include <string.h>
#include "impl_decimal.h"

// Stub implementation

typedef struct StubContext StubContext;
struct StubContext { };

void* decimalInitSystem()                                                        { return decimalContextCreate(); }
void* decimalContextCreate()                                                     { return malloc(sizeof(StubContext)); }
void  decimalContextCopy(void* target, void* source)                             { }
void  decimalContextDestroy(void* context)                                       { }
int   decimalPrecision(void* decCtx)                                             { return 1; }
int   decimalMaxExp(void* decCtx)                                                { return 0; }
int   decimalMinExp(void* decCtx)                                                { return 0; }
char  const* decimalRoundingMode(void* decCtx)                                   { return "Not implemented"; }
int   decimalSetPrecision(void* decCtx, int new_prec, char** zErrMsg)            { return SQLITE_ERROR; }
int   decimalSetMaxExp(void* decCtx, int new_exp, char** zErrMsg)                { return SQLITE_ERROR; }
int   decimalSetMinExp(void* decCtx, int new_exp, char** zErrMsg)                { return SQLITE_ERROR; }
int   decimalSetRoundingMode(void* decCtx, char const* new_mode, char** zErrMsg) { return SQLITE_ERROR; }
int   decimalClearFlag(void* decCtx, char const* flag, char** zErrMsg)           { return SQLITE_OK; }
int   decimalSetFlag(void* decCtx, char const* flag, char** zErrMsg)             { return SQLITE_ERROR; }
char  const* decimalGetFlag(void* decCtx, size_t n)                              { return "Not implemented"; }
int   decimalClearTrap(void* decCtx, char const* flag, char** zErrMsg)           { return SQLITE_OK; }
int   decimalSetTrap(void* decCtx, char const* flag, char** zErrMsg)             { return SQLITE_ERROR; }
char  const* decimalGetTrap(void* decCtx, size_t n)                              { return "Not implemented"; }

#define SQLITE_DECIMAL_NOT_IMPL0(fun)                                  \
  void decimal ## fun(sqlite3_context* context) {                   \
    (void)context;                                                  \
    sqlite3_result_error(context, "Operation not implemented", -1); \
    return;                                                         \
  }

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

#define SQLITE_DECIMAL_NOT_IMPL3(fun)                                                                         \
  void decimal ## fun(sqlite3_context* context, sqlite3_value* v1, sqlite3_value* v2, sqlite3_value* v3) { \
    (void)context;                                                                                         \
    (void)v1;                                                                                              \
    (void)v2;                                                                                              \
    (void)v3;                                                                                              \
    sqlite3_result_error(context, "Operation not implemented", -1);                                        \
    return;                                                                                                \
  }

#define SQLITE_DECIMAL_NOT_IMPL_N(fun)                                               \
  void decimal ## fun(sqlite3_context* context, int argc, sqlite3_value** argv) { \
    (void)context;                                                                \
    (void)argc;                                                                   \
    (void)argv;                                                                   \
    sqlite3_result_error(context, "Operation not implemented", -1);               \
    return;                                                                       \
  }

#define SQLITE_DECIMAL_NOT_IMPL_AGGR(fun)                                                    \
  void decimal ## fun ## Step(sqlite3_context* context, int argc, sqlite3_value** argv) { \
    (void)context;                                                                        \
    (void)argc;                                                                           \
    (void)argv;                                                                           \
    sqlite3_result_error(context, "Operation not implemented", -1);                       \
    return;                                                                               \
  }                                                                                       \
  void decimal ## fun ## Final(sqlite3_context* context) {                                \
    (void)context;                                                                        \
    sqlite3_result_error(context, "Operation not implemented", -1);                       \
    return;                                                                               \
  }

SQLITE_DECIMAL_NOT_IMPL0(ClearStatus)
SQLITE_DECIMAL_NOT_IMPL0(Status)
SQLITE_DECIMAL_NOT_IMPL0(Version)
SQLITE_DECIMAL_NOT_IMPL1(Abs)
SQLITE_DECIMAL_NOT_IMPL1(Bits)
SQLITE_DECIMAL_NOT_IMPL1(Bytes)
SQLITE_DECIMAL_NOT_IMPL1(Class)
SQLITE_DECIMAL_NOT_IMPL1(Create)
SQLITE_DECIMAL_NOT_IMPL1(Digits)
SQLITE_DECIMAL_NOT_IMPL1(Exp)
SQLITE_DECIMAL_NOT_IMPL1(GetCoefficient)
SQLITE_DECIMAL_NOT_IMPL1(GetExponent)
SQLITE_DECIMAL_NOT_IMPL1(Invert)
SQLITE_DECIMAL_NOT_IMPL1(IsCanonical)
SQLITE_DECIMAL_NOT_IMPL1(IsFinite)
SQLITE_DECIMAL_NOT_IMPL1(IsInfinite)
SQLITE_DECIMAL_NOT_IMPL1(IsInteger)
SQLITE_DECIMAL_NOT_IMPL1(IsLogical)
SQLITE_DECIMAL_NOT_IMPL1(IsNaN)
SQLITE_DECIMAL_NOT_IMPL1(IsNegative)
SQLITE_DECIMAL_NOT_IMPL1(IsNormal)
SQLITE_DECIMAL_NOT_IMPL1(IsPositive)
SQLITE_DECIMAL_NOT_IMPL1(IsSigned)
SQLITE_DECIMAL_NOT_IMPL1(IsSubnormal)
SQLITE_DECIMAL_NOT_IMPL1(IsZero)
SQLITE_DECIMAL_NOT_IMPL1(Ln)
SQLITE_DECIMAL_NOT_IMPL1(Log10)
SQLITE_DECIMAL_NOT_IMPL1(LogB)
SQLITE_DECIMAL_NOT_IMPL1(Minus)
SQLITE_DECIMAL_NOT_IMPL1(NextDown)
SQLITE_DECIMAL_NOT_IMPL1(NextUp)
SQLITE_DECIMAL_NOT_IMPL1(Norm)
SQLITE_DECIMAL_NOT_IMPL1(Plus)
SQLITE_DECIMAL_NOT_IMPL1(Reduce)
SQLITE_DECIMAL_NOT_IMPL1(Sqrt)
SQLITE_DECIMAL_NOT_IMPL1(ToInt32)
SQLITE_DECIMAL_NOT_IMPL1(ToInt64)
SQLITE_DECIMAL_NOT_IMPL1(ToIntegral)
SQLITE_DECIMAL_NOT_IMPL1(ToString)
SQLITE_DECIMAL_NOT_IMPL1(Trim)

SQLITE_DECIMAL_NOT_IMPL2(And)
SQLITE_DECIMAL_NOT_IMPL2(Compare)
SQLITE_DECIMAL_NOT_IMPL2(Divide)
SQLITE_DECIMAL_NOT_IMPL2(DivideInteger)
SQLITE_DECIMAL_NOT_IMPL2(Equal)
SQLITE_DECIMAL_NOT_IMPL2(GreaterThan)
SQLITE_DECIMAL_NOT_IMPL2(GreaterThanOrEqual)
SQLITE_DECIMAL_NOT_IMPL2(LessThan)
SQLITE_DECIMAL_NOT_IMPL2(LessThanOrEqual)
SQLITE_DECIMAL_NOT_IMPL2(NotEqual)
SQLITE_DECIMAL_NOT_IMPL2(Or)
SQLITE_DECIMAL_NOT_IMPL2(Power)
SQLITE_DECIMAL_NOT_IMPL2(Quantize)
SQLITE_DECIMAL_NOT_IMPL2(Remainder)
SQLITE_DECIMAL_NOT_IMPL2(Rotate)
SQLITE_DECIMAL_NOT_IMPL2(SameQuantum)
SQLITE_DECIMAL_NOT_IMPL2(ScaleB)
SQLITE_DECIMAL_NOT_IMPL2(Shift)
SQLITE_DECIMAL_NOT_IMPL2(Subtract)
SQLITE_DECIMAL_NOT_IMPL2(Xor)

SQLITE_DECIMAL_NOT_IMPL3(FMA)

SQLITE_DECIMAL_NOT_IMPL_N(Add)
SQLITE_DECIMAL_NOT_IMPL_N(Max)
SQLITE_DECIMAL_NOT_IMPL_N(MaxMag)
SQLITE_DECIMAL_NOT_IMPL_N(Min)
SQLITE_DECIMAL_NOT_IMPL_N(MinMag)
SQLITE_DECIMAL_NOT_IMPL_N(Multiply)

SQLITE_DECIMAL_NOT_IMPL_AGGR(Sum)
SQLITE_DECIMAL_NOT_IMPL_AGGR(Min)
SQLITE_DECIMAL_NOT_IMPL_AGGR(Max)
SQLITE_DECIMAL_NOT_IMPL_AGGR(Avg)

