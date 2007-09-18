#
# -*- Makefile -*-
#
#

BINDIR = ~/bin

all: kossmann-main ksldg nd

kossmann-main: kossmann-main.cpp
	gcc -o $@ $< -lstdc++

ksldg: ksldg.c
	gcc -std=c99 -Wall -o $@ $<

nd: nd.c
	gcc -std=c99 -Wall -o $@ $<

clean:
	rm -rf kossmann-main ksldg kossmann-main.exe ksldg.exe

install:
	make clean
	make
	install ksldg $(BINDIR)
	install pg_sldg.pl $(BINDIR)
