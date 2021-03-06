# Makefile

PREFIX=/usr/local
SHAREDIR=$(PREFIX)/share/c4
REV = $(shell cat ../REV)
CC = gcc

CFLAGS = -g -O2 -W -Wall -Wshadow -Wpointer-arith -Wsign-compare \
	-Wstrict-prototypes -I. -I.. -DLINUX

all: libcalc4.so

libcalc4.so: libcalc4.o
	$(CC) -fPIC -shared $^ -o $@
	@echo $^ from rev.$(REV)

libcalc4.o: libcalc4.c
	$(CC) $(CFLAGS) -fPIC -DSHAREDIR=\"$(SHAREDIR)\" -c -o $@ $<

mill.o: mill.c
	$(CC) $(CFLAGS) -fPIE -c -o $@ $^

mill: mill.o
	$(CC) -o $@ $^ -fPIE -pie -pthread -lm

selftest: selftest.c libcalc4.c
	test -f $(SHAREDIR)/prim32.dat
	$(CC) $(CFLAGS) -fPIC -DSHAREDIR=\"$(SHAREDIR)\" -c -o libcalc4.tmp.o libcalc4.c
	$(CC) $(CFLAGS) -c -o selftest.o selftest.c
	$(CC) -o selftest selftest.o libcalc4.tmp.o -lm

clean:
	-rm -f *~ *.o *.so mill selftest

install: libcalc4.so
	install -d -m 755  $(DESTDIR)$(PREFIX)/lib/
	install -m 444 -p libcalc4.so  $(DESTDIR)$(PREFIX)/lib/
	ldconfig
	install -d -m 755  $(DESTDIR)$(PREFIX)/include/
	install -m 444 -p calc4.h  $(DESTDIR)$(PREFIX)/include/
	@if test -f prim32.dat ; then \
	  install -d -m 755  $(SHAREDIR)/ ;\
	  install -m 444 -p prim32.dat  $(SHAREDIR)/ ;\
	else \
	  if test -f $(SHAREDIR)/prim32.dat ; then \
	    echo $(SHAREDIR)/prim32.dat exists ;\
	  else \
	    echo $(SHAREDIR)/prim32.dat does not exist ;\
	  fi \
	fi

uninstall:
	@if test -f $(DESTDIR)$(PREFIX)/lib/libcalc4.so ; then \
	  rm -f $(DESTDIR)$(PREFIX)/lib/libcalc4.so ;\
	fi
	@if test -f $(DESTDIR)$(PREFIX)/include/calc4.h ; then \
	  rm -f $(DESTDIR)$(PREFIX)/include/calc4.h ;\
	fi
	@if test -f $(SHAREDIR)/prim32.dat ; then \
	  echo $(SHAREDIR)/prim32.dat exists ;\
	else \
	  echo $(SHAREDIR)/prim32.dat does not exist ;\
	fi

