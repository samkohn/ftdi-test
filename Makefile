all: test.c
	gcc -o out `libftdi1-config --libs` `libftdi1-config --cflags` test.c
