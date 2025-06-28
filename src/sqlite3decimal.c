/**
 * \file      decimal.c
 * \author    Lifepillar
 * \copyright Copyright (c) 2019 Lifepillar.
 *            This program is free software; you can redistribute it and/or
 *            modify it under the terms of the Simplified BSD License (also
 *            known as the "2-Clause License" or "FreeBSD License".)
 * \copyright This program is distributed in the hope that it will be useful,
 *            but without any warranty; without even the implied warranty of
 *            merchantability or fitness for a particular purpose.
 *
 * \brief     SQLite3 Decimal extension
 */
#include <stdlib.h>
#include <string.h>
#include "decimal.h"

SQLITE_EXTENSION_INIT1

#pragma mark Macros

#if !defined(SQLITE_DECIMAL_PREFIX)
/**
 * \brief The prefix of all SQL decimal functions.
 */
#define SQLITE_DECIMAL_PREFIX "dec"
#endif

/**
 * \brief Checks for `NULL`s and sets the result accordingly.
 **/
#define CHECK_NULL(context, arg)                \
  if (sqlite3_value_type(arg) == SQLITE_NULL) { \
    sqlite3_result_null(context);               \
    return;                                     \
  }

#pragma mark Nullary functions

/**
 * \brief Prototype for generating nullary (0-ary) functions.
 */
#define SQLITE_DECIMAL_OP0(fun)                                                                              \
  static void decimal ## fun ## Func(sqlite3_context* context, int argc, sqlite3_value* argv[static argc]) { \
    (void)argc;                                                                                              \
    (void)argv;                                                                                              \
    decimal ## fun(context);                                                                                 \
  }

SQLITE_DECIMAL_OP0(ClearStatus)
SQLITE_DECIMAL_OP0(Status)
SQLITE_DECIMAL_OP0(Version)

#pragma mark Unary functions

/**
 * \brief Prototype for generating unary functions.
 */
#define SQLITE_DECIMAL_OP1(fun)                                                                               \
  static void decimal ## fun ## Func (sqlite3_context* context, int argc, sqlite3_value* argv[static argc]) { \
    (void) argc;                                                                                              \
    CHECK_NULL(context, argv[0])                                                                              \
    decimal ## fun(context, argv[0]);                                                                         \
  }

SQLITE_DECIMAL_OP1(Abs)
SQLITE_DECIMAL_OP1(Bytes)
SQLITE_DECIMAL_OP1(Class)
SQLITE_DECIMAL_OP1(Create)
SQLITE_DECIMAL_OP1(Exp)
SQLITE_DECIMAL_OP1(GetCoefficient)
SQLITE_DECIMAL_OP1(GetExponent)
SQLITE_DECIMAL_OP1(Invert)
SQLITE_DECIMAL_OP1(IsCanonical)
SQLITE_DECIMAL_OP1(IsFinite)
SQLITE_DECIMAL_OP1(IsInfinite)
SQLITE_DECIMAL_OP1(IsInteger)
SQLITE_DECIMAL_OP1(IsLogical)
SQLITE_DECIMAL_OP1(IsNegative)
SQLITE_DECIMAL_OP1(IsNaN)
SQLITE_DECIMAL_OP1(IsNormal)
SQLITE_DECIMAL_OP1(IsPositive)
SQLITE_DECIMAL_OP1(IsSigned)
SQLITE_DECIMAL_OP1(IsSubnormal)
SQLITE_DECIMAL_OP1(IsZero)
SQLITE_DECIMAL_OP1(Ln)
SQLITE_DECIMAL_OP1(Log10)
SQLITE_DECIMAL_OP1(LogB)
SQLITE_DECIMAL_OP1(Minus)
SQLITE_DECIMAL_OP1(NextDown)
SQLITE_DECIMAL_OP1(NextUp)
SQLITE_DECIMAL_OP1(Plus)
SQLITE_DECIMAL_OP1(Reduce)
SQLITE_DECIMAL_OP1(Sqrt)
SQLITE_DECIMAL_OP1(ToInt32)
SQLITE_DECIMAL_OP1(ToInt64)
SQLITE_DECIMAL_OP1(ToIntegral)
SQLITE_DECIMAL_OP1(ToString)
SQLITE_DECIMAL_OP1(Trim)

#pragma mark Binary functions

/**
 * \brief Prototype for generating binary functions.
 */
#define SQLITE_DECIMAL_OP2(fun)                                                                               \
  static void decimal ## fun ## Func (sqlite3_context* context, int argc, sqlite3_value* argv[static argc]) { \
    (void)argc;                                                                                               \
    CHECK_NULL(context, argv[0])                                                                              \
    CHECK_NULL(context, argv[1])                                                                              \
    decimal ## fun(context, argv[0], argv[1]);                                                                \
  }

SQLITE_DECIMAL_OP2(And)
SQLITE_DECIMAL_OP2(Compare)
SQLITE_DECIMAL_OP2(Divide)
SQLITE_DECIMAL_OP2(DivideInteger)
SQLITE_DECIMAL_OP2(Equal)
SQLITE_DECIMAL_OP2(GreaterThan)
SQLITE_DECIMAL_OP2(GreaterThanOrEqual)
SQLITE_DECIMAL_OP2(LessThan)
SQLITE_DECIMAL_OP2(LessThanOrEqual)
SQLITE_DECIMAL_OP2(NotEqual)
SQLITE_DECIMAL_OP2(Or)
SQLITE_DECIMAL_OP2(Power)
SQLITE_DECIMAL_OP2(Quantize)
SQLITE_DECIMAL_OP2(Remainder)
SQLITE_DECIMAL_OP2(Rotate)
SQLITE_DECIMAL_OP2(ScaleB)
SQLITE_DECIMAL_OP2(Shift)
SQLITE_DECIMAL_OP2(SameQuantum)
SQLITE_DECIMAL_OP2(Subtract)
SQLITE_DECIMAL_OP2(Xor)

#pragma mark Ternary functions

/**
 * \brief Multiplies two decimals and adds a third decimal.
 *
 * \see decNumber's manual, p. 36 and p. 56.
 */
static void decimalFMAFunc(sqlite3_context* context, int argc, sqlite3_value** argv) {
  for (int i = 0; i < argc; ++i) {
    CHECK_NULL(context, argv[i])
  }
  decimalFMA(context, argv[0], argv[1], argv[2]);
}

#pragma mark Variadic functions

/**
 * \brief Prototype for generating variadic functions.
 */
#define SQLITE_DECIMAL_OPn(fun)                                                                               \
  static void decimal ## fun ## Func (sqlite3_context* context, int argc, sqlite3_value* argv[static argc]) { \
    for (int i = 0; i < argc; ++i) {                                                                          \
      CHECK_NULL(context, argv[i])                                                                            \
    }                                                                                                         \
    decimal ## fun(context, argc, argv);                                                                      \
  }

SQLITE_DECIMAL_OPn(Add)
SQLITE_DECIMAL_OPn(Max)
SQLITE_DECIMAL_OPn(MaxMag)
SQLITE_DECIMAL_OPn(Min)
SQLITE_DECIMAL_OPn(MinMag)
SQLITE_DECIMAL_OPn(Multiply)

#pragma mark Aggregate functions

/**
 * \brief Prototype for generating aggregate functions.
 */
#define SQLITE_DECIMAL_AGGR(fun)                                                                                 \
  static void decimal ## fun ## StepFunc(sqlite3_context* context, int argc, sqlite3_value* argv[static argc]) { \
    CHECK_NULL(context, argv[0]);                                                                                \
    decimal ## fun ## Step(context, argc, argv);                                                                 \
  }                                                                                                              \
  static void decimal ## fun ## FinalFunc(sqlite3_context* context) {                                            \
    decimal ## fun ## Final(context);                                                                            \
  }

SQLITE_DECIMAL_AGGR(Sum)
SQLITE_DECIMAL_AGGR(Min)
SQLITE_DECIMAL_AGGR(Max)
SQLITE_DECIMAL_AGGR(Avg)

#pragma mark Virtual tables

#ifndef SQLITE_OMIT_VIRTUALTABLE

/**
 * \brief The data structure managed by the virtual tables.
 *
 * Instances of this structure are used by the context, status and traps
 * virtual tables.
 */
struct decimalContextVTab {
  sqlite3_vtab base;  /**< Base class - must be first. */
  void* decCtx;       /**< Shared decimal context. */
  void* oldCtx;       /**< Keeps tracks of the original context, for rollback. */
  char* name;         /**< Name of the virtual table (used for reporting). */
};

typedef struct decimalContextVTab decimalContextVTab;

/**
 * \brief The data structure describing a cursor into a virtual table.
 *
 * Instances of this structure are used by the context, status and traps
 * virtual tables.
 */
struct decimalContextCursor {
  sqlite3_vtab_cursor base;
  int row;
};

typedef struct decimalContextCursor decimalContextCursor;

/** \brief Column index for the `prec` column of the Context virtual table.  */
#define SQLITE_DECIMAL_PREC_COLUMN 0
/** \brief Column index for the `emax` column of the Context virtual table.  */
#define SQLITE_DECIMAL_EMAX_COLUMN 1
/** \brief Column index for the `emin` column of the Context virtual table.  */
#define SQLITE_DECIMAL_EMIN_COLUMN 2
/** \brief Column index for the `round` column of the Context virtual table. */
#define SQLITE_DECIMAL_ROUNDING_COLUMN 3

/**
 * \brief SQL definition of the Context virtual table.
 *
 * This is a one-row table containing the settings of the current context for
 * decimal operations. Strictly speaking then, the only key of this table is
 * the empty set of attributes. But SQLite3 does not allow empty primary keys,
 * so an artificial proper superkey is used as a primary key instead (`rowid`).
 */
#define SQLITE_DECIMAL_CONTEXT_TABLE                \
  "create table " SQLITE_DECIMAL_PREFIX "Context (" \
  "  prec  integer not null,"                       \
  "  emax  integer not null,"                       \
  "  emin  integer not null,"                       \
  "  round text    not null"                        \
  ")"                                               \

/**
 * \brief SQL definition of the Status virtual table.
 */
#define SQLITE_DECIMAL_STATUS_TABLE                \
  "create table " SQLITE_DECIMAL_PREFIX "Status (" \
  "  flag text not null,"                          \
  "  primary key (flag)"                           \
  ") without rowid"                                \

/**
 * \brief SQL definition of the Traps virtual table.
 */
#define SQLITE_DECIMAL_TRAPS_TABLE                \
  "create table " SQLITE_DECIMAL_PREFIX "Traps (" \
  "  flag text not null,"                         \
  "  primary key (flag)"                          \
  ") without rowid"                               \


/**
 * \brief Connects a virtual table.
 */
#define SQLITE_DECIMAL_VTAB_CONNECT(vtabname, vtabdef)                    \
  static int decimal ## vtabname ## Connect(sqlite3*           db,        \
                                            void*              pAux,      \
                                            int                argc,      \
                                            char const* const* argv,      \
                                            sqlite3_vtab**     ppVtab,    \
                                            char**             pzErr) {   \
  (void)(argc);                                                           \
  (void)(argv);                                                           \
  (void)(pzErr);                                                          \
                                                                          \
  decimalContextVTab* pVtab;                                              \
  int rc;                                                                 \
                                                                          \
  rc = sqlite3_declare_vtab(db, vtabdef);                                 \
  if (rc == SQLITE_OK) {                                                  \
    pVtab = sqlite3_malloc(sizeof(*pVtab));                               \
    *ppVtab = (sqlite3_vtab*)pVtab;                                       \
    if (pVtab == 0) return SQLITE_NOMEM;                                  \
    memset(pVtab, 0, sizeof(*pVtab));                                     \
    pVtab->name = sqlite3_mprintf("%s", SQLITE_DECIMAL_PREFIX #vtabname); \
    pVtab->decCtx = pAux;                                                 \
    pVtab->oldCtx = 0;                                                    \
  }                                                                       \
  return rc;                                                              \
}

/**
 * \brief Disconnects a virtual table.
 */
#define SQLITE_DECIMAL_VTAB_DISCONNECT(vtabname)                       \
  static int decimal ## vtabname ## Disconnect(sqlite3_vtab* pVtab)  { \
    decimalContextVTab* p = (decimalContextVTab*)pVtab;                \
    if (p->name) sqlite3_free(p->name);                                \
    if (p->oldCtx) decimalContextDestroy(p->oldCtx);                   \
    sqlite3_free(p);                                                   \
    return SQLITE_OK;                                                  \
  }

/**
 * \brief Opens a cursor.
 */
#define SQLITE_DECIMAL_CUR_OPEN(vtabname)                                                   \
  static int decimal ## vtabname ## Open(sqlite3_vtab* p, sqlite3_vtab_cursor** ppCursor) { \
    (void)p;                                                                                \
    decimalContextCursor* pCur;                                                             \
    pCur = sqlite3_malloc(sizeof(*pCur));                                                   \
    if (pCur == 0) return SQLITE_NOMEM;                                                     \
    memset(pCur, 0, sizeof(*pCur));                                                         \
    *ppCursor = &pCur->base;                                                                \
    pCur->row = 0;                                                                          \
    return SQLITE_OK;                                                                       \
  }

/**
 * \brief Closes a cursors.
 */
#define SQLITE_DECIMAL_CUR_CLOSE(vtabname)                            \
  static int decimal ## vtabname ## Close(sqlite3_vtab_cursor* cur) { \
    sqlite3_free(cur);                                                \
    return SQLITE_OK;                                                 \
  }

/**
 * \brief Advances a cursor.
 */
#define SQLITE_DECIMAL_CUR_NEXT(vtabname)                            \
  static int decimal ## vtabname ## Next(sqlite3_vtab_cursor* cur) { \
    decimalContextCursor* pCur = (decimalContextCursor*)cur;         \
    pCur->row++;                                                     \
    return SQLITE_OK;                                                \
  }

/**
 * \brief Gets the row id the cursor is currently pointed at.
 */
#define SQLITE_DECIMAL_VTAB_ROWID(vtabname)                                                 \
  static int decimal ## vtabname ## Rowid(sqlite3_vtab_cursor* cur, sqlite_int64 *pRowid) { \
    decimalContextCursor* pCur = (decimalContextCursor*)cur;                                \
    *pRowid = pCur->row;                                                                    \
    return SQLITE_OK;                                                                       \
  }

/**
 * \brief Prototype for the xBestIndex method.
 */
#define SQLITE_DECIMAL_VTAB_BEST_INDEX(vtabname)                                                  \
  static int decimal ## vtabname ## BestIndex(sqlite3_vtab* tab, sqlite3_index_info* pIdxInfo)  { \
    (void)tab;                                                                                    \
    (void)pIdxInfo;                                                                               \
    return SQLITE_OK;                                                                             \
  }

/**
 * \brief Prototype for the xFilter method.
 */
#define SQLITE_DECIMAL_VTAB_FILTER(vtabname)                                             \
  static int decimal ## vtabname ## Filter(sqlite3_vtab_cursor* pVtabCursor, int idxNum, \
      char const* idxStr, int argc, sqlite3_value** argv) {                              \
    (void)pVtabCursor;                                                                   \
    (void)idxNum;                                                                        \
    (void)idxStr;                                                                        \
    (void)argc;                                                                          \
    (void)argv;                                                                          \
    return SQLITE_OK;                                                                    \
  }

/**
 * \brief Prototype for the xBegin method.
 */
#define SQLITE_DECIMAL_VTAB_BEGIN(vtabname)                      \
  static int decimal ## vtabname ## Begin(sqlite3_vtab* pVtab) { \
    decimalContextVTab* p = (decimalContextVTab*)pVtab;          \
    p->oldCtx = decimalContextCreate();                          \
    if (p->oldCtx == 0) return SQLITE_NOMEM;                     \
    decimalContextCopy(p->oldCtx, p->decCtx);                    \
    return SQLITE_OK;                                            \
  }

/**
 * \brief Prototype for the xCommit method.
 */
#define SQLITE_DECIMAL_VTAB_COMMIT(vtabname)                      \
  static int decimal ## vtabname ## Commit(sqlite3_vtab* pVtab) { \
    decimalContextVTab* p = (decimalContextVTab*)pVtab;           \
    decimalContextDestroy(p->oldCtx);                             \
    p->oldCtx = 0;                                                \
    return SQLITE_OK;                                             \
  }

/**
 * \brief Prototype for the xRollback method.
 */
#define SQLITE_DECIMAL_VTAB_ROLLBACK(vtabname)                      \
  static int decimal ## vtabname ## Rollback(sqlite3_vtab* pVtab) { \
    decimalContextVTab* p = (decimalContextVTab*)pVtab;             \
    decimalContextCopy(p->decCtx, p->oldCtx);                       \
    decimalContextDestroy(p->oldCtx);                               \
    p->oldCtx = 0;                                                  \
    return SQLITE_OK;                                               \
  }

/**
 * \brief Prototype for the xRename method.
 */
#define SQLITE_DECIMAL_VTAB_RENAME(vtabname)                                        \
  static int decimal ## vtabname ## Rename(sqlite3_vtab* pVtab, char const* zNew) { \
    decimalContextVTab* p = (decimalContextVTab*)pVtab;                             \
    if (p->name) sqlite3_free(p->name);                                             \
    p->name = sqlite3_mprintf(zNew);                                                \
    if (p->name == 0) return SQLITE_NOMEM;                                          \
    return SQLITE_OK;                                                               \
  }

/**
 * \brief Prototype for a module.
 */
#define SQLITE_DECIMAL_MODULE(name)                   \
  static sqlite3_module decimal ## name ## Module = { \
    0,                                                \
    0,                                                \
    decimal ## name ## Connect,                       \
    decimal ## name ## BestIndex,                     \
    decimal ## name ## Disconnect,                    \
    decimal ## name ## Disconnect,                    \
    decimal ## name ## Open,                          \
    decimal ## name ## Close,                         \
    decimal ## name ## Filter,                        \
    decimal ## name ## Next,                          \
    decimal ## name ## Eof,                           \
    decimal ## name ## Column,                        \
    decimal ## name ## Rowid,                         \
    decimal ## name ## Update,                        \
    decimal ## name ## Begin,                         \
    0,                                                \
    decimal ## name ## Commit,                        \
    decimal ## name ## Rollback,                      \
    0,                                                \
    decimal ## name ## Rename,                        \
    0,                                                \
    0,                                                \
    0,                                                \
    0,                                                \
    0,                                                \
  };

#pragma mark Context virtual table

SQLITE_DECIMAL_VTAB_CONNECT(Context, SQLITE_DECIMAL_CONTEXT_TABLE)
SQLITE_DECIMAL_VTAB_DISCONNECT(Context)
SQLITE_DECIMAL_CUR_OPEN(Context)
SQLITE_DECIMAL_CUR_NEXT(Context)
SQLITE_DECIMAL_CUR_CLOSE(Context)
SQLITE_DECIMAL_VTAB_ROWID(Context)
SQLITE_DECIMAL_VTAB_BEST_INDEX(Context)
SQLITE_DECIMAL_VTAB_FILTER(Context)
SQLITE_DECIMAL_VTAB_BEGIN(Context)
SQLITE_DECIMAL_VTAB_COMMIT(Context)
SQLITE_DECIMAL_VTAB_ROLLBACK(Context)
SQLITE_DECIMAL_VTAB_RENAME(Context)

/**
 * \brief Implementation of the xEof method for the Context virtual table.
 */
static int decimalContextEof(sqlite3_vtab_cursor* cur) {
  decimalContextCursor* pCur = (decimalContextCursor*)cur;
  return pCur->row == 1;
}

/**
 * \brief Implementation of the xColumn method for the Context virtual table.
 */
static int decimalContextColumn(sqlite3_vtab_cursor* cur, sqlite3_context* ctx, int i) {
  decimalContextCursor* pCur = (decimalContextCursor*)cur;
  decimalContextVTab* pVtab = (decimalContextVTab*)(pCur->base.pVtab);

  switch (i) {
    case SQLITE_DECIMAL_PREC_COLUMN: {
      sqlite3_result_int(ctx, decimalPrecision(pVtab->decCtx));
      break;
    }
    case SQLITE_DECIMAL_EMAX_COLUMN: {
      sqlite3_result_int(ctx, decimalMaxExp(pVtab->decCtx));
      break;
    }
    case SQLITE_DECIMAL_EMIN_COLUMN: {
      sqlite3_result_int(ctx, decimalMinExp(pVtab->decCtx));
      break;
    }
    case SQLITE_DECIMAL_ROUNDING_COLUMN: {
      char const* mode = decimalRoundingMode(pVtab->decCtx);
      sqlite3_result_text(ctx, mode, -1, SQLITE_TRANSIENT);
      break;
    }
    default:
      break;
  }
  return SQLITE_OK;
}

/**
 * \brief Implementation of the xUpdate method for the Context virtual table.
 */
static int decimalContextUpdate(sqlite3_vtab* pVtab, int argc, sqlite3_value** argv, sqlite_int64* pRowid) {
  (void)pRowid;
  decimalContextVTab* p = (decimalContextVTab*)pVtab;

  if (argc == 1) {
    pVtab->zErrMsg = sqlite3_mprintf("Deleting from %s is not allowed", p->name);
    return SQLITE_ERROR;
  }
  if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
    pVtab->zErrMsg = sqlite3_mprintf("Inserting into %s is not allowed", p->name);
    return SQLITE_ERROR;
  }

  if (argc < 3) return SQLITE_ERROR; // This should never be the case

  for (int i = 2; i < argc; i++) {
    if (sqlite3_value_type(argv[i]) == SQLITE_NULL) {
      pVtab->zErrMsg = sqlite3_mprintf("Value cannot be NULL");
      return SQLITE_ERROR;
    }
  }

  // Update
  if (decimalSetPrecision(p->decCtx, sqlite3_value_int(argv[SQLITE_DECIMAL_PREC_COLUMN + 2]), &(pVtab->zErrMsg)) == SQLITE_ERROR)
    return SQLITE_ERROR;
  if (decimalSetMaxExp(p->decCtx, sqlite3_value_int(argv[SQLITE_DECIMAL_EMAX_COLUMN + 2]), &(pVtab->zErrMsg)) == SQLITE_ERROR)
    return SQLITE_ERROR;
  if (decimalSetMinExp(p->decCtx, sqlite3_value_int(argv[SQLITE_DECIMAL_EMIN_COLUMN + 2]), &(pVtab->zErrMsg)) == SQLITE_ERROR)
    return SQLITE_ERROR;
  if (decimalSetRoundingMode(p->decCtx, (char*)sqlite3_value_text(argv[SQLITE_DECIMAL_ROUNDING_COLUMN+2]), &(pVtab->zErrMsg)) == SQLITE_ERROR)
    return SQLITE_ERROR;
  return SQLITE_OK;
}

/**
 * \brief An eponymous-only virtual table module that provides access to the
 *        shared decimal context.
 */
SQLITE_DECIMAL_MODULE(Context)

#pragma mark Status virtual table

SQLITE_DECIMAL_VTAB_CONNECT(Status, SQLITE_DECIMAL_STATUS_TABLE)
SQLITE_DECIMAL_VTAB_DISCONNECT(Status)
SQLITE_DECIMAL_CUR_OPEN(Status)
SQLITE_DECIMAL_CUR_NEXT(Status)
SQLITE_DECIMAL_CUR_CLOSE(Status)
SQLITE_DECIMAL_VTAB_ROWID(Status)
SQLITE_DECIMAL_VTAB_BEST_INDEX(Status)
SQLITE_DECIMAL_VTAB_FILTER(Status)
SQLITE_DECIMAL_VTAB_BEGIN(Status)
SQLITE_DECIMAL_VTAB_COMMIT(Status)
SQLITE_DECIMAL_VTAB_ROLLBACK(Status)
SQLITE_DECIMAL_VTAB_RENAME(Status)

/**
 * \brief Implementation of the xEof method for the Status virtual table.
 */
static int decimalStatusEof(sqlite3_vtab_cursor* cur) {
  decimalContextCursor* pCur = (decimalContextCursor*)cur;
  decimalContextVTab* pVtab = (decimalContextVTab*)(pCur->base.pVtab);
  return (decimalGetFlag(pVtab->decCtx, pCur->row) == 0);
}

/**
 * \brief Implementation of the xColumn method for the Status virtual table.
 */
static int decimalStatusColumn(sqlite3_vtab_cursor* cur, sqlite3_context* ctx, int i) {
  decimalContextCursor* pCur = (decimalContextCursor*)cur;
  decimalContextVTab* pVtab = (decimalContextVTab*)(pCur->base.pVtab);

  switch (i) {
    case 0: {
      sqlite3_result_text(ctx, decimalGetFlag(pVtab->decCtx, pCur->row), -1, SQLITE_STATIC);
      break;
    }
    default:
      break;
  }
  return SQLITE_OK;
}

/**
 * \brief Implementation of the xUpdate method for the Status virtual table.
 */
static int decimalStatusUpdate(sqlite3_vtab* pVtab, int argc, sqlite3_value** argv, sqlite_int64* pRowid) {
  (void)pRowid;
  decimalContextVTab* p = (decimalContextVTab*)pVtab;

  if (argc == 1) { // Deletion
    return decimalClearFlag(p->decCtx, (char*)sqlite3_value_text(argv[0]), &(pVtab->zErrMsg));
  }

  if (argc < 3) return SQLITE_ERROR; // This should never happen

  if (sqlite3_value_type(argv[2]) == SQLITE_NULL) {
    pVtab->zErrMsg = sqlite3_mprintf("Value cannot be NULL");
    return SQLITE_ERROR;
  }

  if (sqlite3_value_type(argv[0]) == SQLITE_NULL) { // Insertion
    return decimalSetFlag(p->decCtx, (char*)sqlite3_value_text(argv[2]), &(pVtab->zErrMsg));
  }

  // Update
  if (decimalClearFlag(p->decCtx, (char*)sqlite3_value_text(argv[0]), &(pVtab->zErrMsg)) == SQLITE_OK) {
    return decimalSetFlag(p->decCtx, (char*)sqlite3_value_text(argv[2]), &(pVtab->zErrMsg));
  }
  return SQLITE_ERROR;
}

/**
 * \brief An eponymous-only virtual table module that provides access to the
 *        shared decimal status.
 */
SQLITE_DECIMAL_MODULE(Status)

#pragma mark Traps virtual table

SQLITE_DECIMAL_VTAB_CONNECT(Traps, SQLITE_DECIMAL_TRAPS_TABLE)
SQLITE_DECIMAL_VTAB_DISCONNECT(Traps)
SQLITE_DECIMAL_CUR_OPEN(Traps)
SQLITE_DECIMAL_CUR_NEXT(Traps)
SQLITE_DECIMAL_CUR_CLOSE(Traps)
SQLITE_DECIMAL_VTAB_ROWID(Traps)
SQLITE_DECIMAL_VTAB_BEST_INDEX(Traps)
SQLITE_DECIMAL_VTAB_FILTER(Traps)
SQLITE_DECIMAL_VTAB_BEGIN(Traps)
SQLITE_DECIMAL_VTAB_COMMIT(Traps)
SQLITE_DECIMAL_VTAB_ROLLBACK(Traps)
SQLITE_DECIMAL_VTAB_RENAME(Traps)

/**
 * \brief Implementation of the xEof method for the Traps virtual table.
 */
static int decimalTrapsEof(sqlite3_vtab_cursor* cur) {
  decimalContextCursor* pCur = (decimalContextCursor*)cur;
  decimalContextVTab* pVtab = (decimalContextVTab*)(pCur->base.pVtab);
  return (decimalGetTrap(pVtab->decCtx, pCur->row) == 0);
}

/**
 * \brief Implementation of the xColumn method for the Traps virtual table.
 */
static int decimalTrapsColumn(sqlite3_vtab_cursor* cur, sqlite3_context* ctx, int i) {
  decimalContextCursor* pCur = (decimalContextCursor*)cur;
  decimalContextVTab* pVtab = (decimalContextVTab*)(pCur->base.pVtab);

  switch (i) {
    case 0: {
      sqlite3_result_text(ctx, decimalGetTrap(pVtab->decCtx, pCur->row), -1, SQLITE_STATIC);
      break;
    }
    default:
      break;
  }
  return SQLITE_OK;
}

/**
 * \brief Implementation of the xUpdate method for the Traps virtual table.
 */
static int decimalTrapsUpdate(sqlite3_vtab* pVtab, int argc, sqlite3_value** argv, sqlite_int64* pRowid) {
  (void)pRowid;
  decimalContextVTab* p = (decimalContextVTab*)pVtab;

  if (argc == 1) { // Deletion
    return decimalClearTrap(p->decCtx, (char*)sqlite3_value_text(argv[0]), &(pVtab->zErrMsg));
  }

  if (argc < 3) return SQLITE_ERROR; // This should never be the case

  if (sqlite3_value_type(argv[2]) == SQLITE_NULL) {
    pVtab->zErrMsg = sqlite3_mprintf("Value cannot be NULL");
    return SQLITE_ERROR;
  }

  if (sqlite3_value_type(argv[0]) == SQLITE_NULL) { // Insertion
    // Remove the flag from the status flag (if it exists) and add it to the traps table
    if (decimalClearFlag(p->decCtx, (char*)sqlite3_value_text(argv[2]), &(pVtab->zErrMsg)) == SQLITE_OK) {
      return decimalSetTrap(p->decCtx, (char*)sqlite3_value_text(argv[2]), &(pVtab->zErrMsg));
    }
    return SQLITE_ERROR;
  }

  // Update
  if (decimalClearTrap(p->decCtx, (char*)sqlite3_value_text(argv[0]), &(pVtab->zErrMsg)) == SQLITE_OK) {
    // See above
    if (decimalClearFlag(p->decCtx, (char*)sqlite3_value_text(argv[2]), &(pVtab->zErrMsg)) == SQLITE_OK) {
      return decimalSetTrap(p->decCtx, (char*)sqlite3_value_text(argv[2]), &(pVtab->zErrMsg));
    }
    return SQLITE_ERROR;
  }
  return SQLITE_ERROR;
}

/**
 ** \brief An eponymous-only virtual table module that provides access to the
 **        shared decimal status.
 **/
SQLITE_DECIMAL_MODULE(Traps)

#endif /* SQLITE_OMIT_VIRTUALTABLE */

#pragma mark Public interface

/**
 ** \brief Entry point of the SQLite3 Decimal extension.
 **/
#ifdef _WIN32
__declspec(dllexport)
#endif

int sqlite3_decimal_init(
    sqlite3* db,
    [[maybe_unused]] char** pzErrMsg,
    sqlite3_api_routines const* pApi
    ) {
  static const struct {
    char const* zName;
    int nArg;
    void (*xFunc)(sqlite3_context*, int, sqlite3_value**);
  } aFunc[] = {
    { SQLITE_DECIMAL_PREFIX "",               1, decimalCreateFunc             },
    { SQLITE_DECIMAL_PREFIX "Abs",            1, decimalAbsFunc                },
    { SQLITE_DECIMAL_PREFIX "Add",           -1, decimalAddFunc                },
    { SQLITE_DECIMAL_PREFIX "And",            2, decimalAndFunc                },
    { SQLITE_DECIMAL_PREFIX "Bytes",          1, decimalBytesFunc              },
    { SQLITE_DECIMAL_PREFIX "Class",          1, decimalClassFunc              },
    { SQLITE_DECIMAL_PREFIX "ClearStatus",    0, decimalClearStatusFunc        },
    { SQLITE_DECIMAL_PREFIX "Cmp",            2, decimalCompareFunc            },
    { SQLITE_DECIMAL_PREFIX "Compare",        2, decimalCompareFunc            },
    { SQLITE_DECIMAL_PREFIX "Div",            2, decimalDivideFunc             },
    { SQLITE_DECIMAL_PREFIX "DivInt",         2, decimalDivideIntegerFunc      },
    { SQLITE_DECIMAL_PREFIX "Eq",             2, decimalEqualFunc              },
    { SQLITE_DECIMAL_PREFIX "Equal",          2, decimalEqualFunc              },
    { SQLITE_DECIMAL_PREFIX "Exp",            1, decimalExpFunc                },
    { SQLITE_DECIMAL_PREFIX "Exponent",       1, decimalGetExponentFunc        },
    { SQLITE_DECIMAL_PREFIX "FMA",            3, decimalFMAFunc                },
    { SQLITE_DECIMAL_PREFIX "Ge",             2, decimalGreaterThanOrEqualFunc },
    { SQLITE_DECIMAL_PREFIX "Greatest",      -1, decimalMaxFunc                },
    { SQLITE_DECIMAL_PREFIX "Gt",             2, decimalGreaterThanFunc        },
    { SQLITE_DECIMAL_PREFIX "Invert",         1, decimalInvertFunc             },
    { SQLITE_DECIMAL_PREFIX "IsCanonical",    1, decimalIsCanonicalFunc        },
    { SQLITE_DECIMAL_PREFIX "IsFinite",       1, decimalIsFiniteFunc           },
    { SQLITE_DECIMAL_PREFIX "IsInf",          1, decimalIsInfiniteFunc         },
    { SQLITE_DECIMAL_PREFIX "IsInfinite",     1, decimalIsInfiniteFunc         },
    { SQLITE_DECIMAL_PREFIX "IsInt",          1, decimalIsIntegerFunc          },
    { SQLITE_DECIMAL_PREFIX "IsInteger",      1, decimalIsIntegerFunc          },
    { SQLITE_DECIMAL_PREFIX "IsLogical",      1, decimalIsLogicalFunc          },
    { SQLITE_DECIMAL_PREFIX "IsNaN",          1, decimalIsNaNFunc              },
    { SQLITE_DECIMAL_PREFIX "IsNeg",          1, decimalIsNegativeFunc         },
    { SQLITE_DECIMAL_PREFIX "IsNegative",     1, decimalIsNegativeFunc         },
    { SQLITE_DECIMAL_PREFIX "IsNormal",       1, decimalIsNormalFunc           },
    { SQLITE_DECIMAL_PREFIX "IsPos",          1, decimalIsPositiveFunc         },
    { SQLITE_DECIMAL_PREFIX "IsPositive",     1, decimalIsPositiveFunc         },
    { SQLITE_DECIMAL_PREFIX "IsSigned",       1, decimalIsSignedFunc           },
    { SQLITE_DECIMAL_PREFIX "IsSubnormal",    1, decimalIsSubnormalFunc        },
    { SQLITE_DECIMAL_PREFIX "IsZero",         1, decimalIsZeroFunc             },
    { SQLITE_DECIMAL_PREFIX "Le",             2, decimalLessThanOrEqualFunc    },
    { SQLITE_DECIMAL_PREFIX "Least",         -1, decimalMinFunc                },
    { SQLITE_DECIMAL_PREFIX "Log10",          1, decimalLog10Func              },
    { SQLITE_DECIMAL_PREFIX "LogB",           1, decimalLogBFunc               },
    { SQLITE_DECIMAL_PREFIX "Ln",             1, decimalLnFunc                 },
    { SQLITE_DECIMAL_PREFIX "Lt",             2, decimalLessThanFunc           },
    { SQLITE_DECIMAL_PREFIX "Mantissa",       1, decimalGetCoefficientFunc     },
    { SQLITE_DECIMAL_PREFIX "MaxMag",        -1, decimalMaxMagFunc             },
    { SQLITE_DECIMAL_PREFIX "MinMag",        -1, decimalMinMagFunc             },
    { SQLITE_DECIMAL_PREFIX "Minus",          1, decimalMinusFunc              },
    { SQLITE_DECIMAL_PREFIX "Mul",           -1, decimalMultiplyFunc           },
    { SQLITE_DECIMAL_PREFIX "Ne",             2, decimalNotEqualFunc           },
    { SQLITE_DECIMAL_PREFIX "NextDown",       1, decimalNextDownFunc           },
    { SQLITE_DECIMAL_PREFIX "NextUp",         1, decimalNextUpFunc             },
    { SQLITE_DECIMAL_PREFIX "NotEqual",       2, decimalNotEqualFunc           },
    { SQLITE_DECIMAL_PREFIX "Or",             2, decimalOrFunc                 },
    { SQLITE_DECIMAL_PREFIX "Pow",            2, decimalPowerFunc              },
    { SQLITE_DECIMAL_PREFIX "Plus",           1, decimalPlusFunc               },
    { SQLITE_DECIMAL_PREFIX "Quantize",       2, decimalQuantizeFunc           },
    { SQLITE_DECIMAL_PREFIX "Reduce",         1, decimalReduceFunc             },
    { SQLITE_DECIMAL_PREFIX "Remainder",      2, decimalRemainderFunc          },
    { SQLITE_DECIMAL_PREFIX "Rotate",         2, decimalRotateFunc             },
    { SQLITE_DECIMAL_PREFIX "SameQuantum",    2, decimalSameQuantumFunc        },
    { SQLITE_DECIMAL_PREFIX "ScaleB",         2, decimalScaleBFunc             },
    { SQLITE_DECIMAL_PREFIX "Shift",          2, decimalShiftFunc              },
    { SQLITE_DECIMAL_PREFIX "Status",         0, decimalStatusFunc             },
    { SQLITE_DECIMAL_PREFIX "Str",            1, decimalToStringFunc           },
    { SQLITE_DECIMAL_PREFIX "Sqrt",           1, decimalSqrtFunc               },
    { SQLITE_DECIMAL_PREFIX "Sub",            2, decimalSubtractFunc           },
    { SQLITE_DECIMAL_PREFIX "ToInt32",        1, decimalToInt32Func            },
    { SQLITE_DECIMAL_PREFIX "ToInt64",        1, decimalToInt64Func            },
    { SQLITE_DECIMAL_PREFIX "ToIntegral",     1, decimalToIntegralFunc         },
    { SQLITE_DECIMAL_PREFIX "Trim",           1, decimalTrimFunc               },
    { SQLITE_DECIMAL_PREFIX "Version",        0, decimalVersionFunc            },
    { SQLITE_DECIMAL_PREFIX "Xor",            2, decimalXorFunc                },
  };

  static const struct {
    char const* zName;
    int nArg;
    void (*xStep)(sqlite3_context*, int, sqlite3_value**);
    void (*xFinal)(sqlite3_context*);
  } aAgg[] = {
    { SQLITE_DECIMAL_PREFIX "Sum", 1, decimalSumStepFunc, decimalSumFinalFunc },
    { SQLITE_DECIMAL_PREFIX "Min", 1, decimalMinStepFunc, decimalMinFinalFunc },
    { SQLITE_DECIMAL_PREFIX "Max", 1, decimalMaxStepFunc, decimalMaxFinalFunc },
    { SQLITE_DECIMAL_PREFIX "Avg", 1, decimalAvgStepFunc, decimalAvgFinalFunc },
  };

  int rc = SQLITE_OK;
  SQLITE_EXTENSION_INIT2(pApi);

  void* decimalSharedContext = decimalInitSystem();
  if (decimalSharedContext == 0)
    return SQLITE_NOMEM;

  for (size_t i = 0; i < sizeof(aFunc) / sizeof(aFunc[0]) && rc == SQLITE_OK; i++) {
    rc = sqlite3_create_function(db, aFunc[i].zName, aFunc[i].nArg,
                                 SQLITE_UTF8 | SQLITE_DETERMINISTIC,
                                 decimalSharedContext,
                                 aFunc[i].xFunc, 0, 0);
  }
  for (size_t i = 0; i < sizeof(aAgg) / sizeof(aAgg[0]) && rc == SQLITE_OK; i++) {
    rc = sqlite3_create_function(db, aAgg[i].zName, aAgg[i].nArg,
                                 SQLITE_UTF8 | SQLITE_DETERMINISTIC,
                                 decimalSharedContext,
                                 0, aAgg[i].xStep, aAgg[i].xFinal);
  }

#ifndef SQLITE_OMIT_VIRTUALTABLE
  if (rc == SQLITE_OK) {
    rc = sqlite3_create_module(db, SQLITE_DECIMAL_PREFIX "Status",
                               &decimalStatusModule, decimalSharedContext);
  }
  if (rc == SQLITE_OK) {
    rc = sqlite3_create_module(db, SQLITE_DECIMAL_PREFIX "Traps",
                               &decimalTrapsModule, decimalSharedContext);
  }
  if (rc == SQLITE_OK) {
    rc = sqlite3_create_module_v2(db, SQLITE_DECIMAL_PREFIX "Context",
                                  &decimalContextModule, decimalSharedContext, decimalContextDestroy);
  }
#endif

  return rc;
}

