all: x86 z80

x86:
	gcc -O2 -std=c11 \
	-Wall -Wextra -pedantic -fanalyzer \
	-Wundef \
	-Wcast-qual -Wcast-align=strict \
	mlisp-zero.c -o mlisp-zero

z80:
	mkdir -p z80_build
	sdcc -DSDCC -mz80 --no-std-crt0 -o z80_build/ mlisp-zero.c

clean:
	rm -rf mlisp-zero
	rm -rf z80_build/

