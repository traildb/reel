.PHONY: all test clean build _build

RM ?= rm -f
MKDIR ?= mkdir -p

sources := reel_io.c reel_std.c reel_query.c reel_util.c
headers := reel.h reel_util.h
scripts := reel_compile reel

quoted := reel_io_c reel_std_c reel_query_c reel_util_c reel_h reel_util_h
quoted := $(addprefix _build/,$(addsuffix .quot,$(quoted)))


all: test


build: _build/reel_compile;

_build: $(sources) $(headers) $(scripts)
	$(RM) -rf _build
	$(MKDIR) _build

_build/reel_compile: reel_compile $(quoted) _build
	cp $< $@
	cat $(quoted) >> $@

_build/reel_io_c.quot:     reel_io.c     _build ;     ./python_quote _REEL_IO_C $< > $@

_build/reel_std_c.quot:    reel_std.c    _build ;    ./python_quote _REEL_STD_C $< > $@

_build/reel_query_c.quot:  reel_query.c  _build ;  ./python_quote _REEL_QUERY_C $< > $@

_build/reel_util_c.quot:   reel_util.c   _build ;   ./python_quote _REEL_UTIL_C $< > $@

_build/reel_h.quot:        reel.h        _build ;        ./python_quote _REEL_H $< > $@

_build/reel_util_h.quot:   reel_util.h   _build ;   ./python_quote _REEL_UTIL_H $< > $@


test: _test _test/reel_compile _test/test.rl _test/trails-bug.tdb.tar build;

_test: $(sources) $(headers)
	$(RM) -rf _test
	$(MKDIR) _test

_test/reel_compile: _build/reel_compile
	cp $< $@

_test/test.rl: test.rl
	cp $< $@

clean:
	$(RM) -rf ./_build ./_test
