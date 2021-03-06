
# Makefile

PREFIX=/usr/local
SHAREDIR=$(PREFIX)/share/c4
BINARY = c4
REV = $(shell cat ../REV)
CC = gcc

CFLAGS = -g -O2 -W -Wall -fPIE -Wshadow -Wpointer-arith -Wsign-compare \
	-Wstrict-prototypes -I.. -I../lib

LDFLAGS = -fPIE -pie -rdynamic

all: $(BINARY)

# the new calculator, test version with local lib
$(BINARY): $(BINARY).o ../lib/libcalc4.so
	$(CC) -o $(BINARY) $(BINARY).o $(LDFLAGS) -Wl,-rpath -Wl,../lib \
		-lcalc4 -lm -lreadline
	@echo $(BINARY) from rev.$(REV)

clean:
	-rm -f *~ *.o core* $(BINARY)

# install, recompile with the installed libcalc4.so
install: $(BINARY).o
	$(CC) -o $(BINARY) $(BINARY).o $(LDFLAGS) -L$(PREFIX)/lib \
		-lcalc4 -lm -lreadline
	install -d -m 755 $(DESTDIR)$(PREFIX)/bin/
	install -m 555 -p $(BINARY) $(DESTDIR)$(PREFIX)/bin/
	@rm -f $(BINARY)
	@if test -f $(SHAREDIR)/prim32.dat ; then \
	  echo ok, $(SHAREDIR)/prim32.dat exists ; \
	else \
	  echo not ok, $(SHAREDIR)/prim32.dat does not exist ; \
	fi \

uninstall:
	if test -f $(DESTDIR)$(PREFIX)/bin/$(BINARY) ; then \
	  rm -f $(DESTDIR)$(PREFIX)/bin/$(BINARY) ;\
	fi

# ------------------------------------

.SUFFIXES: .o

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

# ------------------------------------
