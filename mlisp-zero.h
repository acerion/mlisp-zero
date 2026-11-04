#pragma once

enum type {
	ZERO=0,
	T_BOOLEAN    = 2,
	T_SYMBOL     = 4,
	T_PAIR       = 8  // T_PAIR must be last
};

typedef unsigned int symbol_t;

typedef struct sobject {
  union {
    struct {
      struct sobject *car;
      struct sobject *cdr;
    };
    struct {
      unsigned int type;
      union {
        symbol_t name;
        int integer;
      };
    };
  };
} object;


void printobject(const object * form);

