#
# -*- Makefile -*- tell emacs -- it's a Makefile ;)
#
#

BINDIR = ~/bin

all: sldg

sldg: sldg.c
	gcc -Wall -o $@ $<

clean:
	rm -rf sldg 

install:
	make clean
	make
	install sldg $(BINDIR)
	install pg_sldg $(BINDIR)
