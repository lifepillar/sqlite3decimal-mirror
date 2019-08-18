/**
 * \file      mu_unit.h
 * \author    Lifepillar
 * \copyright Copyright (c) 2017–2019 Lifepillar.
 *            This program is free software; you can redistribute it and/or
 *            modify it under the terms of the Simplified BSD License (also
 *            known as the "2-Clause License" or "FreeBSD License".)
 * \copyright This program is distributed in the hope that it will be useful,
 *            but without any warranty; without even the implied warranty of
 *            merchantability or fitness for a particular purpose.
 * \date      Created on 2017-10-11
 *
 * \brief A micro unit testing framework
 */
#ifndef mu_unit_h
#define mu_unit_h

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#pragma mark Helper macros

#ifndef MU_OUT
  /**
   * \brief Where MU Unit's output should go.
   *
   * For example, to send the output to `stderr`:
   *
   *     #define MU_OUT stderr
   *     #include "mu_unit.h"
   */
#define MU_OUT stdout
#endif

  /**
   * \brief Prints a message (using `printf()`'s syntax).
   */
#define MU_MSG(...) do { fprintf(MU_OUT, __VA_ARGS__); } while (0)

  /**
   * \brief Returns a value that can be interpreted as a pointer value.
   *
   * That is, any pointer will be returned as such, but other arithmetic
   * values will result in a `0`.
   *
   * \param X A value of any type
   *
   * \see http://icube-icps.unistra.fr/index.php/File:ModernC.pdf
   */
#define MU_FORCE_POINTER(X)   \
  _Generic((X),               \
      char:               0,  \
      signed char:        0,  \
      unsigned char:      0,  \
      signed short:       0,  \
      unsigned short:     0,  \
      signed:             0,  \
      unsigned:           0,  \
      signed long:        0,  \
      unsigned long:      0,  \
      signed long long:   0,  \
      unsigned long long: 0,  \
      float:              0,  \
      double:             0,  \
      long double:        0,  \
      default:            (X) \
      )

  /**
   * \brief Returns a `void const*` if \a X is a pointer, otherwise returns the
   *        value itself.
   *
   * \param X A value of any type
   */
#define MU_CONVERT(X)                                                \
  _Generic((X),                                                      \
      char:               (X),                                       \
      signed char:        (X),                                       \
      unsigned char:      (X),                                       \
      signed short:       (X),                                       \
      unsigned short:     (X),                                       \
      signed:             (X),                                       \
      unsigned:           (X),                                       \
      signed long:        (X),                                       \
      unsigned long:      (X),                                       \
      signed long long:   (X),                                       \
      unsigned long long: (X),                                       \
      float:              (X),                                       \
      double:             (X),                                       \
      long double:        (X),                                       \
      default:            ((void const*){ 0 } = MU_FORCE_POINTER(X)) \
      )

  /**
   * \brief Returns a format that is suitable for `fprintf()` (or for
   *        #MU_MSG()).
   *
   * \param M A string literal
   * \param X A value of any type
   */
#define MU_FORMAT(M, X)                  \
  _Generic((X),                          \
      char:               "" M "%c",     \
      signed char:        "" M "%hhd",   \
      unsigned char:      "" M "0x%02X", \
      signed short:       "" M "%hd",    \
      unsigned short:     "" M "%hu",    \
      signed:             "" M "%d",     \
      unsigned:           "" M "%u",     \
      signed long:        "" M "%ld",    \
      unsigned long:      "" M "%lu",    \
      signed long long:   "" M "%lld",   \
      unsigned long long: "" M "%llu",   \
      float:              "" M "%.8f",   \
      double:             "" M "%.12f",  \
      long double:        "" M "%.20Lf", \
      char*:              "" M "%s",     \
      char const*:        "" M "%s",     \
      default:            "" M "%p"      \
      )

  /**
   * \brief Prints a value without having to specify a format.
   *
   * \param M A string literal: this text is prepended to the printed value
   * \param X A value of any type
   */
#define MU_PRINT_VALUE(M, X)                \
  do {                                      \
    MU_MSG(MU_FORMAT(M, X), MU_CONVERT(X)); \
  } while (0)


#pragma mark Assertions

  /**
   * \brief Skips a test under certain conditions.
   *
   * If \a cond is non-zero then the test is interrupted. This does not result
   * in a failure, but \a message is printed to inform the user that the test
   * has not been run.
   *
   * \param cond The condition to be verified for a test to be skipped
   * \param message The message to be printed when a test is skipped
   */
#define mu_skip_if(cond, message)    \
  do {                               \
    if (!mu_test_status && (cond)) { \
      MU_MSG(" » %s\n", message);    \
      mu_test_status = 2;            \
    }                                \
  } while (0)

  /**
   * \brief Succeeds when \a test returns a non-zero value.
   *
   * If \a test returns `0`, then the test fails and \a message is printed.
   *
   * \param test An expression to be tested
   * \param message A message to be printed when the test fails.
   *
   * \return Nothing
   */
#define mu_assert(test, message)                            \
  do {                                                      \
    ++mu_assert_num;                                        \
    if (!mu_test_status && !(test)) {                       \
      MU_MSG("\n\n  ASSERTION %d FAILED\n", mu_assert_num); \
      MU_MSG("  %s\n\n", message);                          \
      mu_test_status = 1;                                   \
    }                                                       \
  } while (0)

  /**
   * \brief Compares two values of arbitrary types.
   *
   * For example:
   *
   *     mu_assert_cmp(3, !=, 7); // Succeeds
   *     mu_assert_cmp(3, >=, 4); // Fails
   *
   * \param expected The expected value
   * \param cmp A comparison operator, one among `==`, `!=`, `<`, `>`, `<=` or
   *            `>=`.
   * \param value The expression to be compared.
   *
   * \return Nothing
   */
#define mu_assert_cmp(expected, cmp, value)                 \
  do {                                                      \
    ++mu_assert_num;                                        \
    if (!mu_test_status && !((expected) cmp (value))) {     \
      MU_MSG("\n\n  ASSERTION %d FAILED\n", mu_assert_num); \
      MU_PRINT_VALUE("  Expected  ", expected);             \
      if (strncmp("==", #cmp, 2) == 0) {                    \
        MU_PRINT_VALUE("\n   but got  ", value);            \
      }                                                     \
      else {                                                \
        MU_PRINT_VALUE("\n     to be  " #cmp, value);       \
      }                                                     \
      MU_MSG("\n\n");                                       \
      mu_test_status = 1;                                   \
    }                                                       \
  } while (0)

  /**
   * \brief Succeeds when \a expected equals \a value.
   *
   * \note Do not use this to compare strings: use #mu_assert_streq()
   *       instead.
   *
   * \see #mu_assert_cmp().
   */
#define mu_assert_eq(expected, value) mu_assert_cmp(expected, ==, value)

  /**
   * \brief Succeeds when \a expected is not equal to \a value.
   * \see #mu_assert_cmp().
   */
#define mu_assert_ne(expected, value) mu_assert_cmp(expected, !=, value)

  /**
   * \brief Succeeds when \a expected is greater than \a value.
   * \see #mu_assert_cmp().
   */
#define mu_assert_gt(expected, value) mu_assert_cmp(expected, >, value)

  /**
   * \brief Succeeds when \a expected is greater than or equal to \a value.
   * \see #mu_assert_cmp().
   */

#define mu_assert_ge(expected, value) mu_assert_cmp(expected, >=, value)
  /**
   * \brief Succeeds when \a expected is less than \a value.
   * \see #mu_assert_cmp().
   */
#define mu_assert_lt(expected, value) mu_assert_cmp(expected, <, value)

  /**
   * \brief Succeeds when \a expected is less than or equal to \a value.
   * \see #mu_assert_cmp().
   */
#define mu_assert_le(expected, value) mu_assert_cmp(expected, <=, value)

  /**
   * \brief Succeeds when \a value is a `NULL` pointer.
   */
#define mu_assert_null(value) mu_assert_eq((void*)0, value)

  /**
   * \brief Succeeds when \a value is not a `NULL` pointer.
   */
#define mu_assert_not_null(value) mu_assert_ne((void*)0, value)

  /**
   * \brief Compares two null-terminated strings using `strcmp()`.
   *
   * Succeeds when \a expected is equal to \a value.
   *
   * \param expected The expected string
   * \param value The actual string
   *
   * \return Nothing
   */
#define mu_assert_streq(expected, value)                    \
  do {                                                      \
    ++mu_assert_num;                                        \
    if (!mu_test_status && strcmp(expected, value) != 0) {  \
      MU_MSG("\n\n  ASSERTION %d FAILED\n", mu_assert_num); \
      MU_PRINT_VALUE("  Expected  ", expected);             \
      MU_PRINT_VALUE("\n   but got  ", value);              \
      MU_MSG("\n\n");                                       \
      mu_test_status = 1;                                   \
    }                                                       \
  } while (0)


#pragma mark Runner


  typedef void mu_func(void);  /**< \brief The type of mu_setup() and mu_teardown() functions. */
  static int mu_test_status;   /**< \brief The current test's status: `0` (ok), '1` (failed) or `2` (skipped) */
  static int mu_tests_run;     /**< \brief The total number of tests run. */
  static int mu_tests_failed;  /**< \brief The total number of failed tests. */
  static int mu_tests_skipped; /**< \brief The total number of skipped tests. */
  static int mu_assert_num;    /**< \brief Counter of assertions within a test. */
  static clock_t start_time;   /**< \brief Time when the tests are started. */
  static double time_elapsed;  /**< \brief Time spent running all the tests. */

  /**
   * \brief Trivial function that does nothing.
   *
   * This may be used to define mu_setup() and/or mu_teardown() as no-ops
   * (which is the default).
   */
  static void mu_noop(void) { }

  mu_func* mu_setup = mu_noop;    /**< \brief Function executed before each test. */
  mu_func* mu_teardown = mu_noop; /**< \brief Function executed after each test. */

  /**
   * \brief Registers a function as a test function.
   *
   * Each test is a function `f` with signature `void f(void)`. Every such
   * function must be registered using this macro. For example:
   *
   *     #include "mu_unit.h"
   *
   *     // A simple test
   *     static int will_9_by_9_make_81(void) {
   *       mu_assert_eq(81, 9 * 9);
   *     }
   *
   *     static void my_test_suite(void) {
   *       mu_test(will_9_by_9_make_81);      // Register the test
   *     }
   *
   *     int main(void) {
   *       mu_init();
   *       mu_run_test_suite(my_test_suite);  // Run baby run
   *       mu_test_summary();
   *       return (mu_tests_failed > 0);
   *     }
   *
   * \param test The name of the function.
   */
#define mu_test(test)               \
  do {                              \
    mu_test_status = 0;             \
    mu_assert_num = 0;              \
    MU_MSG("%s ", #test);           \
    (*mu_setup)();                  \
    test();                         \
    (*mu_teardown)();               \
    ++mu_tests_run;                 \
    if (mu_test_status == 1) {      \
      ++mu_tests_failed;            \
    }                               \
    else if (mu_test_status == 2) { \
      ++mu_tests_skipped;           \
    }                               \
    else {                          \
      MU_MSG(" ✔︎\n");               \
    }                               \
  } while (0)

  /**
   * \brief Registers a function as a test suite.
   *
   * A **test suite** is just a function that registers zero or more tests
   * using mu_test(). Test suites may be used to partition the tests into
   * distinct logical groups. For example:
   *
   *     static void my_test_suite(void) {
   *       mu_test(something);
   *       mu_test(something_else);
   *     }
   *
   *     int main(void) {
   *       mu_init();
   *       mu_run_test_suite(my_test_suite);
   *       mu_test_summary();
   *       return (mu_tests_failed > 0);
   *     }
   *
   * \param name The name of the test suite.
   */
#define mu_run_test_suite(name)                                       \
  do {                                                                \
    MU_MSG("\n---- RUNNING SUITE: %s\n", #name);                      \
    start_time = clock();                                             \
    name();                                                           \
    time_elapsed += 1000.0 * (clock() - start_time) / CLOCKS_PER_SEC; \
  } while (0)

  /**
   * \brief Initializes MU Unit.
   *
   * This macro **must** be invoked before running any test.
   */
#define mu_init()         \
  do {                    \
    mu_tests_run = 0;     \
    mu_tests_failed = 0;  \
    mu_tests_skipped = 0; \
    time_elapsed = 0.0;   \
  } while (0)

  /**
   * \brief Prints a summary of the test results.
   *
   * This macro is typically invoked after all the tests have run. It is
   * possible, however, to call it between tests, e.g., after each test suite,
   * in which case it will print the test results up to the point of
   * invocation. If you call it between tests, you might probably want to reset
   * MU Unit with mu_init(). For example:
   *
   *     int main(void) {
   *       mu_init();
   *       mu_run_test_suite(test_suite_one);
   *       mu_test_summary();                 // Get results for test_suite_one
   *       int tot_failed = mu_tests_failed;
   *       mu_init();                         // Reset counters and time
   *       mu_run_test_suite(test_suite_two);
   *       mu_test_summary();                 // Get results for test_suite_two
   *       tot_failed += mu_tests_failed;
   *       return (tot_failed > 0);           // Exit status based on overall results
   *     }
   *
   */
#define mu_test_summary()                              \
  do {                                                 \
    if (mu_tests_failed > 0) {                         \
      MU_MSG("\nTHERE ARE FAILED TESTS\n");            \
      MU_MSG("Tests run: %d\n", mu_tests_run);         \
      MU_MSG("Tests failed: %d\n", mu_tests_failed);   \
    }                                                  \
    else {                                             \
      MU_MSG("\nALL TESTS PASSED!\n");                 \
      MU_MSG("Tests run: %d\n", mu_tests_run);         \
    }                                                  \
    if (mu_tests_skipped > 0) {                        \
      MU_MSG("Tests skipped: %d\n", mu_tests_skipped); \
    }                                                  \
    MU_MSG("Time elapsed: %.3fms\n", time_elapsed);    \
  } while (0)

#ifdef  __cplusplus
}
#endif

#endif /* mu_unit_h */

