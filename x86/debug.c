#include <stdio.h>

#include "debug.h"
#include "mlisp-zero.h"


#define ptr_pri(x)  ((unsigned int) (((unsigned long) (x)) >> (8 * 4)) & 0xffff), \
                    ((unsigned int) (((unsigned long) (x)) >> (4 * 4)) & 0xffff), \
                    ((unsigned int) (((unsigned long) (x)) >> (0 * 4)) & 0xffff)
#define ptr_fmt "0x:%04x:%04x:%04x"


void dbg_object_fn(const char * label, const object * const o)
{
	// Example pointer representation on x86_64: 0x559562682250
	//
	// We have plenty of CPU time on x86, so we can use printf() with its
	// format string.

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

