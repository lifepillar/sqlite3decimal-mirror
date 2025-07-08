**WARNING: this library comes with no warranty whatsoever. Use at your own risk!**

# SQLite3 Decimal

*SQLite3 Decimal* is an extension for SQLite implementing exact decimal
arithmetic (within a bounded range). Numbers can be stored as strings or as
16-byte blobs conforming to
[IEEE 754](https://en.wikipedia.org/wiki/IEEE_754)
*decimal128* format. **Note** that this representation is different from the
one of
[SQLite's ieee754.c extension](https://sqlite.org/floatingpoint.html),
which deals with a *binary* format encoding, not decimal. This extension is
comparable to
[SQLite's decimal.c extension](https://sqlite.org/floatingpoint.html),
but it provides more operations and uses a different internal representation
for calculations.

The library provides 34 digits of decimal precision and an exponent range in
[-6143,6144]. See also:

- [How much precision and range is needed for decimal arithmetic?](https://speleotrove.com/decimal/decifaq1.html#needed)
- [The Arithmetic Model](https://speleotrove.com/decimal/damodel.html)

This is a quick comparison between this extension and
[SQLite's decimal.c](https://sqlite.org/floatingpoint.html)
to help you decide which library best suits your needs (but note that the two
libraries can be used together!):

| Feature                          | This library                    | SQLite's decimal.c                                        |
|----------------------------------|---------------------------------|-----------------------------------------------------------|
| Storage                          | 16-byte blobs or text format    | Text format only                                          |
| Time performance                 | More efficient                  | Less efficient                                            |
| Operations                       | Almost all IEEE 754 operations  | Few operations                                            |
| Precision                        | 34 decimal digits               | Arbitrary                                                 |
| Rounding                         | Possible (and detectable)       | Only exact operations are implemented (e.g., no division) |
| Special numbers (±Inf, NaNs, -0) | Supported                       | Not supported                                             |
| Trailing zeroes                  | Follows IEEE 754 standard rules | Custom rules                                              |
| Collating                        | Not possible for blobs          | Decimal collate                                           |

The difference about trailing zeroes means that 1.00 x 1.000 is 1.00000
according to this library, but it's 1.00 according to SQLite's decimal.c.

Lack of collating sequences for blobs (a limitation of SQLite) means that it is
not possible in general to compare numbers stored as blobs with operators such
as `=` or `>`: you have to use functions such as `decEq()` and `decGt()`
instead, or normalize the numbers with `decReduce()` (for equality and
inequality only). For numbers stored as text you can use SQLite's decimal
collation.

Although SQLite's decimal.c only implements exact operations on decimals,
SQLite automatically casts a numerical string into a binary float if necessary.
So, operations such as divisions can be performed by falling back to the usual
binary floating point arithmetic (which kind of defeats the purpose of having
a decimal arithmetic library, though).


## Quick Start

    ./configure
    make
    sqlite3 --cmd '.load ./libdecimal'

To build a static library instead of a dynamic library, use:

    make clean && ./configure --static && make


## Sample Session

### General Information

    sqlite> select decVersion();
    Decimal v0.1.3 (decNumber 3.68-p3)

    sqlite> select * from decContext;
    prec  emax  emin   round
    ----  ----  -----  ---------------
    34    6144  -6143  ROUND_HALF_EVEN

### Creating decimals

Decimals can be created from strings, integers, or other decimals. The internal
representation is based on IEEE 754 decimal128 format.

    sqlite> select decBytes(dec('1.89'));
    22078000 00000000 00000000 000000cf

    sqlite> select decStr(dec('1.89'));
    1.89

    sqlite> select decBytes(dec(42));
    22080000 00000000 00000000 00000042

    sqlite> select decStr(dec(42));
    42

**Note:** internally, SQLite3Decimal always manipulates numbers in IEEE's
decimal128 format regardless of the input format. So, the output of
a calculation is always a decimal128-formatted blob. To translate that into
a string you have to use `decStr()`.

### Mathematical operations

    sqlite> select decStr(decMul(decAdd('1.23', '45.0967', '-678.00000000000000001'), '-0.7891'));
    498.453401030000000007891

    sqlite> select decStr(decDivInt(9,4));
    2

    sqlite> select decStr(decSqrt(16.000));
    4.00

    sqlite> select decStr(decExp(-1)); -- e⁻¹
    0.3678794411714423215955237701614609

    sqlite> select decStr(decQuantize(decExp(-1), '.00')); -- Two fractional digits, please
    0.37

### Rounding

    sqlite> select decStr(decDiv(1,7));
    0.1428571428571428571428571428571429

    sqlite> select * from decStatus; -- Was the result rounded?
    Inexact result

Clear the status flags:

    sqlite> delete from decStatus;

The rounding method can be adjusted at runtime by updating the context:

    sqlite> select decStr(decDiv(1,3));
    0.3333333333333333333333333333333333

    sqlite> update decContext set round = 'ROUND_UP';
    sqlite> select decStr(decDiv(1,3));
    0.3333333333333333333333333333333334

### Decimals stored as blobs

    sqlite> create table T(n blob);
    sqlite> insert into T values (dec('.843')), (dec('3427.19')), (dec('-28383.89'));
    sqlite> select decStr(n) from T;
    0.843
    3427.19
    -28383.89

    sqlite> select decStr(n) from T where decLt(n, 10); -- Numbers less than 10
    0.843
    -28383.89

    sqlite> select decStr(decMin(n)), decStr(decMax(n)), decStr(decSum(n)), decStr(decAvg(n)) from T;
    -28383.89|3427.19|-24955.857|-8318.619

    sqlite> select hex(n) from T;
    4D010000000000000000000000400722
    990B0700000000000000000000800722
    CFF924000000000000000000008007A2

    sqlite> select decBytes(n) from T;
    22074000 00000000 00000000 0000014d
    22078000 00000000 00000000 00070b99
    a2078000 00000000 00000000 0024f9cf

Note that blobs are *stored* in little-endian order, but `decBytes()`
*displays* the numbers in big-endian order (so, the first bit—that is, leftmost
bit—is the sign).


### Decimals stored as strings

Similarly to SQLite's own
[decimal.c extension](https://sqlite.org/floatingpoint.html),
numbers stored as text can also be processed:

    sqlite> create table U(n text);
    sqlite> insert into U values ('0.123'), ('-34.20');
    sqlite> select n from U;
     0.123
    -34.20

    select decStr(decMul(X.n, Y.n)) from U X, U Y where decLt(X.n, Y.n);
    -4.20660

(Almost) equivalent formulation with SQLite's decimal.c extension:

    sqlite> select decimal_mul(X.n, Y.n) from U X, U Y where decimal_cmp(X.n, Y.n) < 0;
    -4.2066

When numbers are stored as text, you can even mix the two extensions if you are
careful enough to convert SQLite3Decimal results back to strings:

    sqlite> select decimal_add(decStr(decMul(X.n, Y.n)), '1.5') from U X, U Y where decimal_cmp(X.n, Y.n) < 0;
    -2.70660


### Trapping error conditions

The extension can be configured at runtime to raise or not to raise errors upon
certain conditions. The conditions that should raise an error are found inside
the `decTraps` virtual table:

    sqlite> select flag from decTraps;
    Conversion syntax
    Division by zero
    Division impossible
    Division undefined
    Out of memory
    Invalid context
    Invalid operation

    sqlite> insert into decTraps values ('Inexact result');
    sqlite> select decStr(decDiv(1,7));
    Runtime error: Inexact

    sqlite> select dec('1..2');
    Runtime error: Conversion syntax
    sqlite> delete from decTraps where flag = 'Conversion syntax';
    sqlite> select decStr(dec('1..2'));
    NaN


## Build the code documentation

Install [Doxygen](https://www.doxygen.nl) and [Graphviz](https://graphviz.org/), then:

    make doc

