#pragma once

#include "mlisp-zero.h"

void dbg_object_fn(const char * label, const object * const o);

#if DEBUG == 1
#define dbg_object(l, o) dbg_object_fn((l), (o));
#else
#define dbg_object(l, o)
#endif
