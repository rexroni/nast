# nast - not-as-simple terminal
# See LICENSE file for copyright and license details.
.POSIX:

include config.mk

SRC = nast.c x.c render.c
OBJ = $(SRC:.c=.o)

all: options nast wide

options:
	@echo nast build options:
	@echo "CFLAGS  = $(STCFLAGS)"
	@echo "LDFLAGS = $(STLDFLAGS)"
	@echo "CC      = $(CC)"

config.h:
	cp config.def.h config.h

.c.o:
	$(CC) $(STCFLAGS) -c $<

nast.o: config.h nast.h win.h
x.o: arg.h config.h nast.h win.h
render.o: config.h nast.h win.h

$(OBJ): config.h config.mk

# render: render.o nast.o x.o

nast: $(OBJ)
	$(CC) -o $@ $(OBJ) $(STLDFLAGS)

clean:
	rm -f nast $(OBJ) nast-$(VERSION).tar.gz wide

dist: clean
	mkdir -p nast-$(VERSION)
	cp -R FAQ LEGACY TODO LICENSE Makefile README config.mk\
		config.def.h nast.info nast.1 arg.h nast.h win.h $(SRC)\
		nast-$(VERSION)
	tar -cf - nast-$(VERSION) | gzip > nast-$(VERSION).tar.gz
	rm -rf nast-$(VERSION)

install: nast
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f nast $(DESTDIR)$(PREFIX)/bin
	chmod 755 $(DESTDIR)$(PREFIX)/bin/nast
	mkdir -p $(DESTDIR)$(MANPREFIX)/man1
	sed "s/VERSION/$(VERSION)/g" < nast.1 > $(DESTDIR)$(MANPREFIX)/man1/nast.1
	chmod 644 $(DESTDIR)$(MANPREFIX)/man1/nast.1
	tic -sx nast.info
	@echo Please see the README file regarding the terminfo entry of nast.

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/nast
	rm -f $(DESTDIR)$(MANPREFIX)/man1/nast.1

.PHONY: all options clean dist install uninstall

wide: wide.c
