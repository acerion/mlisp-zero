#pragma once

#include "mlisp-zero.h"


void dbg_object_fn(int indent, const char * label, const object * const o);
void dbg_show_fn(const char * label, const object * obj);
void dbg_obj_mem_fn(int indent, const object * obj);


#if DEBUG == 1
#define dbg_object(i, l, o) dbg_object_fn((i), (l), (o))
#define dbg_show(l, o) dbg_show_fn((l), (o))


// Show given object's/expression's memory layout, recursively
//
// The first object is printed as a root of tree. Child objects, referenced
// through car and cdr, are printed as elements nested under their parents.
//
// The parent/children tree is printed recursively until "()", i.e. empty
// list, child is found.
//
// Each child is nested under its parent.
#define dbg_obj_mem(i, o) dbg_obj_mem_fn((i), (o))


#else
#define dbg_object(i, l, o)
#define dbg_show(l, o)
#define dbg_obj_mem(i, o)
#endif

