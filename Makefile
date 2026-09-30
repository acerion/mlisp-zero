all:
	gcc -O2 -std=c11 \
	-Wall -Wextra -pedantic -fanalyzer \
	-Wundef \
	-Wcast-qual -Wcast-align=strict \
	mlisp-zero.c -o mlisp-zero

clean:
	rm -rf mlisp-zero

