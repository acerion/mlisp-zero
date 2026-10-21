.PHONY: z80 # Don't treat existence of z80/ as having the target built



all: x86 z80



X86_CFLAGS = -O1 -ggdb -std=c11 \
	-Wall -Wextra -pedantic -fanalyzer \
	-Wundef -Wshadow -Wmisleading-indentation -Wwrite-strings -Wbad-function-cast -Wlogical-op \
	-Wuninitialized -Winit-self -Wimplicit-fallthrough -Warith-conversion -Wjump-misses-init \
	-Wduplicated-cond -Wduplicated-branches \
	-Wunused-macros -Wunused-result -Wunused \
	-Wcast-qual -Wcast-align=strict

X86_LDFLAGS =

x86:
	mkdir -p build/x86/
	gcc $(X86_CFLAGS) -c mlisp-zero.c -o build/x86/mlisp-zero.o
	gcc $(X86_LDFLAGS) build/x86/*.o  -o build/x86/mlisp-zero



# Purpose of this target is only to use platform-specific compiler with
# platform-specific flags to confirm that the compiler doesn't complain too
# much.
#
# The target uses very basic crt0 that doesn't match your platform.
#
# Final development and compilation for your platform should use crt0 and
# other files specific to your platform.
Z80_CFLAGS = -mz80
Z80_LDFLAGS = -mz80 --no-std-crt0

z80:
	mkdir -p build/z80/
	sdcc $(Z80_CFLAGS) -c z80/crt0_x86.c -o build/z80/
	sdcc $(Z80_CFLAGS) -c mlisp-zero.c   -o build/z80/
	sdcc $(Z80_LDFLAGS) build/z80/*.rel  -o build/z80/mlisp-zero.ihx



clean:
	rm -rf build
