// crt0 suitable for edit/compile cycles during development of mlisp-zero on
// x86 platform, while x86 platform is the main (and only) actively tested
// target.
//
// Once I start deploying the program to actual target z80 system, then I
// will have to come up with a different (proper) implementation of crt0.

int putchar(int c)
{
	return c;
}

int getchar(void)
{
	return 0;
}

void exit(int x)
{
	(void) x;
	while (1) {
		;
	}
}
