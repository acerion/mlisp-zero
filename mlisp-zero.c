/*
  mlisp-zero
  https://github.com/acerion/mlisp-zero

  Licensed under the MIT license: https://opensource.org/licenses/MIT

  Original copyright notice:
        uLisp Zero 1.1 - www.ulisp.com
        David Johnson-Davies - www.technoblogy.com - 18th May 2017
        Licensed under the MIT license: https://opensource.org/licenses/MIT
*/

#include <ctype.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// C Macros

#define nil                NULL   // uLisp's "()"
#define elist              nil    // Scheme's "empty list". Defined to be "nil" only until "nil" is gone. See also "#define ELIST"
#define car(x)             ((x)->car)
#define cdr(x)             ((x)->cdr)

#define first(x)           ((x)->car)
#define second(x)          (car(cdr(x)))
#define cddr(x)            (cdr(cdr(x)))
#define third(x)           (car(cdr(cdr(x))))

#define push(x, y)         ((y) = cons((x),(y)))
#define pop(y)             ((y) = cdr(y))

#define symbolp(x)         ((x)->type == SYMBOL)

#define mark(x)            (car(x) = (object *)(((uintptr_t)(car(x))) | MARKBIT))
#define unmark(x)          (car(x) = (object *)(((uintptr_t)(car(x))) & ~MARKBIT))
#define marked(x)          ((((uintptr_t)(car(x))) & MARKBIT) != 0)
#define MARKBIT            1

#define PROGMEM // Empty definition

#ifdef SDCC
int putchar(int c) { return c; }
int getchar(void) { return 0; }
void exit(int x)
{
	(void) x;
	while (1);
}
#endif

// Constants

enum type { ZERO=0, SYMBOL=2, PAIR=4 };  // PAIR must be last
enum token { UNUSED, BRA, KET, QUO, DOT };

enum function {
	SYMBOLS,
	ELIST,  // (), empty list
	TEE,    // #t, #true
	EFF,    // #f, #false
	LAMBDA, SPECIAL_FORMS, QUOTE, DEFINE, SETQ, IF, FUNCTIONS, NOT,
NULLFN, CONS, ATOM, LISTP, CONSP, SYMBOLP, EQ, CAR, CDR, EVAL, GLOBALS, LOCALS, ENDFUNCTIONS };

// Typedefs

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

typedef object *(*fn_ptr_type)(object *, object *);

typedef struct {
  const char * string;
  fn_ptr_type fptr;
  int min;
  int max;
} tbl_entry_t;

// Workspace - sizes in bytes
#define WORDALIGNED
#define BUFFERSIZE 18
#define WORKSPACESIZE 320            /* Cells (4*bytes) */

object Workspace[WORKSPACESIZE] WORDALIGNED;
char Buffer[BUFFERSIZE];

// Global variables

jmp_buf exception;
unsigned int Freespace = 0;
char ReturnFlag = 0;
object *Freelist;
//extern uint8_t _end;

object *GlobalEnv;
object *GCStack = NULL;
char LastChar = 0;
char LastPrint = 0;
volatile char Escape = 0;

void error(const char * string);
object *eval(object *form, object *env);
const char * lookupbuiltin(symbol_t name);
void pchar(char c);
void pfl();
void pfstring(const char * s);
void pln();
void printobject(object *form);
object *read();

// Forward references
object * tee;
object * eff;

// Debugging

void dbg_show(const char * label, object * obj)
{
	(void) label;
	(void) obj;
#if 1
	printf("%s: ", label);
	printobject(obj);
	printf("\n");
#endif
}

// Set up workspace

void initworkspace () {
  Freelist = NULL;
  for (int i=WORKSPACESIZE-1; i>=0; i--) {
    object *obj = &Workspace[i];
    car(obj) = NULL;
    cdr(obj) = Freelist;
    Freelist = obj;
    Freespace++;
  }
}

object *myalloc () {
  if (Freespace == 0) error("No room");
  object *temp = Freelist;
  Freelist = cdr(Freelist);
  Freespace--;
  return temp;
}

inline void myfree (object *obj) {
  car(obj) = NULL;
  cdr(obj) = Freelist;
  Freelist = obj;
  Freespace++;
}

// Make each type of object

object *cons (object *arg1, object *arg2) {
  object *ptr = myalloc();
  ptr->car = arg1;
  ptr->cdr = arg2;
  return ptr;
}

object *symbol (symbol_t name) {
  object *ptr = myalloc();
  ptr->type = SYMBOL;
  ptr->name = name;
  return ptr;
}

// Garbage collection

void markobject (object *obj) {
  MARK:
  if (obj == NULL) return;
  if (marked(obj)) return;

  object* arg = car(obj);
  unsigned int type = obj->type;
  mark(obj);
  
  if (type >= PAIR || type == ZERO) { // cons
    markobject(arg);
    obj = cdr(obj);
    goto MARK;
  }
}

void sweep () {
  Freelist = NULL;
  Freespace = 0;
  for (int i=WORKSPACESIZE-1; i>=0; i--) {
    object *obj = &Workspace[i];
    if (!marked(obj)) myfree(obj); else unmark(obj);
  }
}

void gc (object *form, object *env) {
  markobject(tee);
  markobject(eff);
  markobject(GlobalEnv);
  markobject(GCStack);
  markobject(form);
  markobject(env);
  sweep();
}

// Error handling

void error (const char * string) {
  pfl(); pfstring("Error: ");
  pfstring(string); pln();
  GCStack = NULL;
  longjmp(exception, 1);
}

void error2 (object *symbol, const char * string) {
  pfl(); pfstring("Error: ");
  if (symbol == NULL) pfstring("function ");
  else { pchar('\''); printobject(symbol); pfstring("' "); }
  pfstring(string); pln();
  GCStack = NULL;
  longjmp(exception, 1);
}

// Helper functions

bool consp (object *x) {
  if (x == NULL) return false;
  unsigned int type = x->type;
  return type >= PAIR || type == ZERO;
}

bool atom (object *x) {
  if (x == NULL) return true;
  unsigned int type = x->type;
  return type < PAIR && type != ZERO;
}

bool listp (object *x) {
  if (x == NULL) return true;
  unsigned int type = x->type;
  return type >= PAIR || type == ZERO;
}

int toradix40 (char ch) {
  if (ch == 0) return 0;
  if (ch >= '0' && ch <= '9') return ch-'0'+30;
  ch = ch | 0x20;
  if (ch >= 'a' && ch <= 'z') return ch-'a'+1;
  return -1; // Invalid
}

int fromradix40 (int n) {
  if (n >= 1 && n <= 26) return 'a'+n-1;
  if (n >= 30 && n <= 39) return '0'+n-30;
  return 0;
}

int pack40 (char *buffer) {
  return (((toradix40(buffer[0]) * 40) + toradix40(buffer[1])) * 40 + toradix40(buffer[2]));
}

bool valid40 (char *buffer) {
 return (toradix40(buffer[0]) >= 0 && toradix40(buffer[1]) >= 0 && toradix40(buffer[2]) >= 0);
}

const char *name (object *obj) {
  if(!symbolp(obj)) error("Error in name");
  symbol_t x = obj->name;
  if (x < ENDFUNCTIONS) return lookupbuiltin(x);
  Buffer[3] = '\0';
  for (int n=2; n>=0; n--) {
    Buffer[n] = fromradix40(x % 40);
    x = x / 40;
  }
  return Buffer;
}

int issymbol (object *obj, symbol_t n) {
  return symbolp(obj) && obj->name == n;
}

int eq (object *arg1, object *arg2) {
  int same_object = (arg1 == arg2);
  int same_symbol = (symbolp(arg1) && symbolp(arg2) && arg1->name == arg2->name);
  return (same_object || same_symbol);
}

object *progn (object *args, object *env) {
  if (args == NULL) return nil;
  object *more = cdr(args);
  while (more != NULL) {
  eval(car(args), env);
    args = more;
    more = cdr(args);
  }
  return car(args);
}

// Lookup variable in environment

object *value (symbol_t n, object *env) {
  while (env != NULL) {
    object *pair = car(env);
    if (pair != NULL && car(pair)->name == n) return pair;
    env = cdr(env);
  }
  return elist; // Variable not found.
}

object *findvalue (object *var, object *env) {
  symbol_t varname = var->name;
  object * pair = value(varname, env);
  if (pair == elist) pair = value(varname, GlobalEnv);
  if (pair == elist) error2(var,"unknown variable");
  return pair;
}

object *findtwin (object *var, object *env) {
  while (env != NULL) {
    object *pair = car(env);
    if (pair != NULL && car(pair) == var) return pair;
    env = cdr(env);
  }
  return NULL;
}

void dropframe (int tc, object **env) {
  if (tc) {
    while (*env != NULL && car(*env) != NULL) {
      pop(*env);
    }
  } else {
    push(nil, *env);
  }
}

// Handling closures
  
object *closure (object *fname, object *function, object *args, object **env) {
  object *params = first(function);
  function = cdr(function);
  // Add arguments to environment
  while (params != NULL && args != NULL) {
    object *value;
    object *var = first(params);
    value = first(args);
    args = cdr(args);
    push(cons(var,value), *env);
    params = cdr(params);
  }
  if (params != NULL) error2(fname, "has too few parameters");
  if (args != NULL) error2(fname, "has too many parameters");
  // Do an implicit progn
  return progn(function, *env);
}

// Checked car and cdr

inline object *carx (object *arg) {
  if (!listp(arg)) error("Can't take car");
  if (arg == nil) return nil;
  return car(arg);
}

inline object *cdrx (object *arg) {
  if (!listp(arg)) error("Can't take cdr");
  if (arg == nil) return nil;
  return cdr(arg);
}

// Special forms

object *sp_quote (object *args, object *env) {
  (void) env;
  return first(args);
}

object * sp_define(object * args, object * env)
{
	(void) env;

	object * head = car(args); // First object after "define".
	dbg_show("== args", args);
	dbg_show("== head", head);

	if (symbolp(head)) {

		// FIXME (acerion) 2026.09.29: uLisp originally used
		// sp_defvar() with hardcoded limits on min/max count of args
		// to defvar (min=2/max=2). Current code allows us to write
		// (define x 'a 'b 'c ...) - there is no validation that
		// "(define variable expression)" is built with only one
		// 'variable' and only one 'expression'.

		// R7RS-small, chapter 5.3, form 1.
		object * variable = head;
		object * expression = second(args); // 'second' returns non-list.

		dbg_show("== variable", variable);
		dbg_show("== expression", expression);

		// Existing or new object with name == variable and with
		// value set to result of evaluation.
		object * val = eval(expression, env);
		object * pair = value(variable->name, GlobalEnv);
		if (pair != elist) {
			// Update existing object.
			cdr(pair) = val;
		} else {
			// Crate new object.
			push(cons(variable, val), GlobalEnv);
		}
		return variable;

	} else if (consp(head)) {
		// R7RS-small, chapter 5.3, form 2 and 3.
		object * variable = car(head);  // Function's name
		object * formals = cdr(head);   // Function's formal arguments
		object * body = cdr(args);      // Function's body - the tail of "((f x) . (<some operations on x>))"

		object * formals_list =
			listp(formals) ?
			formals :               // To properly represent formals coming from form "(define (f x) <body>)"
			(cons(formals, elist)); // To properly represent formals coming from form "(define (f . x) <body>)"

		dbg_show("== function name", variable);
		dbg_show("== function args", formals_list);
		dbg_show("== function body", body);

		// Existing or new object with name == variable and with
		// value set to lambda.
		object * val = cons(symbol(LAMBDA), cons(formals_list, body));
		object * pair = value(variable->name, GlobalEnv);
		if (pair != elist) {
			// Update existing object.
			cdr(pair) = val;
		} else {
			// Crate new object.
			push(cons(variable, val), GlobalEnv);
		}
		return head;
	} else {
		error2(head, "is neither symbol nor list");
		return elist; // What we return after error2()'s longjmp() call doesn't really matter.
	}
}

object *sp_setq (object *args, object *env) {
  object *arg = eval(second(args), env);
  object *pair = findvalue(first(args), env);
  cdr(pair) = arg;
  return arg;
}

object *sp_if (object *args, object *env) {
  if (eval(first(args), env) != nil) return eval(second(args), env);
  return eval(third(args), env);
}

// Core functions

object *fn_not (object *args, object *env) {
  (void) env;
  return (first(args) == nil) ? tee : nil;
}

object *fn_cons (object *args, object *env) {
  (void) env;
  return cons(first(args),second(args));
}

object *fn_atom (object *args, object *env) {
  (void) env;
  return atom(first(args)) ? tee : nil;
}

object *fn_listp (object *args, object *env) {
  (void) env;
  return listp(first(args)) ? tee : nil;
}

object *fn_consp (object *args, object *env) {
  (void) env;
  return consp(first(args)) ? tee : nil;
}

object *fn_symbolp (object *args, object *env) {
  (void) env;
  return symbolp(first(args)) ? tee : nil;
}

object *fn_eq (object *args, object *env) {
  (void) env;
  return eq(first(args), second(args)) ? tee : nil;
}

// List functions

object *fn_car (object *args, object *env) {
  (void) env;
  return carx(first(args));
}

object *fn_cdr (object *args, object *env) {
  (void) env;
  return cdrx(first(args));
}

// System functions

object *fn_eval (object *args, object *env) {
  return eval(first(args), env);
}

object *fn_globals (object *args, object *env) {
  (void) args, (void) env;
  return GlobalEnv;
}

object *fn_locals (object *args, object *env) {
  (void) args;
  return env;
}

// Insert your own function definitions here

// Built-in procedure names - stored in PROGMEM

const char string0[] PROGMEM = "symbols";
const char string3[] PROGMEM = "lambda";
const char string4[] PROGMEM = "special_forms";
const char string5[] PROGMEM = "quote";
const char string8[] PROGMEM = "setq";
const char string9[] PROGMEM = "if";
const char string10[] PROGMEM = "functions";
const char string11[] PROGMEM = "not";
const char string12[] PROGMEM = "null";
const char string13[] PROGMEM = "cons";
const char string14[] PROGMEM = "atom";
const char string15[] PROGMEM = "listp";
const char string16[] PROGMEM = "consp";
const char string17[] PROGMEM = "symbolp";
const char string18[] PROGMEM = "eq";
const char string19[] PROGMEM = "car";
const char string20[] PROGMEM = "cdr";
const char string21[] PROGMEM = "eval";
const char string22[] PROGMEM = "globals";
const char string23[] PROGMEM = "locals";

const tbl_entry_t lookup_table[] PROGMEM = {
  { string0, NULL, 0, 0 },
  // TODO (acerion) 2026.10.03: I'm not 100% sure if empty list deserves its
  // own entry in this table.
  { "()",       NULL, 0, 0 }, // elist
  { "#t",       NULL, 1, 0 }, // tee
  { "#f",       NULL, 1, 0 }, // eff
  { string3, NULL, 0, 127 },
  { string4, NULL, 0, 0 },
  { string5, sp_quote, 1, 1 },
  { "define",   sp_define, 0, 127 },
  { string8, sp_setq, 2, 2 },
  { string9, sp_if, 2, 3 },
  { string10, NULL, 0, 0 },
  { string11, fn_not, 1, 1 },
  { string12, fn_not, 1, 1 },
  { string13, fn_cons, 2, 2 },
  { string14, fn_atom, 1, 1 },
  { string15, fn_listp, 1, 1 },
  { string16, fn_consp, 1, 1 },
  { string17, fn_symbolp, 1, 1 },
  { string18, fn_eq, 2, 2 },
  { string19, fn_car, 1, 1 },
  { string20, fn_cdr, 1, 1 },
  { string21, fn_eval, 1, 1 },
  { string22, fn_globals, 0, 0 },
  { string23, fn_locals, 0, 0 },
};

// Table lookup functions

// Get index to lookup_table[] at which built in symbol, special form or
// function with given name is placed. Return ENDFUNCTIONS if not found.
int builtin(const char * str)
{
	int entry = 0;
	while (entry < ENDFUNCTIONS) {
		if (strcmp(str, lookup_table[entry].string) == 0) {
			return entry;
		}
		entry++;
	}

	return ENDFUNCTIONS;
}

fn_ptr_type lookupfn(symbol_t name)
{
	return lookup_table[name].fptr;
}

int lookupmin(symbol_t name)
{
	return lookup_table[name].min;
}

int lookupmax(symbol_t name)
{
	return lookup_table[name].max;
}

// Index of symbol in lookup_table -> symbol's name.
const char * lookupbuiltin(symbol_t name)
{
	strcpy(Buffer, lookup_table[name].string);
	return Buffer;
}

// Main evaluator

object *eval (object *form, object *env) {
  // Enough space?
  if (Freespace < 20) gc(form, env);
  // Escape
  if (Escape) { Escape = 0; error("Escape!");}

  // Empty list evaluates to itself.
  if (form == elist) {
    dbg_show("== empty-list", form);
    return elist;
  }

  // The "(define (f x) ...)" form defines an "f" symbol.
  // The "(define x ...)" form defines an "x" symbol.
  // Also undefined objects will he handled here.
  if (symbolp(form)) {
    const symbol_t name = form->name;
    if (name == ELIST) {
      dbg_show("== symbol (elist)", form);
      return elist;
    }

    // Find symbol's value in some environment.
    const object * pair = value(name, env);
    if (pair != elist) {
      dbg_show("== symbol (in local env)", form);
      return cdr(pair);
    }
    pair = value(name, GlobalEnv);
    if (pair != elist) {
      dbg_show("== symbol (in global env)", form);
      return cdr(pair);
    }

    // Is the symbol a built-in symbol/function?
    if (name <= ENDFUNCTIONS) {
      dbg_show("== symbol (built in)", form);
      return form;
    }
    error2(form, "undefined");
  }

  // It's a list
  object *function = car(form);
  object *args = cdr(form);

  // List starts with a symbol?
  if (symbolp(function)) {
    symbol_t name = function->name;

    if (name == LAMBDA) {
      if (env == NULL) return form;
      error("closures not supported");
    }
    
    if ((name > SPECIAL_FORMS) && (name < FUNCTIONS)) {
      return (lookupfn(name))(args, env);
    }
  }
        
  // Evaluate the parameters - result in head
  object *fname = car(form);
  object *head = cons(eval(car(form), env), elist);
  push(head, GCStack); // Don't GC the result list
  object *tail = head;
  form = cdr(form);
  int nargs = 0;

  while (form != NULL) {
    object *obj = cons(eval(car(form),env), elist);
    cdr(tail) = obj;
    tail = obj;
    form = cdr(form);
    nargs++;
  }
    
  function = car(head);
  args = cdr(head);
 
  if (symbolp(function)) {
    symbol_t name = function->name;
    if (name >= ENDFUNCTIONS) error2(fname, "is not valid here");
    if (nargs<lookupmin(name)) error2(fname, "has too few arguments");
    if (nargs>lookupmax(name)) error2(fname, "has too many arguments");
    object *result = (lookupfn(name))(args, env);
    pop(GCStack);
    return result;
  }
      
  if (listp(function) && issymbol(car(function), LAMBDA)) {
    dropframe(0, &env);
    form = closure(fname, cdr(function), args, &env);
    pop(GCStack);
    return eval(form, env);
  } 
  
  error2(fname, "is an illegal function");
  return elist; // What we return after error2()'s longjmp() call doesn't really matter.
}

// Print functions

void pchar (char c) {
  LastPrint = c;
  putchar(c);
  if (c == '\r') putchar('\n');
}

void pstring(const char * s)
{
	while (*s) {
		pchar(*s++);
	}
}

// Originally pfstring function was for PSTR strings that Arduino-supported
// platforms stored in FLASH instead of RAM-located strings. The other
// function, pstring, was for RAM-located strings.
//
// asm code generated by sdcc for z80 tells me that the "const char *"
// strings are put in this area:
//
//      .area _CODE            <---- "_CODE" area, everything under it is in ROM.
// [...]
// ___str_55:
//      .ascii "hello"
//      .db 0x00
//
// And that means that they are in ROM. From the point of view of C code we
// can use the same implementation for pstring and pfstring.
void pfstring(const char * s)
{
	while (*s) {
		pchar(*s++);
	}
}

void pint (int i) {
  int lead = 0;
  if (i<0) pchar('-');
  for (int d=10000; d>0; d=d/10) {
    int j = i/d;
    if (j!=0 || lead || d==1) { pchar(abs(j)+'0'); lead=1;}
    i = i - j*d;
  }
}

void pln () {
  pchar('\r');
}

void pfl () {
  if (LastPrint != '\r') pchar('\r');
}

void printobject(object *form){
  if (form == elist) {
    pfstring("()");
  } else if (listp(form)) {
    pchar('(');
    printobject(car(form));
    form = cdr(form);
    while (form != NULL && listp(form)) {
      pchar(' ');
      printobject(car(form));
      form = cdr(form);
    }
    if (form != NULL) {
      pfstring(" . ");
      printobject(form);
    }
    pchar(')');
  } else if (symbolp(form)) {
    pstring(name(form));
  } else
    error("Error in print.");
}

int gchar () {
  if (LastChar) { 
    char temp = LastChar;
    LastChar = 0;
    return temp;
  }
  char temp = getchar();
  if (temp != '\r') pchar(temp);
  return temp;
}

object *nextitem() {
  int ch = gchar();
  while(isspace(ch)) ch = gchar();

  if (ch == ';') {
    while(ch != '(') ch = gchar();
    ch = '(';
  }
  if (ch == '\r') ch = gchar();
  if (ch == EOF) exit(0);

  if (ch == ')') return (object *)KET;
  if (ch == '(') return (object *)BRA;
  if (ch == '\'') return (object *)QUO;
  if (ch == '.') return (object *)DOT;
  
  // Parse variable
  int index = 0;
  Buffer[2] = '\0'; // In case variable is one letter

  while(!isspace(ch) && ch != ')' && ch != '(' && index < BUFFERSIZE-1) {
    Buffer[index++] = ch;
    ch = gchar();
  }

  Buffer[index] = '\0';
  if (ch == ')') LastChar = ')';
  if (ch == '(') LastChar = '(';

  int x = builtin(Buffer);
  if (x == ELIST) {
    dbg_show("== nextitem(): empty-list", elist);
    return elist;
  }
  if (x < ENDFUNCTIONS) return symbol(x);
  else if (index < 4 && valid40(Buffer)) return symbol(pack40(Buffer));
  error("Illegal symbol");
  return elist; // What we return after error()'s longjmp() call doesn't really matter.
}

object *readrest() {
  object *item = nextitem();

  if(item == (object *)KET) return NULL;
  
  if(item == (object *)DOT) {
    object *arg1 = read();
    if (readrest() != NULL) error("Malformed list");
    return arg1;
  }

  if(item == (object *)QUO) {
    object *arg1 = read();
    return cons(cons(symbol(QUOTE), cons(arg1, elist)), readrest());
  }
   
  if(item == (object *)BRA) item = readrest(); 
  return cons(item, readrest());
}

object *read() {
  object *item = nextitem();
  if (item == (object *)BRA) return readrest();
  if (item == (object *)DOT) return read();
  if (item == (object *)QUO) return cons(symbol(QUOTE), cons(read(), elist));
  return item;
}

// Setup

void initenv() {
  GlobalEnv = NULL;
  tee = symbol(TEE);
  eff = symbol(EFF);
}

void setup() {
  initworkspace();
  initenv();
  pfstring("mlisp-zero v1.0"); pln();
}

// Read/Evaluate/Print loop

void repl(object *env) {
  for (;;) {
    gc(NULL, env);
    pint(Freespace);
    pfstring("> ");
    object *line = read();
    if (line == (object *)KET) error("Unmatched right bracket");
    push(line, GCStack);
    pfl();
    line = eval(line, env);
    pfl();
    printobject(line);
    pop(GCStack);
    pfl();
    pln();
  }
}

int main(void)
{
	setup();
	setjmp(exception);
	repl(NULL);

	return 0;
}

