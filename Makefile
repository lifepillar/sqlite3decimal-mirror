# See https://pubs.opengroup.org/onlinepubs/9699919799/utilities/make.html
.POSIX:
.SUFFIXES:
.SUFFIXES: .o .c

NAME                = sqlite3decimal
VERSION             = 0.0.1
BUILDDIR            = build
TESTDIR             = test

CFLAGS              = -fPIC -Wall -Wextra
CFLAGS             += -Isrc -Isrc/decNumber -Isrc/sqlite
CFLAGS             += -DDECUSE64=1 -DDECSUBSET=0

SQLITE_OPTIONS      = -Wno-unused-parameter
SQLITE_OPTIONS     += -DSQLITE_THREADSAFE=0
SQLITE_OPTIONS     += -DSQLITE_DEFAULT_MEMSTATUS=0
SQLITE_OPTIONS     += -DSQLITE_DEFAULT_WAL_SYNCHRONOUS=1
SQLITE_OPTIONS     += -DSQLITE_LIKE_DOESNT_MATCH_BLOBS
SQLITE_OPTIONS     += -DSQLITE_MAX_EXPR_DEPTH=0
SQLITE_OPTIONS     += -DSQLITE_OMIT_DECLTYPE
SQLITE_OPTIONS     += -DSQLITE_OMIT_DEPRECATED
SQLITE_OPTIONS     += -DSQLITE_OMIT_PROGRESS_CALLBACK
SQLITE_OPTIONS     += -DSQLITE_OMIT_SHARED_CACHE
SQLITE_OPTIONS     += -DSQLITE_USE_ALLOCA

# Defaults for Darwin
# (Use make SOEXT=so LDFLAGS=-shared to override)
SOEXT               = dylib
LDFLAGS             = -dynamiclib

# Products
LIB                 = lib$(NAME).a
LIBV                = lib$(NAME).$(VERSION).a
DYLIB               = lib$(NAME).$(SOEXT)
DYLIBV              = lib$(NAME).$(VERSION).$(SOEXT)

# Source files
SRCS                = src/decNumber/decContext.c
SRCS               += src/decNumber/decPacked.c
SRCS               += src/decimal.c
SRCS               += src/impl_decnumber.c

DEBUGDIR            = $(BUILDDIR)/debug
RELEASEDIR          = $(BUILDDIR)/release
OBJDIR_DEBUG        = $(DEBUGDIR)/obj
OBJDIR_RELEASE      = $(RELEASEDIR)/obj

DEBUG_LIB           = $(DEBUGDIR)/$(LIB)
DEBUG_DYLIB         = $(DEBUGDIR)/$(DYLIB)
DEBUG_LIBV          = $(DEBUGDIR)/$(LIBV)
DEBUG_DYLIBV        = $(DEBUGDIR)/$(DYLIBV)
RELEASE_LIB         = $(RELEASEDIR)/$(LIB)
RELEASE_DYLIB       = $(RELEASEDIR)/$(DYLIB)
RELEASE_LIBV        = $(RELEASEDIR)/$(LIBV)
RELEASE_DYLIBV      = $(RELEASEDIR)/$(DYLIBV)

DEBUGOBJS           = $(OBJDIR_DEBUG)/decContext.o
DEBUGOBJS          += $(OBJDIR_DEBUG)/decNumber.o
DEBUGOBJS          += $(OBJDIR_DEBUG)/decPacked.o
DEBUGOBJS          += $(OBJDIR_DEBUG)/decimal.o
DEBUGOBJS          += $(OBJDIR_DEBUG)/impl_decnumber.o
RELEASEOBJS         = $(OBJDIR_RELEASE)/decContext.o
RELEASEOBJS        += $(OBJDIR_RELEASE)/decNumber.o
RELEASEOBJS        += $(OBJDIR_RELEASE)/decPacked.o
RELEASEOBJS        += $(OBJDIR_RELEASE)/decimal.o
RELEASEOBJS        += $(OBJDIR_RELEASE)/impl_decnumber.o

# Targets

.PHONY: all
all: debug release

# Dependencies
$(OBJDIR_DEBUG)/decContext.o $(OBJDIR_RELEASE)/decContext.o:         \
	src/decNumber/decContext.c                                         \
	src/decNumber/decContext.h src/decNumber/decNumberLocal.h

$(OBJDIR_DEBUG)/decNumber.o $(OBJDIR_RELEASE)/decNumber.o:           \
	src/decNumber/decNumber.c src/decNumber/decNumber.h                \
	src/decNumber/decContext.h src/decNumber/decNumberLocal.h

$(OBJDIR_DEBUG)/decPacked.o $(OBJDIR_RELEASE)/decPacked.o:           \
	src/decNumber/decPacked.c src/decNumber/decNumber.h                \
	src/decNumber/decContext.h src/decNumber/decPacked.h               \
	src/decNumber/decNumberLocal.h


$(OBJDIR_DEBUG)/decimal.o $(OBJDIR_RELEASE)/decimal.o:               \
	src/decimal.c src/impl_decimal.h src/decimal.h src/version.h       \

$(OBJDIR_DEBUG)/impl_decnumber.o $(OBJDIR_RELEASE)/impl_decnumber.o: \
	src/impl_decnumber.c src/decNumber/decNumber.h                     \
	src/decNumber/decContext.h src/decNumber/decPacked.h               \
	src/impl_decimal.h src/decimal.h src/version.h


src/version.h: Makefile
	$(CC) -o util/mkversion util/mkversion.c
	./util/mkversion manifest.uuid manifest Makefile >src/version.h

# Debug build

COMPILE_DEBUG       = $(CC) -c $(CFLAGS) $(DEBUGFLAGS) -o $@
DEBUGFLAGS          = -O0 -g -DDEBUG
DEBUGFLAGS         += -DDECPRINT=1 -DDECCHECK=1 -DDECALLOC=1

.PHONY: debug
debug: $(DEBUGDIR) $(OBJDIR_DEBUG) $(DEBUG_LIBV) $(DEBUG_DYLIBV)

$(DEBUGDIR):
	mkdir -p $(DEBUGDIR)

$(OBJDIR_DEBUG):
	mkdir -p $(OBJDIR_DEBUG)

$(DEBUG_LIBV): $(DEBUGOBJS)
	libtool -static -o $@ $(DEBUGOBJS)
	ln -f -s ./$(LIBV) $(DEBUG_LIB)

$(DEBUG_DYLIBV): $(DEBUGOBJS)
	$(CC) $(LDFLAGS) -o $@ $(DEBUGOBJS)
	ln -f -s ./$(DYLIBV) $(DEBUG_DYLIB)

$(OBJDIR_DEBUG)/decContext.o:
	$(COMPILE_DEBUG) src/decNumber/decContext.c

$(OBJDIR_DEBUG)/decNumber.o:
	$(COMPILE_DEBUG) src/decNumber/decNumber.c

$(OBJDIR_DEBUG)/decPacked.o:
	$(COMPILE_DEBUG) src/decNumber/decPacked.c

$(OBJDIR_DEBUG)/decimal.o:
	$(COMPILE_DEBUG) src/decimal.c

$(OBJDIR_DEBUG)/impl_decnumber.o:
	$(COMPILE_DEBUG) src/impl_decnumber.c

# Release build

COMPILE_RELEASE     = $(CC) -c $(CFLAGS) $(RELEASEFLAGS) -o $@
RELEASEFLAGS        = -Os -DNDEBUG
RELEASEFLAGS       += -DDECPRINT=0 -DDECCHECK=0 -DDECALLOC=0

.PHONY: release
release: $(RELEASEDIR) $(OBJDIR_RELEASE) $(RELEASE_LIBV) $(RELEASE_DYLIBV)

$(RELEASEDIR):
	mkdir -p $(RELEASEDIR)

$(OBJDIR_RELEASE):
	mkdir -p $(OBJDIR_RELEASE)

$(RELEASE_LIBV): $(RELEASEOBJS)
	libtool -static -o $@ $(RELEASEOBJS)
	ln -f -s ./$(LIBV) $(RELEASE_LIB)

$(RELEASE_DYLIBV): $(RELEASEOBJS)
	$(CC) $(LDFLAGS) -o $@ $(RELEASEOBJS)
	ln -f -s ./$(DYLIBV) $(RELEASE_DYLIB)

$(OBJDIR_RELEASE)/decContext.o:
	$(COMPILE_RELEASE) src/decNumber/decContext.c

$(OBJDIR_RELEASE)/decNumber.o:
	$(COMPILE_RELEASE) src/decNumber/decNumber.c

$(OBJDIR_RELEASE)/decPacked.o:
	$(COMPILE_RELEASE) src/decNumber/decPacked.c

$(OBJDIR_RELEASE)/decimal.o:
	$(COMPILE_RELEASE) src/decimal.c

$(OBJDIR_RELEASE)/impl_decnumber.o:
	$(COMPILE_RELEASE) src/impl_decnumber.c

# Other targets

TEST_BIN            = runtests
TEST_DEBUGOBJS      = $(OBJDIR_DEBUG)/sqlite3.o $(OBJDIR_DEBUG)/runtests.o
TEST_RELEASEOBJS    = $(OBJDIR_RELEASE)/sqlite3.o $(OBJDIR_RELEASE)/runtests.o

$(OBJDIR_DEBUG)/sqlite3.o $(OBJDIR_RELEASE)/sqlite3.o:   \
	src/sqlite/sqlite3.c

$(OBJDIR_DEBUG)/runtests.o $(OBJDIR_RELEASE)/runtests.o: \
	test/runtests.c test/mu_unit_sqlite.h test/mu_unit.h   \
	test/test_common.c test/test_decnumber_func.c          \
	test/test_decnumber_bench.c

.PHONY: test
test: testdebug testrelease

.PHONY: testdebug
testdebug: $(DEBUGDIR) $(OBJDIR_DEBUG) $(DEBUGDIR)/$(TEST_BIN)

$(DEBUGDIR)/$(TEST_BIN): $(TEST_DEBUGOBJS)
	$(CC) $(CFLAGS) $(DEBUGFLAGS) -I$(TESTDIR) -o $@ $(TEST_DEBUGOBJS)

$(OBJDIR_DEBUG)/sqlite3.o:
	$(COMPILE_DEBUG) $(SQLITE_OPTIONS) src/sqlite/sqlite3.c

$(OBJDIR_DEBUG)/runtests.o:
	$(COMPILE_DEBUG) test/runtests.c

.PHONY: testrelease
testrelease: $(RELEASEDIR) $(OBJDIR_RELEASE) $(RELEASEDIR)/$(TEST_BIN)

$(RELEASEDIR)/$(TEST_BIN): $(TEST_RELEASEOBJS)
	$(CC) $(CFLAGS) $(RELEASEFLAGS) -I$(TESTDIR) -o $@ $(TEST_RELEASEOBJS)

$(OBJDIR_RELEASE)/sqlite3.o:
	$(COMPILE_RELEASE) $(SQLITE_OPTIONS) -o $@ src/sqlite/sqlite3.c

$(OBJDIR_RELEASE)/runtests.o:
	$(COMPILE_RELEASE) test/runtests.c

.PHONY: doc
doc:
	doxygen

.PHONY: clean
clean:
	@$(RM) -r ./build

.PHONY: distclean
distclean: clean
	@$(RM) cscope.*
	@$(RM) -r html
	@$(RM) Session.vim
	@$(RM) tags
	@$(RM) util/mkversion

