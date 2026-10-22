#include <stdio.h>

#include "debug.h"
#include "mlisp-zero.h"


/*
  Debug utils for program running on z80. The code looks like a duplication
  of x86/debug.c, and to some degree it is. But the duplication allows for
  writing an implementation that is better adjusted to limitations of z80
  than a generic implementation.
*/


// Helper defines for handling Z80's 16-bit pointers in printf().
#define ptr_pri(x) \
	((unsigned int) (((unsigned long) (x)) >> (0 * 4)) & 0xffff)
#define ptr_fmt "0x:%04x"


void dbg_object_fn(const char * label, const object * const o)
{
	// TODO (acerion) 2026.10.21: optimize the code to avoid using costly
	// printf() with its format string handling.

	printf("%s: o: "ptr_fmt", ", label, ptr_pri(o));
	printf("s: { car = "ptr_fmt", cdr = "ptr_fmt" }, t = "ptr_fmt", ", ptr_pri(o->car), ptr_pri(o->cdr), ptr_pri(o->type));
	switch (o->type) {
	case ZERO:
		printf("{ name = "ptr_fmt"  ||  int = "ptr_fmt" } }\n", ptr_pri(o->name), ptr_pri(o->integer));
		break;
	case SYMBOL:
		printf("{ name = "ptr_fmt"  ||  int = "ptr_fmt" } }\n", ptr_pri(o->name), ptr_pri(o->integer));
		break;
	case PAIR:
		printf("{ name = "ptr_fmt"  ||  int = "ptr_fmt" } }\n", ptr_pri(o->name), ptr_pri(o->integer));
		break;
	default:
		printf("{ name = "ptr_fmt"  ||  int = "ptr_fmt" } }\n", ptr_pri(o->name), ptr_pri(o->integer));
		break;
	}
}


void dbg_show_fn(const char * label, const object * obj)
{
	(void) label;
	(void) obj;

	printf("%s: ", label);
	printobject(obj);
	printf("\n");
}

