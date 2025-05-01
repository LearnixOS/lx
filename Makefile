PREFIX ?= /usr/
BINDIR ?= $(PREFIX)/bin
MANDIR ?= $(PREFIX)/share/man

CFLAGS ?= -O2 -Wall -Wextra -pedantic
LDFLAGS ?= -lcrypt

OBJS = lx.o

PERSIST_MSG = "without persistence"

ifeq ($(PERSIST),1)
	CFLAGS += -DPERSIST=1
	OBJS += persist.o
	PERSIST_MSG = "with persistence enabled"
endif
ifeq ($(PERSIST), 0)
	CFLAGS += -DPERSIST=0
endif

all: lx

lx: $(OBJS)
	@echo "Linking lx $(PERSIST_MSG)... "
	$(CC) $(OBJS) $(LDFLAGS) -o lx

lx.o: lx.c persist.h
	@echo "Compiling lx.c $(PERSIST_MSG)..."
	$(CC) $(CFLAGS) -c lx.c -o lx.o

persist.o: persist.c persist.h
	@echo "Compiling persist.c..."
	$(CC) $(CFLAGS) -c persist.c -o persist.o

install: lx
	install -d $(DESTDIR)$(BINDIR)
	install -m 4755 -o root -g root lx $(DESTDIR)$(BINDIR)/
	install -d $(DESTDIR)/etc/
	[ -f $(DESTDIR)/etc/lx.conf ] || install -m 640 -o root -g root /dev/null $(DESTDIR)/etc/lx.conf

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/lx
	rm -f $(DESTDIR)/etc/lx.conf

clean:
	rm -f lx lx.o persist.o
	@echo "Cleaned build files."

.PHONY: all install uninstall clean
