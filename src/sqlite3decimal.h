/**
 * \file      decimal.h
 * \author    Lifepillar
 * \copyright Copyright (c) 2019 Lifepillar.
 *            This program is free software; you can redistribute it and/or
 *            modify it under the terms of the Simplified BSD License (also
 *            known as the "2-Clause License" or "FreeBSD License".)
 * \copyright This program is distributed in the hope that it will be useful,
 *            but without any warranty; without even the implied warranty of
 *            merchantability or fitness for a particular purpose.
 *
 * \brief     SQLite3 Decimal public header
 */
#ifndef sqlite3_decimal_h
#define sqlite3_decimal_h

#include "autoconfig.h"
#include "version.h"

/**
 * \brief SQLite Decimal long version string.
 */
#define SQLITE_DECIMAL_VERSION "SQLite3Decimal v" SQLITE_DECIMAL_SHORT_VERSION

#ifdef __cplusplus
extern "C" {
#endif

#include "sqlite3.h"

int sqlite3_decimal_init(sqlite3* db, char** pzErrMsg, sqlite3_api_routines const* pApi);

#ifdef __cplusplus
}
#endif

#endif /* sqlite3_decimal_h */

