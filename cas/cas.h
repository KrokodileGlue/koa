#pragma once

#include <inttypes.h>
#include <stdbool.h>

//typedef uint8_t digit;

struct sym {
	enum sym_type {
		SYM_NUM,
		SYM_RATIO,
		SYM_PRODUCT,
		SYM_SUM,
		SYM_DIFFERENCE,
		SYM_VARIABLE,
	} type;

	bool sign;

	union {
		struct {
			struct sym *a, *b;
		};

		char *variable;

		struct {
			struct num *sig, *exp;
		};
	};
};

typedef struct sym *sym;

sym sym_new(enum sym_type type);
void sym_float(sym a, double c);
sym sym_add(const sym a, const sym b);
sym sym_sub(const sym a, const sym b);
sym sym_copy(const sym s);
sym sym_eval(const sym s);
sym sym_dec(const sym s);
sym sym_mul(sym a, sym b);
sym sym_div(const sym a, const sym b);
int sym_cmp(const sym a, const sym b);
void sym_print(const sym s);
sym sym_gcf(const sym a, const sym b);
sym sym_simplify(const sym s);
sym sym_sqrt(const sym s);
//sym sym_parse_latex(const char *s);
