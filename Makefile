# See LICENSE for copyright and license details
.PHONY: all options clean install uninstall dist

#VERSION         := $(shell git tag -l | tail -1)
VERSION         := 1.1-dev
EXEC            := xplugd
OBJS            := $(EXEC).o exec.o input.o randr.o
MAN             := $(EXEC).1
PKG             := $(EXEC)-$(VERSION)
ARCHIVE         := $(PKG).tar.xz

PREFIX          ?= /usr/local
INSTALLDIR      := $(DESTDIR)$(PREFIX)

MANPREFIX       ?= $(PREFIX)/share/man
MANPREFIX       := $(DESTDIR)$(MANPREFIX)

CFLAGS          := -O2 -W -Wall -Wextra -pedantic -std=c99
CPPFLAGS        += -D_DEFAULT_SOURCE
CPPFLAGS        += -DVERSION=\"$(VERSION)\"

LDLIBS          := -lX11 -lXrandr -lXi

all: $(EXEC)

.c.o:
	@printf "  CC      $@\n"
	@$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

$(OBJS): $(EXEC).h Makefile

$(EXEC): $(OBJS)
	@printf "  LINK    $@\n"
	@$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

install: $(EXEC)
	@install -d $(INSTALLDIR)/bin
	@install -m 755 $(EXEC) $(INSTALLDIR)/bin
	@install -d $(MANPREFIX)/man1
	@install -m 644 $(MAN) $(MANPREFIX)/man1

uninstall: 
	@$(RM) $(INSTALLDIR)/bin/$(EXEC)
	@$(RM) $(MANPREFIX)/man1/$(MAN)

clean: 
	@$(RM) $(EXEC)

distclean: clean
	@$(RM) $(OBJS) *~ *.bak core

dist: 
	@echo "Building xz tarball of $(PKG) in parent dir ..."
	@git archive --prefix=$(PKG)/ v$(VERSION) | xz > $(ARCHIVE)
	@md5sum $(ARCHIVE) | tee $(ARCHIVE).md5
	@mv -i $(ARCHIVE)* ../
