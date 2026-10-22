# .PHONY: .x86 .z80 # Don't treat existence of a source directory as proof having the target built
DEBUG_FLAG ?= 0

# For people that grab the project from git forge and only have a gcc
# installed by default, without sdcc.
.DEFAULT_GOAL=.x86



# Alias for those with "make all" muscle memory.
all: .release

# A target that is a counterpart of "debug" build.
.release: .x86 .z80 .summary

debug: DEBUG_FLAG=1
debug: .x86_debug .x86 .z80_debug .z80 .summary


# For quick visual indication if target files have fresh timestamp, and how
# large they are (especially the files for small platforms).
.summary:
	@ls -la build/x86/mlisp-zero
	@ls -la build/z80/mlisp-zero.ihx



X86_CFLAGS = -O1 -ggdb -std=c11 \
	-I./ -I./x86/ -DDEBUG=$(DEBUG_FLAG) \
	-Wall -Wextra -pedantic -fanalyzer \
	-Wundef -Wshadow -Wmisleading-indentation -Wwrite-strings -Wbad-function-cast -Wlogical-op \
	-Wuninitialized -Winit-self -Wimplicit-fallthrough -Warith-conversion -Wjump-misses-init \
	-Wduplicated-cond -Wduplicated-branches \
	-Wunused-macros -Wunused-result -Wunused \
	-Wcast-qual -Wcast-align=strict

X86_LDFLAGS =

.x86:
	@mkdir -p build/x86/
	gcc $(X86_CFLAGS) -c mlisp-zero.c -o build/x86/mlisp-zero.o
	gcc $(X86_LDFLAGS) build/x86/*.o  -o build/x86/mlisp-zero
	@echo

.x86_debug:
	@mkdir -p build/x86/
	gcc $(X86_CFLAGS) -c x86/debug.c  -o build/x86/debug.o



# Purpose of this target is only to use platform-specific compiler with
# platform-specific flags to confirm that the compiler doesn't complain too
# much.
#
# The target uses very basic crt0 that doesn't match your platform.
#
# Final development and compilation for your platform should use crt0 and
# other files specific to your platform.
Z80_CFLAGS = -mz80 -I./ -I./z80/ -DDEBUG=$(DEBUG_FLAG)
Z80_LDFLAGS = -mz80 --no-std-crt0

.z80:
	@mkdir -p build/z80/
	sdcc $(Z80_CFLAGS) -c z80/crt0_x86.c   -o build/z80/crt0_x86.rel
	sdcc $(Z80_CFLAGS) -c mlisp-zero.c     -o build/z80/mlisp-zero.rel
	sdcc $(Z80_LDFLAGS) build/z80/*.rel    -o build/z80/mlisp-zero.ihx
	@echo

.z80_debug:
	@mkdir -p build/z80/
	sdcc $(Z80_CFLAGS) -c z80/debug.c      -o build/z80/debug.rel



clean:
	rm -rf build
	rm -rf compile_commands.json
	rm -rf ./cache/
