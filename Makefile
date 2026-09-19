CC = gcc

CFLAGS = -Wall -Wextra -D_GNU_SOURCE -Ithirdparty
LDLIBS = $(shell pkg-config --cflags --libs notcurses libmpdclient) -lm

SRC = src/zait.c src/player.c src/image.c src/mpd.c src/tabs.c thirdparty/tomlc17.c src/config.c

OBJ = $(SRC:.c=.o)

all: zaitc

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

zaitc: $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDLIBS)

install: zaitc
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f zaitc $(DESTDIR)$(PREFIX)/bin
	chmod 755 $(DESTDIR)$(PREFIX)/bin/zaitc

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/zaitc

clean:
	rm -rf zaitc $(OBJ)

.PHONY: all install uninstall clean
