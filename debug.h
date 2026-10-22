#pragma once

#include "mlisp-zero.h"


void dbg_object_fn(const char * label, const object * const o);
void dbg_show_fn(const char * label, const object * obj);


#if DEBUG == 1
#define dbg_object(l, o) dbg_object_fn((l), (o));
#define dbg_show(l, o) dbg_show_fn((l), (o))
#else
#define dbg_object(l, o)
#define dbg_show(l, o)
#endif

