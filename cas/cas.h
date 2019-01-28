#pragma once

#include <inttypes.h>
#include <stdbool.h>

#define DIGITS 256
#define BASE 256
typedef uint8_t num[DIGITS];

#define BUF_SIZE (DIGITS * sizeof (uint8_t))

struct sym {
	enum sym_type {
		SYM_INT,
		SYM_RATIO,
		SYM_PRODUCT,
		SYM_SUM,
		SYM_DIFFERENCE,
		SYM_VARIABLE,
		SYM_FLOAT
	} type;

	bool sign;

	union {
		struct {
			struct sym *a, *b;
		};

		num buf;
		char *variable;

		struct {
			num significand, exp;
		};
	};
};

typedef struct sym *sym;

sym sym_new(enum sym_type type);
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
sym sym_parse_latex(const char *s);
