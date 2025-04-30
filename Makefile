PREFIX ?= /usr/
BINDIR ?= $(PREFIX)/bin
MANDIR ?= $(PREFIX)/share/man

CFLAGS ?= -O2 -Wall -Wextra
LDFLAGS ?= -lcrypt

all: lx

lx: lx.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

install: lx
	install -d $(DESTDIR)$(BINDIR)
	install -m 4755 -o root -g root lx $(DESTDIR)$(BINDIR)/
	install -d $(DESTDIR)/etc/
	[ -f $(DESTDIR)/etc/lx.conf ] || install -m 640 -o root -g root /dev/null $(DESTDIR)/etc/lx.conf

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/lx
	rm -f $(DESTDIR)/etc/lx.conf

clean:
	rm -f lx

.PHONY: all install uninstall clean
