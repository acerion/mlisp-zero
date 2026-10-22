#pragma once

enum type { ZERO=0, SYMBOL=2, PAIR=4 };  // PAIR must be last

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
