all: test simple

test: test.c
	gcc -Wl,--no-as-needed -o test `libftdi1-config --cflags` `libftdi1-config --libs` test.c

simple: simple.c
	gcc -Wl,--no-as-needed -o simple `libftdi1-config --cflags` `libftdi1-config --libs` simple.c
