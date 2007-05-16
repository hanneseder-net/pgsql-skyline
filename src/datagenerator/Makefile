all: kossmann-main ksldg

kossmann-main: kossmann-main.cpp
	gcc -o $@ $< -lstdc++

ksldg: ksldg.c
	gcc -std=c99 -Wall -o $@ $<

clean:
	rm -rf kossmann-main ksldg kossmann-main.exe ksldg.exe
