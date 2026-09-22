all:
	gcc -O2 -std=c11 -Wall -Wextra -pedantic -Wundef -fanalyzer mlisp-zero.c -o mlisp-zero

clean:
	rm -rf mlisp-zero

