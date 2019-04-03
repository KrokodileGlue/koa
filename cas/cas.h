#pragma once

#include <inttypes.h>
#include <stdbool.h>

struct sym {
	enum sym_type {
		SYM_NULL,
		SYM_NUM,
		SYM_RATIO,
		SYM_PRODUCT,
		SYM_CROSS,
		SYM_SUM,
		SYM_DIFFERENCE,
		SYM_VARIABLE,
		SYM_POWER,
		SYM_POLYNOMIAL,
		SYM_INDETERMINATE,
		SYM_SUBSCRIPT,
		SYM_EQUALITY,
		SYM_VECTOR,
		SYM_DERIVATIVE,
		SYM_CONSTANT,
		SYM_LOGARITHM,
		SYM_ABS,
		SYM_NEGATIVE
	} type;

	union {
		/* Binary operators. */
		struct {
			struct sym *a, *b, *c;
		};

		char *text;

		/* Floating-point representation. */
		struct {
			struct num *sig, *exp;
		};

		/* Polynomials. */
		struct {
			char *indeterminate;

			struct polynomial {
				struct sym *exponent;
				struct sym *coefficient;
				struct polynomial *next;
			} *polynomial;
		};

		/* Vectors. */
		struct {
			unsigned len;
			struct sym **vector;
		};

		/* Derivative. */
		struct {
			struct sym *deriv;
			char *wrt;
		};

		enum {
			CONST_E,
			CONST_PI
		} constant;
	};
};

typedef struct sym *sym;

struct sym_env {
	unsigned precision;

	/*
	 * The minimum priority for a step to be shown in the
	 * solution.
	 */
	enum priority {
		PRIO_ALGEBRA,
		PRIO_ANSWER
	} prio;

	/* The steps taken over the course of this session. */
	struct {
		sym result;
		char msg[1024];
	} *hist;

	unsigned hist_len,
		num_eq;

	/* The default coordinate system. */
	char *coords;
};

typedef struct sym_env *sym_env;

sym_env sym_env_new(enum priority prio, unsigned precision);
sym sym_new(sym_env e, enum sym_type type);
sym sym_add(sym_env e, const struct sym *a, const struct sym *b);
sym sym_sub(sym_env e, const struct sym *a, const struct sym *b);
sym sym_negate(sym_env e, const struct sym *s);
sym sym_copy(sym_env e, const struct sym *s);
sym sym_dec(sym_env e, const struct sym *s);
sym sym_pow(sym_env e, const struct sym *a, const struct sym *b);
sym sym_mul(sym_env e, const struct sym *a, const struct sym *b);
sym sym_div(sym_env e, const struct sym *a, const struct sym *b);
sym sym_gcf(sym_env e, const struct sym *a, const struct sym *b);
sym sym_sqrt(sym_env e, const struct sym *s);
sym sym_root(sym_env e, const struct sym *a, const struct sym *b);
int sym_cmp(sym_env e, const struct sym *a, const struct sym *b);
void sym_free(sym_env e, sym s);
void sym_print_history(sym_env env);
void sym_add_coefficient(sym_env e, sym poly, const struct sym *coefficient, const struct sym *exponent);
void sym_add_vec(sym_env e, sym vec, unsigned idx, const struct sym *x);
void sym_set_coords(sym_env e, const char *coords);
int sym_hist_add(enum priority prio,
                 sym_env e,
                 const struct sym *sym,
                 const char *fmt,
                 ...);
