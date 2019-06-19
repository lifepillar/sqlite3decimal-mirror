/**
 * \file      impl_decimal.h
 * \author    Lifepillar
 * \copyright Copyright (c) 2019 Lifepillar.
 *            This program is free software; you can redistribute it and/or
 *            modify it under the terms of the Simplified BSD License (also
 *            known as the "2-Clause License" or "FreeBSD License".)
 * \copyright This program is distributed in the hope that it will be useful,
 *            but without any warranty; without even the implied warranty of
 *            merchantability or fitness for a particular purpose.
 *
 * \brief     Interface for implementations
 */
#ifndef sqlite3_decimal_impl_h
#define sqlite3_decimal_impl_h

#include <stdlib.h>
#include "sqlite3ext.h"
#include "decimal.h"

// Needed for dynamic linking
SQLITE_EXTENSION_INIT3

#pragma mark Context functions

/**
 * \brief Performs any initialization task required by the underlying decimal
 *        library.
 *
 * \return A pointer to a newly allocated decimal context.
 *
 * This function is called once at initialization time. The returned context
 * will be passed to any application function and to the modules implementing
 * the virtual tables.
 */
void* decimalInitSystem(void);

/**
 * \brief Creates a new decimal context initialized with default values.
 *
 * \return A pointer to the data structure describing the decimal context.
 */
void* decimalContextCreate(void);

/**
 * \brief Makes a copy of the source context into the target context.
 */
void decimalContextCopy(void* target, void* source);

/**
 * \brief Performs any clean up task required to dispose of the allocated context.
 *
 * \note This function does not guarantee that the passed pointer is zeroed out.
 */
void decimalContextDestroy(void* context);

#pragma mark Helper functions for context virtual table

/**
 * \brief Returns the current precision used for mathematical operations.
 */
int decimalPrecision(void* decCtx);

/**
 * \brief Returns the magnitude of the largest adjusted exponent that is permitted.
 */
int decimalMaxExp(void* decCtx);

/**
 * \brief Returns the smallest adjusted exponent that is permitted for normal
 *        operations.
 */
int decimalMinExp(void* decCtx);

/**
 * \brief Returns a string specifying the current rounding mode.
 *
 * The returned value is a constant string (no need for the caller to free it).
 *
 * \note The values returned by this function are implementation-defined.
 */
char const* decimalRoundingMode(void* decCtx);

/**
 * \brief Sets the current precision.
 *
 * \note For implementations having fixed precision (e.g., one based on
 *       decQuad), this function returns `SQLITE_ERROR` if the value of
 *       \a new_prec is different from the current precision.
 */
int decimalSetPrecision(void* decCtx, int new_prec, char** zErrMsg);

/**
 * \brief Sets the maximum adjusted exponent that is permitted.
 *
 * \note For implementations that do not allow changing the maximum adjusted
 *       exponent, this function returns `SQLITE_ERROR` if the value of
 *       \a new_exp is different from the current maximum exponent.
 */
int decimalSetMaxExp(void* decCtx, int new_exp, char** zErrMsg);

/**
 * \brief Sets the minimum adjusted exponent that is permitted.
 *
 * \note For implementations that do not allow changing the minimum adjusted
 *       exponent, this function returns `SQLITE_ERROR` if the value of
 *       \a new_exp is different from the current minimum exponent.
 */
int decimalSetMinExp(void* decCtx, int new_exp, char** zErrMsg);

/**
 * \brief Sets the current rounding mode.
 *
 * \note The allowed values for \a new_mode are implementation-defined.
 */
int decimalSetRoundingMode(void* decCtx, char const* new_mode, char** zErrMsg);

#pragma mark Helper functions for status virtual table

/**
 * \brief Resets an error or information flag.
 *
 * \return `SQLITE_OK` if the operation is successful; otherwise, a SQLite3
 *         error code.
 *
 * This function is used to implement deletion from the Status virtual table.
 *
 * \note The set of available flags is implementation-defined.
 */
int decimalClearFlag(void* decCtx, char const* flag, char** zErrMsg);

/**
 * \brief  Sets an error or information flag.
 *
 * \return `SQLITE_OK` if the operation is successful; otherwise, a SQLite3
 *         error code.
 *
 * This function is used to implement insertion into the Status virtual table.
 *
 * \note The set of available flags is implementation-defined.
 */
int decimalSetFlag(void* decCtx, char const* flag, char** zErrMsg);

/**
 * \brief Returns the \a n-th flag that is set.
 *
 * \return The name of the \a n-th flag (counting from `0`) that is set;
 *         returns `0` if less than \a n flags are set or if there are less
 *         than \a n flags.
 *
 * This function assumes that the available flags are totally ordered in some
 * way, which is implementation-defined. The function then returns the \a n-th
 * flag that is set in such order, or zero is no such flag exists.
 */
char const* decimalGetFlag(void* decCtx, size_t n);

#pragma mark Helper functions for traps virtual table

/**
 * \brief Resets a trap flag.
 *
 * \return `SQLITE_OK` if the operation is successful; otherwise, a SQLite3
 *         error code.
 *
 * This function is used to implement deletion from the traps virtual table.
 *
 * \note The same flags as in decimalClearFlag() are used.
 */
int decimalClearTrap(void* decCtx, char const* flag, char** zErrMsg);

/**
 * \brief  Sets a trap glag.
 *
 * \return `SQLITE_OK` if the operation is successful; otherwise, a SQLite3
 *         error code.
 *
 * This function is used to implement insertion into the traps virtual table.
 *
 * \note The same flags as in decimalClearFlag() are used.
 */
int decimalSetTrap(void* decCtx, char const* flag, char** zErrMsg);

/**
 * \brief Returns the \a n-th trap that is set.
 *
 * \return The name of the \a n-th trap flag (counting from `0`) that is set;
 *         returns `0` if less than \a n traps are set or if there are less
 *         than \a n traps.
 *
 * This function assumes that the available flags are totally ordered in some
 * way, which is implementation-defined. The function then returns the \a n-th
 * flag that is set in such order, or zero is no such flag exists.
 */
char const* decimalGetTrap(void* decCtx, size_t n);

#pragma mark Operations

// Operations
#define SQLITE_DECIMAL_OP0_DECL(fun) void decimal ## fun(sqlite3_context* context);
#define SQLITE_DECIMAL_OP1_DECL(fun) void decimal ## fun(sqlite3_context* context, sqlite3_value* x);
#define SQLITE_DECIMAL_OP2_DECL(fun) void decimal ## fun(sqlite3_context* context, sqlite3_value* x, sqlite3_value* y);
#define SQLITE_DECIMAL_OPn_DECL(fun) void decimal ## fun(sqlite3_context* context, int argc, sqlite3_value** argv);
// Aggregates
#define SQLITE_DECIMAL_AGGR_DECL(fun) \
  void decimal ## fun ## Step(sqlite3_context* context, int argc, sqlite3_value** argv); \
void decimal ## fun ## Final(sqlite3_context* context);

#pragma mark Nullary functions

/**
 * \brief Resets all the flags of the shared context.
 *
 * Invoking this function is equivalent to (but possibly more efficient than)
 * the following SQL command:
 *
 *     delete from decStatus;
 */
SQLITE_DECIMAL_OP0_DECL(ClearStatus)

  /**
   * \brief Returns a textual, implementation-defined, representation of the
   *        current status.
   *
   * \note This function is meant mostly for debugging.
   */
SQLITE_DECIMAL_OP0_DECL(Status)

  /**
   * \brief Returns the version of this extension as text.
   *
   * The version string should always begin with the value of
   * #SQLITE_DECIMAL_VERSION. An implementation may add further details, such
   * as the version of the underlying library implementing decimal arithmetic.
   */
SQLITE_DECIMAL_OP0_DECL(Version)

#pragma mark Unary functions

  /**
   * \brief Returns the absolute value of a decimal.
   *
   * NaNs are propagated.
   */
SQLITE_DECIMAL_OP1_DECL(Abs)

  /**
   * \brief Converts a decimal blob into a string of bits, ordered MSB to LSB.
   *
   * \note This is useful mostly for debugging and may not be available in some
   *       implementations.
   */
SQLITE_DECIMAL_OP1_DECL(Bits)

  /**
   * \brief Converts a decimal blob into a string of hexadecimal values.
   *
   * \note This is useful mostly for debugging and may not be available in some
   *       implementations.
   */
SQLITE_DECIMAL_OP1_DECL(Bytes)

  /**
   * \brief Returns the class of a decimal as text.
   *
   * The specific strings returned by this functions are implementation-defined.
   */
SQLITE_DECIMAL_OP1_DECL(Class)

  /**
   * \brief Creates a decimal number.
   *
   * An implementation of this function should support at least conversion from
   * text and from a blob.
   */
SQLITE_DECIMAL_OP1_DECL(Create)

  /**
   * \brief Returns the number of significant digits in a given decimal.
   *
   * The values returned by this function when the argument is a zero or it is
   * not a finite number (i.e., it is an infinite or a NaN) are
   * implementation-defined.
   */
SQLITE_DECIMAL_OP1_DECL(Digits)

  /**
   * \brief Calculates `e^x` where `x` is the argument of the function.
   *
   * Finite results are always full precision and inexact, except when the
   * argument is a zero or `-Inf` (giving `1` and `0`, respectively). If the
   * argument is `+Inf` then the result is `+Inf`. NaNs propagate to the
   * result.
   *
   * \note Rounding rules are implementation-defined.
   */
SQLITE_DECIMAL_OP1_DECL(Exp)

  /**
   * \brief Returns the coefficient (significand) of the given decimal as text.
   */
SQLITE_DECIMAL_OP1_DECL(GetCoefficient)

  /**
   * \brief Returns the exponent of the given decimal as an integer.
   */
SQLITE_DECIMAL_OP1_DECL(GetExponent)

  /**
   * \brief Carries out the digit-wise logical inversion of the given decimal
   *        number.
   *
   * The operand must be zero or a finite positive integer consisting only of
   * zeroes and ones, otherwise an error is raised.
   */
SQLITE_DECIMAL_OP1_DECL(Invert)

  /**
   * \brief Returns `1` if the given decimal number's encoding is canonical;
   *        returns `0` otherwise.
   *
   * The returned value is a SQLite3 integer, not a decimal.
   */
SQLITE_DECIMAL_OP1_DECL(IsCanonical)

  /**
   * \brief Returns `1` if the given decimal number is finite, `0` otherwise.
   *
   * This function returns `1` if its argument is neither infinite nor a NaN;
   * returns `0` otherwise. The returned value is a SQLite3 integer, not
   * a decimal.
   */
SQLITE_DECIMAL_OP1_DECL(IsFinite)

  /**
   * \brief Returns `1` if the given decimal number is an infinity; returns `0`
   *        otherwise.
   *
   * The returned value is a SQLite3 integer, not a decimal.
   */
SQLITE_DECIMAL_OP1_DECL(IsInfinite)

  /**
   * \brief Returns `1` if the given decimal number is finite and has exponent
   *        zero; returns `0` otherwise.
   *
   * The returned value is a SQLite3 integer, not a decimal.
   */
SQLITE_DECIMAL_OP1_DECL(IsInteger)

  /**
   * \brief Returns `1` if the given decimal number is a valid argument for
   *        logical operations; return `0` otherwise.
   *
   * The returned value is a SQLite3 integer, not a decimal.
   */
SQLITE_DECIMAL_OP1_DECL(IsLogical)

  /**
   * \brief Returns `1` if the given decimal number is `NaN`; returns `0`
   *        otherwise.
   *
   * The returned value is a SQLite3 integer, not a decimal.
   */
SQLITE_DECIMAL_OP1_DECL(IsNaN)

  /**
   * \brief Returns `1` if the given decimal number is (strictly) less than
   *        zero and not a `NaN`; returns `0` otherwise.
   *
   * The returned value is a SQLite3 integer, not a decimal.
   *
   * \note `-0` is still zero, so this function returns `0` is its argument is
   *       `-0`.
   */
SQLITE_DECIMAL_OP1_DECL(IsNegative)

  /**
   * \brief Returns `1` if the given decimal number is normal; returns `0`
   *        otherwise.
   *
   * The returned value is a SQLite3 integer, not a decimal.
   *
   * A number is **normal** iff it is finite, non-zero and not subnormal, i.e.,
   * with magnitude greater than or equal to `10^emin`, where `emin` is the
   * value returned by decMinExp().
   *
   * \note Zero is neither normal nor subnormal.
   */
SQLITE_DECIMAL_OP1_DECL(IsNormal)

  /**
   * \brief Returns `1` if the given decimal number is (strictly) greater than
   *        zero and not a `NaN`; returns `0` otherwise.
   *
   * The returned value is a SQLite3 integer, not a decimal.
   */
SQLITE_DECIMAL_OP1_DECL(IsPositive)

  /**
   * \brief Returns `1` if the given decimal number has a negative sign;
   *        returns `0` otherwise.
   *
   * The returned value is a SQLite3 integer, not a decimal.
   *
   * \note Zeroes and NaNs may have a negative signs.
   */
SQLITE_DECIMAL_OP1_DECL(IsSigned)

  /**
   * \brief Returns `1` if the given decimal number is subnormal; returns `0`
   *        otherwise.
   *
   * The returned value is a SQLite3 integer, not a decimal.
   *
   * A number is **subnormal** iff it is finite, non-zero and with magnitude
   * less than `10^emin` where `emin` is the value returned by decMinExp().
   *
   * \note Zero is neither normal nor subnormal.
   */
SQLITE_DECIMAL_OP1_DECL(IsSubnormal)

  /**
   * \brief Returns `1` if the given decimal number is zero; returns `0`
   *        otherwise.
   *
   * The returned value is a SQLite3 integer, not a decimal.
   */
SQLITE_DECIMAL_OP1_DECL(IsZero)

  /**
   * \brief Calculates the natural logarithm of the argument.
   *
   * Finite results are always full precision and inexact, except when the
   * argument is equal to `1`, which gives an exact result of `0`.
   *
   * \note: Rounding rules are implementation-defined.
   */
SQLITE_DECIMAL_OP1_DECL(Ln)

  /**
   * \brief Calculates the base-10 logarithm of the argument.
   *
   * Finite results will always be full precision and inexact, except when the
   * argument is equal to an integral power of ten, in which case the result is
   * the exact integer.
   *
   * \note: Rounding rules are implementation-defined.
   */
SQLITE_DECIMAL_OP1_DECL(Log10)

  /**
   * \brief Returns the adjusted exponent of the given number, according to IEEE
   *        754 rules.
   *
   * The exponent returned is calculated as if the decimal point followed the
   * first significant digit. So, for example, if the number is `123` then the
   * result is `2`.
   *
   * If the given decimal number is infinite, the result is `+Infinity`. If it is
   * a zero, the result is `-Infinity`, and an implementation-defined flag
   * denoting an invalid operation is set. If it is less than zero, the absolute
   * value of the number is used. If the number is `1` then the result is `0`.
   * NaNs are handled (propagated) as for arithmetic operations.
   */
SQLITE_DECIMAL_OP1_DECL(LogB)

  /**
   * \brief Negates a number.
   *
   * Returns `0-x` where `x` is the given decimal number and the exponent of the
   * zero is the same as that of `x` (if `x` is finite). `NaN`s are handled as
   * for arithmetic operations (the sign of a `NaN` is not affected). A zero
   * result has positive sign.
   */
SQLITE_DECIMAL_OP1_DECL(Minus)

  /**
   * \brief Returns the “next” decimal number in the direction of -Infinity
   *        according to IEEE 754 rules for “nextDown”.
   */
SQLITE_DECIMAL_OP1_DECL(NextDown)

  /**
   * \brief Returns the “next” decimal number in the direction of +Infinity
   *        according to IEEE 754 rules for “nextUp”.
   */
SQLITE_DECIMAL_OP1_DECL(NextUp)

SQLITE_DECIMAL_OP1_DECL(Norm)

  /**
   * \brief Returns `0+x` where `x` is the given decimal number.
   *
   * Returns `0+x` where `x` is the given decimal number and the exponent of the
   * zero is the same as that of `x` (if `x` is finite). NaNs are handled as
   * for arithmetic operations (the sign of a NaN is not affected). A zero
   * result has positive sign.
   */
SQLITE_DECIMAL_OP1_DECL(Plus)

  /**
   * \brief Reduces the coefficient of the given decimal to its shortest possible
   *        form without changing the value of the result.
   *
   * This removes all possible trailing zeros from the coefficient (some may
   * remain when the number is very close to the most positive or most negative
   * number). Infinities and NaNs are unchanged. If the argument is a zero the
   * exponent of the result is zero.
   */
SQLITE_DECIMAL_OP1_DECL(Reduce)

  /**
   * \brief Calculates the square root of the argument.
   *
   * \note Rounding rules are implementation-defined.
   */
SQLITE_DECIMAL_OP1_DECL(Sqrt)

  /**
   * \brief Returns the given decimal rounded to a 32-bit integer.
   *
   * The current rounding mode is used.
   */
SQLITE_DECIMAL_OP1_DECL(ToInt32)

  /**
   * \brief Returns the given decimal rounded to a 64-bit integer.
   *
   * The current rounding mode is used.
   */
SQLITE_DECIMAL_OP1_DECL(ToInt64)

  /**
   * \brief Rounds the given decimal to an integral value.
   *
   * If the number is an infinite, Infinity of the same sign is returned. If it
   * is a NaN, the result is as for other arithmetic operations. No flag is set
   * by this function, even if rounding occurs. The result is still a decimal
   * number.
   *
   * \note The current rounding mode is used.
   */
SQLITE_DECIMAL_OP1_DECL(ToIntegral)

  /**
   * \brief Returns a textual representation of a decimal blob.
   */
SQLITE_DECIMAL_OP1_DECL(ToString)

  /**
   * \brief Removes insignificant trailing zeroes from a number, unconditionally.
   *
   * If the number has any fractional trailing zeroes, they are removed by
   * dividing the coefficient by the appropriate power of ten and adjusting the
   * exponent accordingly. See `decReduce()` if you want to remove all trailing
   * zeroes.
   */
SQLITE_DECIMAL_OP1_DECL(Trim)

#pragma mark Binary functions

  /**
   * \brief Carries out the digit-wise logical And of two bit sequences.
   *
   * The operands must be integer, zero or (finite) positive and consist only of
   * zeroes and ones; otherwise, the operation results in an error (e.g.,
   * `'Invalid operation'`).
   */
SQLITE_DECIMAL_OP2_DECL(And)

  /**
   * \brief Compares two numbers numerically.
   *
   * If the first argument is less than the second, the result is `-1`. If the
   * first argument is greater than the second, the result is `1`. If they are
   * equal, the result is `0`. If the operands are not comparable (i.e., one or
   * both are NaN) then the result is `NaN`.
   *
   * \note The result is a decimal, not a SQLite3 integer: you may use
   *       decToInt32() to convert it to an integer.
   */
SQLITE_DECIMAL_OP2_DECL(Compare)

  /**
   * \brief Performs the division of two decimals.
   *
   * NaNs inputs propagate to the result.
   */
SQLITE_DECIMAL_OP2_DECL(Divide)

  /**
   * \brief Performs integer division of two decimals.
   *
   * The returned value is a decimal, not a SQLite3 integer.
   *
   * \note Integer division is an exact operation. For instance, dividing `5.2`
   *       by `0.3` has the exact result `17`: no errors are raised and no
   *       status flags are set. However, if the result cannot be represented
   *       (because it has too many digits) then an error is raised (e.g.,
   *       `Division impossible`).
   */
SQLITE_DECIMAL_OP2_DECL(DivideInteger)

  /**
   * \brief Returns `1` if the two decimals are (mathematically) equal; returns
   *       `NaN` if one or both the arguments are NaNs; returns `0` otherwise.
   *
   * \note The returned value is a decimal, not a SQLite3 integer. Use
   *       decToInt32() to convert it to an integer.
   */
SQLITE_DECIMAL_OP2_DECL(Equal)

  /**
   * \brief Returns `1` if the first decimal is strictly greater than the second;
   *       returns `NaN' if one or both the arguments are NaNs; returns `0`
   *       otherwise.
   *
   * \note The returned value is a decimal, not a SQLite3 integer. Use
   *       decToInt32() to convert it to an integer.
   */
SQLITE_DECIMAL_OP2_DECL(GreaterThan)

  /**
   * \brief Returns `1` if the first decimal is greater than or equal to the
   *        second; returns `NaN' if one or both the arguments are NaNs;
   *        returns `0` otherwise.
   *
   * \note The returned value is a decimal, not a SQLite3 integer. Use
   *       decToInt32() to convert it to an integer.
   */
SQLITE_DECIMAL_OP2_DECL(GreaterThanOrEqual)

  /**
   * \brief Returns `1` if the first decimal is strictly less than the second;
   *        returns `NaN' if one or both the arguments are NaNs; returns `0`
   *        otherwise.
   *
   * \note The returned value is a decimal, not a SQLite3 integer. Use
   *       decToInt32() to convert it to an integer.
   */
SQLITE_DECIMAL_OP2_DECL(LessThan)

  /**
   * \brief Returns `1` if the first decimal is less than or equal to the second;
   *        returns `NaN' if one or both the arguments are NaNs; returns `0`
   *        otherwise.
   *
   * \note The returned value is a decimal, not a SQLite3 integer. Use
   *       decToInt32() to convert it to an integer.
   */
SQLITE_DECIMAL_OP2_DECL(LessThanOrEqual)

  /**
   * \brief Returns `1` if the two decimal are not (mathematically) equal;
   *        returns `NaN' if one or both the arguments are NaNs; returns `0`
   *        otherwise.
   *
   * \note The returned value is a decimal, not a SQLite3 integer. Use
   *       decToInt32() to convert it to an integer.
   */
SQLITE_DECIMAL_OP2_DECL(NotEqual)

  /**
   * \brief Carries out the digit-wise logical Or of two bit sequences.
   *
   * The operands must be integer, zero or (finite) positive and consist only of
   * zeroes and ones; otherwise, the operation results in an error.
   */
SQLITE_DECIMAL_OP2_DECL(Or)

  /**
   * \brief Raises the first argument to the power of the second argument.
   *
   * Inexact results are always full precision.
   *
   * \note Rounding rules are implementation-defined.
   */
SQLITE_DECIMAL_OP2_DECL(Power)

  /**
   * \brief Modifies a number so that its exponent has a specific value, equal to
   *        the exponent of the second argument.
   */
SQLITE_DECIMAL_OP2_DECL(Quantize)

  /**
   * \brief Returns the remainder of the integer division of the arguments.
   *
   * \note As for decDivideIntegerFunc(), it must be possible to express the
   *       intermediate result as a decimal integer. If the result has too many
   *       digits, an error is raised.
   */
SQLITE_DECIMAL_OP2_DECL(Remainder)

  /**
   * \brief Rotates the digits of the coefficient without adjusting the exponent.
   *
   * This function rotates the digits of the coefficient of the first argument to
   * the left (if the second argument is positive) or to the right (if the second
   * argument is negative) without adjusting the exponent or the sign of the
   * first argument.
   *
   * The second argument is the count of positions to rotate and it must be a
   * finite integer. The range of allowed values is implementation-defined.
   */
SQLITE_DECIMAL_OP2_DECL(Rotate)

  /**
   * \brief Returns `1` if the operands have the same exponent or are both `NaN`
   *        or both infinite; return `0` otherwise.
   */
SQLITE_DECIMAL_OP2_DECL(SameQuantum)

  /**
   * \brief Calculates `x × 10^y`, where `x` is the first argument and `y` is the
   *        second argument, which must be an integer.
   *
   * The range of allowed integers for the second argument is
   * implementation-defined.
   *
   * \note Overflow and underflow may occur.
   */
SQLITE_DECIMAL_OP2_DECL(ScaleB)

  /**
   * \brief Shifts the digits of the coefficient without adjusting the exponent.
   *
   * This function shifts the digits of the coefficient of the first argument to
   * the left (if the second argument is positive) or to the right (if the second
   * argument is negative) without adjusting the exponent or the sign of the
   * first argument. Any digits “shifted in” from the left or from the right will
   * be zero.
   *
   * The second argument is the count of positions to shift and it must be a
   * finite integer. The range of allowed values is implementation-defined.
   */
SQLITE_DECIMAL_OP2_DECL(Shift)

  /**
   * \brief Subtracts the second argument from the first.
   */
SQLITE_DECIMAL_OP2_DECL(Subtract)

  /**
   * \brief Carries out the digit-wise logical exclusive Or of two bit sequences.
   *
   * The operands must be integer, zero or (finite) positive and consist only of
   * zeroes and ones; otherwise, the operation results in an error.
   */
SQLITE_DECIMAL_OP2_DECL(Xor)

#pragma mark Ternary functions

  /**
   * \brief Calculates the fused multiply-add `v1 × v2 + v3`.
   *
   * NaNs propagate to the results.
   */
  void decimalFMA(sqlite3_context* context, sqlite3_value* v1, sqlite3_value* v2, sqlite3_value* v3);

#pragma mark Variadic functions

  /**
   * \brief Adds up zero or more decimals.
   *
   * \note The sum of zero decimals (i.e., when the function is called without
   *       arguments) is `0`, not `null`. If any of the arguments is `null`
   *       then the result is `null`. If any of the argument is a NaN then the
   *       result is a NaN.
   */
SQLITE_DECIMAL_OPn_DECL(Add)

  /**
   * \brief Returns the maximum of zero or more numbers.
   *
   * If any of the arguments is `NULL` then the result is `NULL`. Otherwise,
   * if all arguments are `NaN` then the result is `NaN`. If some, but not all
   * arguments are `NaN`, the maximum is computed as if there were no `NaN`.
   *
   * \note When this function is invoked without arguments, it returns +Inf.
   */
SQLITE_DECIMAL_OPn_DECL(Max)

  /**
   * \brief Returns the decimal with maximum absolute value among the given
   *        decimals.
   *
   * This is like decMaxFunc() except that the absolute values of the arguments
   * are used for comparisons.
   *
   * \note When this function is invoked without arguments, it returns +Inf.
   */
SQLITE_DECIMAL_OPn_DECL(MaxMag)

  /**
   * \brief Returns the minimum of zero or more decimal numbers.
   *
   * If any of the arguments is `NULL` then the result is `NULL`. Otherwise,
   * if all arguments are `NaN` then the result is `NaN`. If some, but not all
   * arguments are `NaN`, the minimum is computed as if there were no `NaN`.
   *
   * \note When this function is invoked without arguments, it returns -Inf.
   */
SQLITE_DECIMAL_OPn_DECL(Min)

  /**
   * \brief Returns the decimal with minimum absolute value among the given
   *        decimals.
   *
   * This is like decMinFunc() except that the absolute values of the arguments
   * are used for comparisons.
   *
   * \note When this function is invoked without arguments, it returns zero.
   */
SQLITE_DECIMAL_OPn_DECL(MinMag)

  /**
   * \brief Multiplies zero or more decimals.
   *
   * If any of the arguments is `NULL` then the result is `NULL`. Otherwise,
   * if all arguments are `NaN` then the result is `NaN`. If some, but not all
   * arguments are `NaN`, the product is computed as if there were no `NaN`.
   *
   * \note When this function is invoked without arguments, it returns `1`.
   */
SQLITE_DECIMAL_OPn_DECL(Multiply)

#pragma mark Aggregate functions

  /**
   * \brief Aggregate sum for decimals.
   */
SQLITE_DECIMAL_AGGR_DECL(Sum)

  /**
   * \brief Aggregate min for decimals.
   */
SQLITE_DECIMAL_AGGR_DECL(Min)

  /**
   * \brief Aggregate max for decimals.
   */
SQLITE_DECIMAL_AGGR_DECL(Max)

  /**
   * \brief Aggregate average for decimals.
   */
SQLITE_DECIMAL_AGGR_DECL(Avg)

#endif /* sqlite3_decimal_impl_h */

