.PHONY: z80 # Don't treat existence of z80/ as having the target built

all: x86 z80

x86:
	mkdir -p build/x86/
	gcc -O2 -std=c11 \
	-Wall -Wextra -pedantic -fanalyzer \
	-Wundef -Wshadow -Wmisleading-indentation -Wwrite-strings -Wbad-function-cast -Wlogical-op \
	-Wuninitialized -Winit-self -Wimplicit-fallthrough -Warith-conversion -Wjump-misses-init \
	-Wduplicated-cond -Wduplicated-branches \
	-Wunused-macros -Wunused-result -Wunused \
	-Wcast-qual -Wcast-align=strict \
	mlisp-zero.c -o build/x86/mlisp-zero

z80:
	mkdir -p build/z80/
	sdcc -mz80 -o build/z80/ -c z80/crt0_x86.c
	sdcc -mz80 -o build/z80/ -c mlisp-zero.c
	sdcc -mz80 --no-std-crt0 -o build/z80/mlisp-zero.ihx build/z80/crt0_x86.rel build/z80/mlisp-zero.rel

clean:
	rm -rf mlisp-zero
	rm -rf build

