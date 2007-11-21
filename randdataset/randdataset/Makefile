#
# -*- Makefile -*- tell emacs -- it's a Makefile ;)
#
#

BINDIR = ~/bin
CFLAGS = -Wall -g
CC = gcc

all: randdataset

randdataset: randdataset.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -rf randdataset

install:
	make clean
	make
	install sldg $(BINDIR)
	install pg_sldg $(BINDIR)
