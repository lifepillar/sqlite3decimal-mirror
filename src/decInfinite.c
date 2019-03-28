/**
 * \file      decInfinite.c
 * \author    Lifepillar
 * \copyright Copyright (c) 2019 Lifepillar.
 *            This program is free software; you can redistribute it and/or
 *            modify it under the terms of the Simplified BSD License (also
 *            known as the "2-Clause License" or "FreeBSD License".)
 * \copyright This program is distributed in the hope that it will be useful,
 *            but without any warranty; without even the implied warranty of
 *            merchantability or fitness for a particular purpose.
 *
 * \brief     Support for encoding and decoding a decNumber into and from (a
 *            slight variant of) the decimalInfinite format.
 *
 * The decimalInfinite format is an order-preserving binary encoding that
 * supports arbitrarily large or small decimals (i.e., any number that can be
 * represented with a finite number of digits). "Order-preserving" means that,
 * for any two decimals m and n, m < n if and only if enc(m) lexicographically
 * precedes enc(n), where enc() is the decimalInfinite encoding function. This
 * property makes decimalInfinite a suitable encoding for database
 * applications, because it permits direct comparisons on encoded decimals,
 * hence it does not require anything special for indexing (i.e, B-trees on
 * encoded decimals may be used).
 *
 * The format is fully described in:
 *
 *     Fourny, Ghislain
 *     decimalInfinite: All Decimals In Bits. No Loss. Same Order. Simple.
 *     arXiv:1506.01598v2
 *     17 Jun 2015
 *
 * This following description is not meant to replace a good reading.
 *
 * Zeroes and special numbers (`+Inf`, `-Inf`, `NaN`) are encoded in one byte;
 * all other numbers occupy two or more bytes. Special numbers are encoded as
 * follows:
 *
 * - `-Inf` is encoded as       `00 000000`;
 * - `-0` is encoded as         `01 000000`;
 * - `+0` is encoded as         `10 000000`;
 * - `+Inf` is encoded as       `110 00000`;
 * - `NaN` is encoded as        `111 00000`.
 *
 * The encoding of all finite, negative, non-zero decimals starts with `00`.
 * The encoding of all finite, positive, non-zero decimals starts with `10`.
 * Since all finite numbers are encoded in two or more bytes, all negative
 * numbers are strictly between `-Inf` and `-0`, and all positive numbers are
 * strictly between `+0` and `+Inf`.
 *
 * It is well known that every real number can be written in the form:
 *
 *   S x M x 10^(T x E)
 *
 * where S is the sign (-1 or +1); M is the significand, which is a real in the
 * interval [0,10); T is the sign of the exponent (-1 or +1); and E is a natural
 * number (the absolute value of the exponent).
 *
 * decimalInfinite represents each number by encoding each of S, T, E, and
 * M separately, then concatenating the results in that order (of course, this
 * assumes that M has a finite number of digits). We have already said that
 * S is encoded as either `00` (-1) or `10` (+1). In this implementation of
 * decimalInfinite, the sign of the exponent T is encoded in the *fourth* bit
 * (the third bit is unused and is always zero) according to the following
 * scheme:
 *
 * - `0000`: negative sign, non-negative exponent;
 * - `0001`: negative sign, negative exponent;
 * - `1000`: positive sign, negative exponent;
 * - `1001`: positive sign, non-negative exponent.
 *
 * (The reason why we skip the third bit is that, by doing so, the declets of
 * the mantissa gets aligned on even bits: in particular, no declet spans three
 * bytes).
 *
 * The absolute value of the exponent E is encoded in two steps using
 * a modified Gamma code. Gamma codes are variable-length self-delimiting
 * encodings (i.e., prefix codes) of the natural numbers. The basic idea is to
 * encode the length of a binary number in unary, followed by the binary
 * representation of the number. For instance, 9 (`1001` in binary) might be
 * encoded (in a sub-optimal way) as `1111` `0` `1001` (the zero in the middle
 * acting as a separator, signaling where the unary encoding of the number's
 * length stops). Gamma codes are cleverer, though: if the number is offset by
 * one, it can be assumed that the most significant bit of its binary
 * representation is always `1`, hence it is not necessary to encode such bit.
 * So, the Gamma code of 9 is in fact just `111` `0` `010` (i.e., the naïve
 * Gamma encoding of 9+1 with the leading `1` dropped). It is easy to verify
 * that this Gamma code is order-preserving.
 *
 * In decimalInfinite, the exponent is offset by two instead of one, so that
 * the unary prefix is always at least two bits long (this permits to encode
 * the length as well as the sign of the exponent in the unary prefix). The
 * first step (pre-encoding) of E is as follows: let N be the number of bits of
 * the binary representation of E+2. The pre-encoding of E is a sequence of N-1
 * ones followed by a zero followed by the trailing N-1 bits of E+2.
 *
 * For instance, if E=9 then the binary representation of E+2 is `1011`, hence
 * N=4. The pre-encoding of E is then written as `111` `0` `011`. As another
 * example, if E=0 then N=2 (because E+2 is `10` in binary). Hence, the
 * pre-encoding of 0 is `100`.
 *
 * In the second step, if T (i.e., the fourth bit of the overall encoding of
 * the number) is `0` then the pre-encoding is bitwise complemented; otherwise
 * it is left unchanged. Finally, since the first bit of the encoded absolute
 * exponent is always equal to T, the first bit of the encoded absolute
 * exponent can be dropped (or, equivalently, the encoded exponent can be
 * appended starting at the fourth bit). An N-bit (offset) exponent is
 * therefore encoded (sign included) as TE using 2N-1 bits.
 *
 * Finally, the significand M must be encoded. If the decimal is negative, 10-M
 * is encoded instead of M, in order to preserve the lexicographic ordering of
 * the binary encoding (in this case, the mantissa is in the interval (0,9], so
 * its first digit may be zero). In the paper, the first digit is encoded in
 * four bits in the natural way; the rest is divided into groups of three
 * digits, and each group is encoded into a declet (ten bits) in the natural
 * way, i.e., by simply converting each three-digit decimal number to binary.
 * Any order-preserving encoding of the significand will do, though: in this
 * implementation we have found it more convenient to encode everything in
 * declets, including the first digit.
 *
 * Note that, when comparing two numbers in decimalInfinite format, their
 * significands need to be compared only if the two numbers have the same sign
 * and the same (signed) exponent.
 *
 * To summarise, the essential ingredients of decimalInfinite are the following:
 *
 * - the sign of the number is encoded in the first two bits, so that special
 *   numbers can be accounted for in an order-preserving way as well;
 * - the sign of the exponent is encoded next, in one bit;
 * - the absolute value of the exponent is encoded using a prefix code, so
 *   exponents of arbitrary lengths can be stored while still preserving the
 *   lexicographic order of the encoding;
 * - the significand is encoded last, using a simple, order-preserving,
 *   compression scheme.
 *
 * An example: 1.9 can be written as +1 x 1.9 x 10^(+1 x 0). Hence, its
 * encoding is:
 *
 *     S = 10
 *     T = 1
 *     E = 00
 *     M = 0010111110 (i.e., 190 in 10 bits)
 *
 *     That is, in this implementation: 10 0 1 00 001011110
 *                                      |  | | |  |
 *                                  S --   | | |  |
 *                       Pad (unused) -----  | |  |
 *                                  T -------  |  |
 *                                  E ---------   |
 *                                  M ------------
 *
 * Note that, in general, it may be necessary to add padding zeroes to the last
 * group of digits of the significand so that each group has three digits.
 *
 * Another example: -199.8 is -1 x 1.998 x 10^(+1 x 2) and encoded as follows:
 *
 *     S = 00
 *     T = 0
 *     E = 0111
 *     M = 1100100000 0011001000 (i.e., 800 and 200 in declets)
 *
 * Note that, since the number is negative, 10 - 1.998 = 8.002 is encoded.
 */

#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#include "decInfinite.h"
#include "decNumber/decNumberLocal.h" // This *must* come after decInfinite.h

static_assert(DECDPUN == 3, "decInfinite assumes DECDPUN == 3");
static_assert(DECNUMDIGITS >= 3, "DECNUMDIGITS must be at least 3");
static_assert(DECNUMDIGITS % 3 == 0, "DECNUMDIGITS must be a multiple of 3");
static_assert(DECINF_EXPSIZE >= 5, "DECINF_EXPSIZE must be at least 5");

/**
 * \brief Tests if a decNumber, known to be finite, is zero.
 *
 * This is *slightly* more efficient than calling decNumberIsZero(), but
 * works only for finite numbers.
 */
#define finiteDecNumberIsZero(dn) (*(dn)->lsu == 0 && (dn)->digits == 1)

/**
 * \brief Magic constants for divisions by powers of ten.
 *
 * Constant multipliers for divide-by-power-of five using reciprocal
 * multiply, after removing powers of 2 by shifting, and final shift
 * of 17.
 *
 * \see decNumber.c
 */
static uInt const multies[] = {131073, 26215, 5243, 1049, 210};

/**
 * \brief Returns the quotient of unit \a u divided by `10**n`.
 *
 * \see decNumber.c
 */
#define QUOT10(u,n) ((((uInt)(u) >> (n)) * multies[n]) >> 17)

/**
 * \brief Array of masks to clear the high bits of a byte or an integer.
 *
 * The i-th entry clears the 32-i most significant bits of a `uByte` or
 * `uInt`.
 */
static uInt const MASK[33] = {
  0x00000000,
  0x00000001, 0x00000003, 0x00000007, 0x0000000F, 0x0000001F, 0x0000003F, 0x0000007F, 0x000000FF,
  0x000001FF, 0x000003FF, 0x000007FF, 0x00000FFF, 0x00001FFF, 0x00003FFF, 0x00007FFF, 0x0000FFFF,
  0x0001FFFF, 0x0003FFFF, 0x0007FFFF, 0x000FFFFF, 0x001FFFFF, 0x003FFFFF, 0x007FFFFF, 0x00FFFFFF,
  0x01FFFFFF, 0x03FFFFFF, 0x07FFFFFF, 0x0FFFFFFF, 0x1FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF, 0xFFFFFFFF,
};


typedef uint_fast8_t uCount; /**< Used for small counts (up to 255). */

/**
 * \brief Represents a position in a stream of bits.
 */
struct bitPos {
  uByte* pos;  /**< A pointer to the current byte. */
  uCount free; /**< The number of available bits in the current byte. */
};

typedef struct bitPos bitPos;

/**
 * \brief Helper structure used for encoding and decoding a declet.
 */
union dUnit {
  Unit declet;
  uByte byte[2];
};

typedef union dUnit dUnit;

#ifdef DECLITEND
/** \brief The low byte of a declet (endian-dependent). */
#define DECLET_LO(u) (u).byte[0]
/** \brief The high byte of a declet (endian-dependent). */
#define DECLET_HI(u) (u).byte[1]
#else
#define DECLET_LO(u) (u).byte[1]
#define DECLET_HI(u) (u).byte[0]
#endif


#pragma mark Private functions

/**
 * \brief Finds the base-2 integer logarithm of an integer number.
 *
 * \param v A number
 * \return The base-2 logarithm of \a v, that is, the position of the highest
 *         bit set. This is also one less than the number of bits in the binary
 *         representation of \a v.
 *
 * \see https://graphics.stanford.edu/~seander/bithacks.html rules!
 */
static inline uInt ilog2(uInt v) {
#define LT(n) n, n, n, n, n, n, n, n, n, n, n, n, n, n, n, n
  static char const LogTable256[256] = {
    -1, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
    LT(4), LT(5), LT(5), LT(6), LT(6), LT(6), LT(6),
    LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7)
  };

  uCount r;            // r will be lg(v)
  register uInt t, tt; // temporaries

  if ((tt = v >> 16))
    r = (t = tt >> 8) ? 24 + LogTable256[t] : 16 + LogTable256[tt];
  else
    r = (t = v >> 8) ? 8 + LogTable256[t] : LogTable256[v];

  return r;
}

/**
 * \brief Returns the number of bits of the encoded exponent.
 *
 * Counts the number of consecutive bits equal to the fourth bit of the
 * encoding. This gives the number of bits of the encoded exponent.
 *
 * This function is meant to be used only by decInfiniteUnpackExponent().
 *
 * \param p The current position in the byte stream
 * \param end Pointer one past the end of the byte stream
 *
 * \return The number of bits of the encoded exponent, or `0` if the length of
 *         the unary prefix exceeds the maximum size (no more than
 *         #DECINF_EXPSIZE bits).
 */
static uCount decInfiniteReadUnaryPrefix1(bitPos* p, uByte const* end) {
  uCount n;
  // Check if the last 5 bits of the first byte are all ones
  if ((*p->pos & 0x1F) == 0x1F) {
    n = 5;
    ++p->pos;
    while (p->pos < end && *p->pos == 0xFF) { n += 8; ++p->pos; }
    if (p->pos == end) return 0;
    p->free = 7;
    while (((*p->pos >> p->free) & 1)) { ++n; --p->free; }
    if (n > DECINF_EXPSIZE) return 0;
  }
  else { // n < 5
    n = 1;
    p->free = 3;
    while (((*p->pos >> p->free) & 1)) { ++n; --p->free; }
  }
  return n;
}

/**
 * \brief Returns the number of bits of the encoded exponent.
 *
 * \see decInfiniteReadUnaryPrefix1()
 */
static uCount decInfiniteReadUnaryPrefix0(bitPos* p, uByte const* end) {
  uCount n;
  // Check if the last 5 bits of the first byte are all zeroes
  if (!(*p->pos & 0x1F)) {
    n = 5;
    ++p->pos;
    while (p->pos < end && !(*p->pos)) { n += 8; ++p->pos; }
    if (p->pos == end) return 0;
    p->free = 7;
    while (!((*p->pos >> p->free) & 1)) { ++n; --p->free; }
    if (n > DECINF_EXPSIZE) return 0;
  }
  else { // n < 5
    n = 1;
    p->free = 3;
    while (!((*p->pos >> p->free) & 1)) { ++n; --p->free; }
  }
  return n;
}

/**
 * \brief Packs the \a n least significant bits from a byte into a byte stream.
 *
 * \param p The current position in the byte stream
 * \param bits The bits to pack
 * \param n The number of bits in \a bits to pack (in the range [1,8])
 *
 * \return Updated \a p
 */
static bitPos pack(bitPos p, uByte bits, uCount n) {
  assert(n <= 8 && n > 0);
  assert(p.free <= 8);

  if (p.free == 0) {
    ++p.pos;
    *p.pos = 0;
    p.free = 8;
  }

  uByte mask = MASK[p.free];

  if (p.free < n) {
    n -= p.free;
    *p.pos |= (bits >> n) & mask;
    ++p.pos;
    p.free = 8 - n;
    *p.pos = 0;
    *p.pos |= (bits << p.free);
  }
  else {
    p.free -= n;
    *p.pos |= (bits << p.free) & mask;
  }

  assert(p.free < 8);

  return p;
}

/**
 * \brief Unpacks \a n bits from a byte stream into the lower bits of the
 *        output.
 *
 * \param p The current position in the byte stream
 * \param bits The byte to receive the unpacked bits
 * \param n The number of bits to unpack into \a bits (in the range [1,8])
 *
 * \return Updated \a p
 */
static bitPos unpack(bitPos p, uByte* bits, uCount n) {
  assert(n > 0 && n <= 8);
  assert(p.free <= 8);

  if (p.free == 0) {
    ++p.pos;
    p.free = 8;
  }

  uByte mask = MASK[n];

  if (p.free < n) {
    n -= p.free;
    *bits = (*p.pos << n) & mask;
    ++p.pos;
    *bits |= *p.pos >> (8 - n);
    p.free = 8 - n;
  }
  else { // n <= p.free
    p.free -= n;
    *bits = (*p.pos >> p.free) & mask;
  }

  assert(p.free < 8);

  return p;
}

/**
 * \brief Look-up table for bitwise-complemented encoded tiny exponents.
 *
 * A tiny exponent has absolute value in the range [0,5] and will fit in the
 * first byte of the encoding.
 */
static uByte const TINY_C_EXP[6] = { 12,  8,  7,  6,  5, 4 };

/**
 * \brief Look-up table for non-complemented encoded tiny exponents.
 *
 * A tiny exponent has absolute value in the range [0,5] and will fit in the
 * first byte of the encoding.
 */
static uByte const  TINY_EXP[6] = { 16, 20, 24, 25, 26, 27 };

/**
 * \brief Free-bit look-up table numbers for tiny exponents.
 *
 * Each entry represents the number of free bits in the current byte after
 * packing a tiny expoennt.
 */
static uCount const TINY_FREE[6] = { 2, 2, 0, 0, 0, 0 };

/**
 * \brief Look-up table for bitwise-complemented encoded small exponents.
 *
 * A small exponent has absolute value in the range [6,125] and will fit in the
 * first two bytes of the encoding.
 *
 * \note The first six entries of this look-up table are not used.
 */
static uShort const SMALL_C_EXP[126] = {
  0,    0,    0,    0,    0,    0,
  960,  896,  832,  768,  704,  640,  576,  512,  496,  480,  464,  448,
  432,  416,  400,  384,  368,  352,  336,  320,  304,  288,  272,  256,
  252,  248,  244,  240,  236,  232,  228,  224,  220,  216,  212,  208,
  204,  200,  196,  192,  188,  184,  180,  176,  172,  168,  164,  160,
  156,  152,  148,  144,  140,  136,  132,  128,  127,  126,  125,  124,
  123,  122,  121,  120,  119,  118,  117,  116,  115,  114,  113,  112,
  111,  110,  109,  108,  107,  106,  105,  104,  103,  102,  101,  100,
  99,   98,   97,   96,   95,   94,   93,   92,   91,   90,   89,   88,
  87,   86,   85,   84,   83,   82,   81,   80,   79,   78,   77,   76,
  75,   74,   73,   72,   71,   70,   69,   68,   67,   66,   65,   64,
};

/**
 * \brief Look-up table for non-complemented encoded small exponents.
 *
 * A small exponent has absolute value in the range [6,125] and will fit in the
 * first two bytes of the encoding.
 *
 * \note The first six entries of this look-up table are not used.
 */
static uShort const SMALL_EXP[126] = {
  0,    0,    0,    0,    0,    0,
  7168, 7232, 7296, 7360, 7424, 7488, 7552, 7616, 7680, 7696, 7712, 7728,
  7744, 7760, 7776, 7792, 7808, 7824, 7840, 7856, 7872, 7888, 7904, 7920,
  7936, 7940, 7944, 7948, 7952, 7956, 7960, 7964, 7968, 7972, 7976, 7980,
  7984, 7988, 7992, 7996, 8000, 8004, 8008, 8012, 8016, 8020, 8024, 8028,
  8032, 8036, 8040, 8044, 8048, 8052, 8056, 8060, 8064, 8065, 8066, 8067,
  8068, 8069, 8070, 8071, 8072, 8073, 8074, 8075, 8076, 8077, 8078, 8079,
  8080, 8081, 8082, 8083, 8084, 8085, 8086, 8087, 8088, 8089, 8090, 8091,
  8092, 8093, 8094, 8095, 8096, 8097, 8098, 8099, 8100, 8101, 8102, 8103,
  8104, 8105, 8106, 8107, 8108, 8109, 8110, 8111, 8112, 8113, 8114, 8115,
  8116, 8117, 8118, 8119, 8120, 8121, 8122, 8123, 8124, 8125, 8126, 8127,
};

/**
 * \brief Free-bit look-up table for small exponents.
 *
 * Each entry represents the number of free bits in the current byte after
 * packing a small expoennt.
 */
static uCount const SMALL_FREE[126] = {
  0, 0, 0, 0, 0, 0, // Unused
  6, 6, 6, 6, 6, 6, 6, 6, 4, 4, 4, 4,
  4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

/**
 * \brief Packs an exponent into an output byte stream.
 *
 * \param exp The absolute value of an (adjusted) exponent (<=999,999,999].
 * \param T The bit encoding the exponent sign, as per decInfinite's paper
 * \param p The current position in the target byte stream
 *
 * \return Updated \a p
 */
static bitPos decInfinitePackExponent(uInt exp, Flag T, bitPos p) {
  assert(p.free == 5);
  assert(exp <= 999999999);

  if (exp < 6) { // Tiny exponent, fits in the first byte
    if (T)
      *p.pos |= TINY_EXP[exp];
    else
      *p.pos |= TINY_C_EXP[exp];
    p.free = TINY_FREE[exp];

    return p;
  }

  if (exp < 126) { // && exp >= 6 (Small exponent, fits in two bytes)
    dUnit u;
    if (T)
      u = (dUnit)SMALL_EXP[exp];
    else
      u = (dUnit)SMALL_C_EXP[exp];
    *p.pos |= DECLET_HI(u);
    ++p.pos;
    *p.pos = DECLET_LO(u);
    p.free = SMALL_FREE[exp];
    return p;
  }

  // If we get here, exp >= 126 (the offset exponent is on at least 8 bits)
  exp += 2; // Offset the exponent
  uCount n = ilog2(exp); // One less than the number of bits of exp
  assert(n >= 7 && n <= DECINF_EXPSIZE);

  uByte pad1 = 0xff;
  uByte pad2 = 0xfe;
  if (!T) {
    pad1 = ~pad1;
    pad2 = ~pad2;
    exp  = ~exp;
  }

  if (n < 8) {
    return pack(pack(p, pad2, n + 1), (uByte)exp, n); // exp stripped of the msb
  }
  else { // Large exponent
    // Encode unary prefix
    uByte n2 = n;
    do {
      p = pack(p, pad1, 8);
      n2 -= 8;
    } while (n2 >= 8);
    p = pack(p, pad2, n2 + 1);
    // Pack the exponent
    if (n == 8) {
      p = pack(p, (uByte)exp, n);
    }
    else if (n <= 16) {
      p = pack(p, (uByte)(exp >> 8), n - 8);
      p = pack(p, (uByte)exp, 8);
    }
    else if (n <= 24) {
      p = pack(p, (uByte)(exp >> 16), n - 16);
      p = pack(p, (uByte)(exp >> 8), 8);
      p = pack(p, (uByte)exp, 8);
    }
    else { // n <= DECINF_EXPSIZE
      p = pack(p, (uByte)(exp >> 24), n - 24);
      p = pack(p, (uByte)(exp >> 16), 8);
      p = pack(p, (uByte)(exp >> 8), 8);
      p = pack(p, (uByte)exp, 8);
    }
    return p;
  }
}

/**
 * \brief Unpacks an encoded exponent.
 *
 * \param decnum The target decNumber
 * \param p The current position in the byte stream
 * \param end A pointer one past the end of the byte stream
 *
 * \return Updated \a p
 */
static bitPos decInfiniteUnpackExponent(decNumber* decnum, bitPos p, uByte const* end) {
  uByte T = *p.pos & 0x10; // Extract the encoding of the exponent sign
  uByte esign = (T << 3) ^ (*p.pos & 0x80); // Exponent sign (0=positive,>0=negative)

  uCount n;
  if (T)
    n = decInfiniteReadUnaryPrefix1(&p, end); // Sets d->free
  else
    n = decInfiniteReadUnaryPrefix0(&p, end); // Sets d->free

  if (!n) { p.pos = 0; return p; } // Too large an exponent
  // Check that there are enough bits left to read the exponent
  if (((end - p.pos) << 3) - (8 - p.free) < n) { p.pos = 0; return p; }

  uByte byte;
  uInt e; // To hold the absolute value of the exponent

  if (n <= 8) {
    p = unpack(p, &byte, n);
    e = byte;
  }
  else if (n <= 16) {
    p = unpack(p, &byte, n - 8);
    e = byte;
    p = unpack(p, &byte, 8);
    e = (e << 8) | byte;
  }
  else if (n <= 24) {
    p = unpack(p, &byte, n - 16);
    e = byte;
    p = unpack(p, &byte, 8);
    e = (e << 8) | byte;
    p = unpack(p, &byte, 8);
    e = (e << 8) | byte;
  }
  else { // n <= DECINF_EXPSIZE
    p = unpack(p, &byte, n - 24);
    e = byte;
    p = unpack(p, &byte, 8);
    e = (e << 8) | byte;
    p = unpack(p, &byte, 8);
    e = (e << 8) | byte;
    p = unpack(p, &byte, 8);
    e = (e << 8) | byte;
  }

  if (!T) e = ~e & MASK[n]; // Bitwise-complement exponent
  e |= (1 << n); // Restore msb
  e -= 2;        // Cancel offset
  assert(e < INT32_MAX); // Detecting too long unary prefixes prevent this to be violated
  if (e > 999999999) { p.pos = 0; return p; } // But the exponent might still be too large
  decnum->exponent = (Int)e;
  if (esign) decnum->exponent = -decnum->exponent;

  return p;
}

/**
 * \brief Computes the ten's complement of the mantissa of a decNumber.
 *
 * \param decnum A decNumber
 *
 * \return The number with the mantissa complemented
 */
static decNumber* decInfiniteComplementMantissa(decNumber* decnum) {
  Unit const* rp = decnum->lsu + D2U(decnum->digits) - 1;
  Unit* lp = decnum->lsu;

  for (; lp <= rp; ++lp)
    *lp = 999 - *lp;

  for (lp = decnum->lsu; *lp + 1 > DECDPUNMAX; ++lp)
    *lp = 0;
  ++*lp;

  return decnum;
}

/**
 * \brief Encodes the significand.
 *
 * \param decnum The decNumber to be encoded
 * \param p The current position if the target byte stream
 *
 * \return Updated \a p
 */
static bitPos decInfinitePackMantissa(decNumber const* decnum, bitPos p) {
  Unit const* lp = decnum->lsu;
  Unit const* rp = decnum->lsu + D2U(decnum->digits);

  // Skip units of trailings zeroes (not all units can be all zeroes, unless
  // something is wrong with the input: to be on the safe side we add a bound
  // check anyway):
  while (lp < rp && *lp == 0) ++lp;

  dUnit out;
  while (rp > lp) {
    --rp;

    if (p.free == 0) {
      ++p.pos;
      *p.pos = 0;
      p.free = 8;
    }

    out = (dUnit)(*rp);
    out.declet &= 0x03FF; // Not needed
    p.free -= 2;
    out.declet <<= p.free;
    *p.pos |= DECLET_HI(out);
    ++p.pos;
    *p.pos = DECLET_LO(out);
  }
  return p;
}

/**
 * \brief Unpacks declets from a Decimal Infinite stream.
 *
 * This function interprets the byte stream starting at \a bitPos until \a end
 * as a sequence of declets to decode into the given decNumber. It also
 * checks for decoding errors and returns `0` in case of error. It also assumes
 * that the target decNumber has enough allocated space to hold the decoded
 * mantissa (at most #DECINF_MAXSIZE bytes).
 *
 * \param decnum The target decNumber
 * \param p The current position in the byte stream
 * \param end A pointer one past the end of the byte stream
 *
 * \return The number of decoded declets; returns `0` in case of error
 *         (mantissa too large or wrongly encoded, e.g., with declets encoding
 *         numbers greater than 999).
 */
static size_t decInfiniteUnpackMantissa(decNumber* decnum, bitPos p, uByte const* end) {
  dUnit out;

  // Number of declets to decode
  size_t n = (8 * (end - p.pos) - (8 - p.free)) / 10;

  if (n == 1) { // Single unit case
    decnum->digits = 3;

    if (p.free == 0) {
      ++p.pos;
      p.free = 8;
    }

    DECLET_HI(out) = *p.pos & MASK[p.free];
    ++p.pos;
    DECLET_LO(out) = *p.pos;
    p.free -= 2;
    out.declet >>= p.free;

    if (out.declet == 0 || out.declet > 999) return 0; // Invalid unit

    if (decNumberIsNegative(decnum))
      out.declet = 1000 - out.declet; // Ten's complement

    *decnum->lsu = out.declet;

    return n;
  }

  // If we get here, n > 1
  decnum->digits = 3 * n;
  if (decnum->digits > DECNUMDIGITS) return 0; // Mantissa too long

  Unit* rp = decnum->lsu + n;

  // Unpack declets
  for (size_t i = 0; i < n; ++i) {
    --rp;

    if (p.free == 0) {
      ++p.pos;
      p.free = 8;
    }

    DECLET_HI(out) = *p.pos & MASK[p.free];
    ++p.pos;
    DECLET_LO(out) = *p.pos;
    p.free -= 2;
    out.declet >>= p.free;
    if (out.declet > 999) return 0; // Invalid unit
    *rp = out.declet;
  }

  if (*rp == 0) return 0; // The least significant unit cannot be zero
  if (decNumberIsNegative(decnum)) {
    if (*(rp + n - 1) > 899) return 0; // Out of range
    decInfiniteComplementMantissa(decnum);
  }
  else {
    if (*(rp + n - 1) == 0) return 0; // The most significant unit cannot be zero
  }

  return n;
}

/**
 * \brief Shifts the digits in the mantissa of a decNumber towards the most
 *        significant unit.
 *
 * Aligns the mantissa of a finite, non-zero, decNumber so that the most
 * significant unit contains three digits, padding the least significant unit
 * with zeros as necessary. As a result, the number of digits in `decnum`
 * becomes a multiple of three. The number of digits is updated accordingly.
 * The exponent is left unchanged.
 *
 * \param decnum A decNumber
 *
 * \return Nothing.
 */
static void decShiftToMost(decNumber* decnum) {
  Unit* rp = decnum->lsu + D2U(decnum->digits) - 1; // msu

  // According to decNumber's manual, p. 31, the msu contains from 1 through
  // DECDPUN digits, and must not be 0 unless the number is zero.
  // So, we may safely assume that *rp > 0 here, and we need to shift at most
  // by two digits.
  assert(*rp > 0);

  if (*rp > 99) return; // Nothing to do

  if (*rp > 9) { // Shift by one
    ++decnum->digits;
    *rp = X10(*rp);
    while (rp > decnum->lsu) {
      Unit* lp = rp - 1;
      Unit quot = QUOT10(*lp, 2); // Extract first digit of next unit
      Unit rem = *lp - X100(quot);
      *rp += quot;
      *lp = X10(rem);
      --rp;
    }
  }
  else { // Shift by two
    decnum->digits += 2;
    *rp = X100(*rp);
    while (rp > decnum->lsu) { // More than one unit
      Unit* lp = rp - 1;
      Unit quot = QUOT10(*lp, 1); // Extract first two digits
      Unit rem = *lp - X10(quot);
      *rp += quot;
      *lp = X100(rem);
      --rp;
    }
  }
  assert(decnum->digits % 3 == 0);
  return;
}


#pragma mark Public interface

// Conversions

size_t decInfiniteFromNumber(size_t len, uByte result[len], decNumber* decnum) {
  // Treat special cases (zero, infinites and NaNs) first.
  if (decNumberIsSpecial(decnum)) {
    if (decNumberIsInfinite(decnum))
      result[0] = decNumberIsNegative(decnum) ? 0x00 : 0xC0;
    else // NaN
      result[0] = 0xE0;
    return 1;
  }

  if (finiteDecNumberIsZero(decnum)) {
    result[0] = decNumberIsNegative(decnum) ? 0x40 : 0x80;
    return 1;
  }

  // If we get here, decnum is a non-zero finite number.
  // Its encoding will occupy at least two bytes.

  // Compute the adjusted exponent, i.e., the exponent of the number when
  // represented with a mantissa in [0,10).
  Int adj_exp = decnum->exponent + decnum->digits - 1;

  // As per decNumber's requirements
  assert(adj_exp >= -999999999);
  assert(adj_exp <= 999999999);

  bitPos p = { .pos = &result[0], .free = 5 };

  if (decNumberIsNegative(decnum)) {
    *p.pos = 0x00; // Set negative sign and padding bit
    Flag T = adj_exp < 0;
    p = decInfinitePackExponent(T ? -adj_exp : adj_exp, T, p);
    decShiftToMost(decnum); // Align digits to msu (leaves exponent intact)
    decInfiniteComplementMantissa(decnum);
    p = decInfinitePackMantissa(decnum, p);
  }
  else {
    *p.pos = 0x80; // Set positive sign and padding bit
    Flag T = adj_exp >= 0;
    p = decInfinitePackExponent(T ? adj_exp : -adj_exp, T, p);
    decShiftToMost(decnum); // Align digits to msu (leaves exponent intact)
    p = decInfinitePackMantissa(decnum, p);
  }

  return (p.pos - &result[0] + 1);
}

decNumber* decInfiniteToNumber(size_t len, uint8_t const bytes[len], decNumber* decnum) {
  assert(len > 0);

  if (len == 1) { // Assume zero or special number
    decNumberZero(decnum);
    switch (bytes[0]) {
      case 0x80:                        // Zero
        break;
      case 0x40:
        decnum->bits = DECNEG;          // Minus zero
        break;
      case 0x00:
        decnum->bits = DECNEG | DECINF; // -Inf
        break;
      case 0xC0:
        decnum->bits = DECINF;          // +Inf
        break;
      case 0xE0:
        decnum->bits = DECNAN;          // Simple (not signaling) NaN
        break;
      default:
        return 0;                       // Error
    }
    return decnum;
  }

  // Check that the sign is encoded correctly (00 or 10) and that the pad bit is zero
  switch (bytes[0] & 0xE0) {
    case 0x00:
      decnum->bits = DECNEG;
      break;
    case 0x80:
      decnum->bits = 0;
      break;
    default:
      return 0;
  }

  // Check that zero is not encoded as a negative exponent
  switch (bytes[0] & 0xFC) {
    case 0x8C: // 10 0 011 is invalid
    case 0x10: // 00 0 100 is invalid
      return 0;
    default:
      break;
  }

  bitPos p = { .pos = (uByte*)&bytes[0], .free = 5 };
  uByte const* end = bytes + len;

  // Decode the adjusted exponent
  p = decInfiniteUnpackExponent(decnum, p, end);
  if (!p.pos) return 0; // Too large an exponent

  // Decode the significand
  if (decInfiniteUnpackMantissa(decnum, p, end)) {
    // Set unadjusted exponent
    decnum->exponent = decnum->exponent - decnum->digits + 1;
  }
  else
    return 0; // Invalid mantissa

  return decnum;
}

