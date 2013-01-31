# AUTOMATICALLY GENERATED -- DO NOT EDIT
BINDIR = $(EPREFIX)/bin
CC = clang
DATADIR = $(DATAROOTDIR)
DATAROOTDIR = $(PREFIX)/share
DOCDIR = $(DATAROOTDIR)/doc/$(PACKAGE)
EPREFIX = $(PREFIX)
INCLUDEDIR = $(PREFIX)/include
INFODIR = $(DATAROOTDIR)/info
INSTALL ?= /usr/bin/install
LD = clang
LIBDIR = $(EPREFIX)/lib
LIBEXECDIR = $(EPREFIX)/libexec
LOCALEDIR = $(DATAROOTDIR)/locale
LOCALSTATEDIR = $(PREFIX)/var
MANDIR = $(DATAROOTDIR)/man
OLDINCLUDEDIR = /usr/include
PKGDATADIR = $(DATADIR)/$(PACKAGE)
PKGINCLUDEDIR = $(INCLUDEDIR)/$(PACKAGE)
PKGLIBDIR = $(LIBDIR)/$(PACKAGE)
PREFIX = /usr/local
SBINDIR = $(EPREFIX)/sbin
SHAREDSTATEDIR = $(PREFIX)/com
SYSCONFDIR = $(PREFIX)/etc


#
# Detect the canonical system type of the system we are building on
# (build) and the system the package runs on (host)
# 
BUILD_CPU=$(shell uname -m)
HOST_CPU=$(BUILD_CPU)
BUILD_VENDOR=unknown
HOST_VENDOR=$(BUILD_VENDOR)
BUILD_KERNEL=$(shell uname | tr '[A-Z]' '[a-z]')
HOST_KERNEL=$(BUILD_KERNEL)
BUILD_SYSTEM=gnu
HOST_SYSTEM=$(BUILD_SYSTEM)
BUILD_TYPE=$(BUILD_CPU)-$(BUILD_VENDOR)-$(BUILD_KERNEL)-$(BUILD_SYSTEM)
HOST_TYPE=$(HOST_CPU)-$(HOST_VENDOR)-$(HOST_KERNEL)-$(HOST_SYSTEM)

# Allow variables to be overridden via a ./configure script that outputs config.mk
# FIXME -- requires GNU Make
-include config.mk

default: all

all:  libBlocksRuntime.so libBlocksRuntime.a blktest

blktest: test/main.o
	$(LD)  -o blktest -L . -Wl,-rpath,. -L . $(LDFLAGS) test/main.o libBlocksRuntime.a $(LDADD)

check:  blktest
	./blktest

clean: 
	rm -f *.rpm
	rm -f libBlocksRuntime-0.3.tar.gz
	rm -f runtime.o
	rm -f data.o
	rm -f libBlocksRuntime.so
	rm -f libBlocksRuntime.a
	rm -f test/main.o
	rm -f blktest

config.h: 
	@echo "checking build system type... $(BUILD_TYPE)"
	@echo "checking host system type... $(HOST_TYPE)"
	@echo "/* AUTOMATICALLY GENERATED -- DO NOT EDIT */" > config.h.tmp
	@date > config.log
	@rm -f conftest.c conftest.o
	@echo "creating config.h"
	@mv config.h.tmp config.h

data.o: data.c GNUmakefile
	$(CC) -DHAVE_CONFIG_H -I. -DBlocksRuntime_EXPORTS -DHAVE_SYNC_BOOL_COMPARE_AND_SWAP_INT -DHAVE_SYNC_BOOL_COMPARE_AND_SWAP_LONG -std=c99 -Wall -Wextra -W -pedantic -Wno-unused-parameter -o data.o -fPIC -DPIC $(CFLAGS) -c data.c

dist: libBlocksRuntime-0.3.tar.gz

distclean: clean 
	rm -f GNUmakefile
	rm -f libBlocksRuntime-0.3.tar.gz
	rm -f config.h
	rm -f config.yaml
	rm -f rpm.spec

distdir: 
	umask 22 ; mkdir -p '$(distdir)/test'
	cp -RL runtime.c Block_private.h config.h GNUmakefile data.c Block.h configure configure.rb $(distdir)
	cp -RL test/main.c $(distdir)/test

install: 
	/usr/bin/test -e $(DESTDIR)$(LIBDIR) || $(INSTALL) -d -m 755 $(DESTDIR)$(LIBDIR)
	$(INSTALL) -m 0644 libBlocksRuntime.so $(DESTDIR)$(LIBDIR)/libBlocksRuntime.so.0.0
	/usr/bin/test -e $(DESTDIR)$(INCLUDEDIR) || $(INSTALL) -d -m 755 $(DESTDIR)$(INCLUDEDIR)
	$(INSTALL) -m 644 Block.h Block_private.h $(DESTDIR)$(INCLUDEDIR)
	rm -f $(DESTDIR)$(LIBDIR)/libBlocksRuntime.so
	ln -s libBlocksRuntime.so.0.0 $(DESTDIR)$(LIBDIR)/libBlocksRuntime.so
	rm -f $(DESTDIR)$(LIBDIR)/libBlocksRuntime.so.0
	ln -s libBlocksRuntime.so.0.0 $(DESTDIR)$(LIBDIR)/libBlocksRuntime.so.0

libBlocksRuntime-0.3.tar.gz: 
	rm -rf libBlocksRuntime-0.3
	mkdir libBlocksRuntime-0.3
	$(MAKE) distdir distdir=libBlocksRuntime-0.3
	rm -rf libBlocksRuntime-0.3.tar libBlocksRuntime-0.3.tar.gz
	tar cf libBlocksRuntime-0.3.tar libBlocksRuntime-0.3
	gzip libBlocksRuntime-0.3.tar
	rm -rf libBlocksRuntime-0.3

libBlocksRuntime.a: runtime.o data.o
ifneq ($(DISABLE_STATIC),1)
	$(AR) cru libBlocksRuntime.a runtime.o data.o
	$(RANLIB) libBlocksRuntime.a
endif

libBlocksRuntime.so: runtime.o data.o
	$(LD)  -o libBlocksRuntime.so -shared -fPIC -L . -Wl,-soname,libBlocksRuntime.so.0 $(LDFLAGS) runtime.o data.o $(LDADD)

package: clean libBlocksRuntime-0.3.tar.gz
	rm -rf rpm *.rpm
	mkdir -p rpm/BUILD rpm/RPMS rpm/SOURCES rpm/SPECS rpm/SRPMS
	mkdir -p rpm/RPMS/i386 rpm/RPMS/x86_64
	cp libBlocksRuntime-0.3.tar.gz rpm/SOURCES
	rpmbuild --define "_topdir `pwd`/rpm" -bs rpm.spec
	cp rpm.spec rpm/SPECS/rpm.spec
	rpmbuild --define "_topdir `pwd`/rpm" -bb ./rpm/SPECS/rpm.spec
	mv ./rpm/SRPMS/* ./rpm/RPMS/*/*.rpm .
	rm -rf rpm

runtime.o: runtime.c Block_private.h config.h GNUmakefile
	$(CC) -DHAVE_CONFIG_H -I. -DBlocksRuntime_EXPORTS -DHAVE_SYNC_BOOL_COMPARE_AND_SWAP_INT -DHAVE_SYNC_BOOL_COMPARE_AND_SWAP_LONG -std=c99 -Wall -Wextra -W -pedantic -Wno-unused-parameter -o runtime.o -fPIC -DPIC $(CFLAGS) -c runtime.c

test/main.o: test/main.c Block.h GNUmakefile
	$(CC) -DHAVE_CONFIG_H -I. -I. -fblocks -g -O0 -o test/main.o $(CFLAGS) -c test/main.c

uninstall: 
	rm -f $(DESTDIR)$(LIBDIR)/libBlocksRuntime.so
	rm -f $(DESTDIR)$(INCLUDEDIR)/Block.h $(DESTDIR)$(INCLUDEDIR)/Block_private.h
