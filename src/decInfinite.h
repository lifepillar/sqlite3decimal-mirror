/**
 * \file      decInfinite.h
 * \author    Lifepillar
 * \copyright Copyright (c) 2019 Lifepillar.
 *            This program is free software; you can redistribute it and/or
 *            modify it under the terms of the Simplified BSD License (also
 *            known as the "2-Clause License" or "FreeBSD License".)
 * \copyright This program is distributed in the hope that it will be useful,
 *            but without any warranty; without even the implied warranty of
 *            merchantability or fitness for a particular purpose.
 *
 * \brief     Decimal Infinite public interface
 */

#include "autoconfig.h"

#if !defined(DECINFINITE)

#define DECINFINITE
#define DECINF_NAME "decInfinite"                 /**< Short name.          */
#define DECINF_FULLNAME "Decimal Infinite Module" /**< Full name.           */
#define DECINF_AUTHOR "Lifepillar"                /**< Who to bl... praise. */

#if HAVE_LITTLE_ENDIAN
#define DECLITEND 1         /**< 1=little-endian, 0=big-endian. */
#else
#define DECLITEND 0
#endif

#ifndef DECNUMDIGITS
/**
 * \brief Maximum number of digits that a decNumber can have.
 *
 * \note  #DECNUMDIGITS **must** be a multiple of three, because before
 *        encoding a decNumber its digits may be shifted towards the most
 *        significant unit (and, correspondingly, trailing zeroes may be added)
 *        so that each #Unit contains three digits.
 *
 * \note According to decNumber's manual, the theoretical upper limit for this
 *       constant is 999,999,999. Given the way #DECINF_MAXSIZE is defined,
 *       however, the maximum allowed value is "only" 99,999,999.
 */
#define DECNUMDIGITS 39
#endif

#if DECNUMDIGITS < 3
#error DECNUMDIGITS must be at least 3
#endif

#if DECNUMDIGITS % 3 != 0
#error DECNUMDIGITS must be a multiple of 3
#endif


#ifndef DECINF_EXPSIZE
/**
 * \brief Maximum size of the absolute value of an adjusted exponent, in bits.
 *
 * This must be at least 5 and no greater than 30. The lower bound is
 * determined by the fact that this implementation must be able to pack at
 * least those exponents that fit in one byte (i.e., 5 bits, once the bits for
 * the sign and padding are factored out). The upper limit depends on the fact
 * that the adjusted exponent of a decNumber must be in
 * [-999,999,999,999,999,999].
 */
#define DECINF_EXPSIZE 30
#endif

#if DECINF_EXPSIZE < 5
#error DECINF_EXPSIZE must be at least 5
#endif


/**
 * \brief Maximum size of a Decimal Infinite encoded number, in bytes.
 *
 * The encoding consists of:
 *
 * - `2` bits for the sign;
 * - `1` bit of padding (unused);
 * - At most `2 * #DECINF_EXPSIZE - 1` bits for the encoded exponent (sign and
 *   value);
 * - At most `10 * #DECNUMDIGITS / 3` bits for the mantissa.
 *
 * The number is rounded up to the nearest integer.
 */
#define DECINF_MAXSIZE (1 + (2 + 1 + (2 * DECINF_EXPSIZE - 1) + (10 * DECNUMDIGITS / 3) - 1) / 8)

#include "decNumber/decNumber.h"
#include "decNumber/decNumberLocal.h"

#if DECDPUN != 3
#error decInfinite assumes DECDPUN == 3
#endif

/**
 * \brief Encodes a decNumber into a stream of bytes.
 *
 * \param len The length of the output stream of bytes
 * \param result The output stream of bytes
 * \param decnum The input number
 *
 * \return The size of the result, in bytes
 *
 * \note This function does not check for buffer overflows; rather, it assumes
 *       that the output buffer is large enough to hold the encoded number.
 *       The maximum space an encoded number can use is #DECINF_MAXSIZE
 *       bytes. This function also assumes that the exponent of the input
 *       number is within the valid range (adjusted exponent in
 *       [-999,999,999,999,999,999]).
 *
 * \note The input number is **modified** by this function. If you need to use
 *       the number after invoking this function, **make a copy first.**
 */
size_t decInfiniteFromNumber(size_t len, uint8_t result[len], decNumber* decnum);

/**
 * \brief Decodes a Decimal Infinite byte stream into a \c decNumber.
 *
 * \param len The number of bytes of the encoded number
 * \param bytes The encoded number
 * \param decnum The output decNumber
 *
 * \return \a decnum unless a decoding error occurs, in which case the returned
 *         value is `0`.
 */
decNumber* decInfiniteToNumber(size_t len, uint8_t const bytes[len], decNumber* decnum);

/**
 * \brief Determines whether an encoded number is special.
 *
 * A special number is any of `-Inf`, `+Inf`, or `NaN`.
 *
 * \param len The number of bytes of the encoded number
 * \param bytes The encoded number
 *
 * \return `1` if the number is recognized as a special number; `0` otherwise.
 */
int decInfiniteIsSpecial(size_t len, uint8_t const bytes[len]);

/**
 * \brief Returns the sign of an encoded decimal.
 *
 * The sign is returned also for special numbers.
 *
 * \param bytes A non-null pointer to the encoded number
 *
 * \return `1` if the sign is positive, `-1` if the sign is negative.
 */
int decInfiniteSign(uint8_t const* bytes);

/**
 * \brief Returns the exponent of a decimal.
 *
 * \param len The number of bytes of the encoded decimal number
 * \param bytes The encoded decimal number
 *
 * \return The adjusted exponent of the decimal, i.e., the exponent of the
 *         number when expressed in scientific notation. If the decimal is
 *         a special number (`-NaN`, `-Inf`, `+Inf`, `+NaN`), the returned
 *         value is `-2^#DECINF_EXPSIZE` or `+2^#DECINF_EXPSIZE` (i.e., an
 *         out-of-range exponent).
 *
 */
int32_t decInfiniteExponent(size_t len, uint8_t const bytes[len]);

/**
 * \brief Returns the significand of a decimal as a string.
 *
 * \param len The number of bytes of the encoded number
 * \param bytes The encoded number
 * \param significand The output buffer: this is assumed to have enough space
 *        to hold the coefficient, i.e., at most `#DECNUMDIGITS + 2` bytes
 *        (#DECNUMDIGITS digits, a dot, the string terminator).
 *
 * \return \a significand, or `0` if a decoding error occurs.
 */
char* decInfiniteCoefficient(size_t len, uint8_t const bytes[len], char* significand);

/**
 * \brief Returns an encoded number as a hexadecimal string.
 *
 * This function is meant mostly for debugging (or for fun).
 *
 * \param len The number of bytes of the encoded number
 * \param bytes The encoded number
 * \param hexes A non-null pointer to a buffer for the output string. This
 *              function asssumes that the buffer has enough space, which is at
 *              most `3 * #DECINF_MAXSIZE` bytes.
 *
 * \return \a hexes unless \a len exceeds #DECINF_MAXSIZE, in which case the
 *         returned value is `0`.
 */
char* decInfiniteToBytes(size_t len, uint8_t const bytes[len], char* hexes);

/**
 * \brief Returns an encoded number as a formatted string of bits.
 *
 * This function is meant mostly for debugging (or for fun).
 *
 * \param len The number of bytes of the encoded number
 * \param bytes The encoded number
 * \param bitstring A non-null pointer to a buffer for the output string. This
 *        must have enough space for the output string, which is at most
 *        (approximately) `#DECINF_MAXSIZE * 9 + 1` bytes.
 *
 * \return \a bitstring unless \a len exceeds #DECINF_MAXSIZE, in which case
 *         the returned value is `0`.
 */
char* decInfiniteToBits(size_t len, uint8_t const bytes[len], char* bitstring);

#endif

