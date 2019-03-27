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
		SYM_LIST
	} type;

	bool sign;

	union {
		/* Binary operators. */
		struct {
			struct sym *a, *b;
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
sym sym_new(sym_env env, enum sym_type type);
sym sym_add(sym_env env, const sym a, const sym b);
sym sym_sub(sym_env env, const sym a, const sym b);
sym sym_copy(sym_env env, const sym s);
sym sym_eval(sym_env e, sym s, enum priority prio);
sym sym_dec(sym_env env, const sym s);
sym sym_pow(sym_env env, const sym a, const sym b);
sym sym_mul(sym_env env, const sym a, const sym b);
sym sym_div(sym_env env, const sym a, const sym b);
sym sym_gcf(sym_env env, const sym a, const sym b);
sym sym_sqrt(sym_env env, const sym s);
sym sym_root(sym_env env, const sym a, const sym b);
int sym_cmp(sym_env env, const sym a, const sym b);
void sym_print(sym_env env, const sym s);
void sym_free(sym_env env, sym s);
void sym_print_history(sym_env env);
void sym_add_coefficient(sym_env env, sym poly, const sym coefficient, const sym exponent);
void sym_add_vec(sym_env env, sym vec, unsigned idx, const sym x);
void sym_set_coords(sym_env env, const char *coords);
int sym_hist_add(enum priority prio,
                 sym_env env,
                 const sym sym,
                 const char *fmt,
                 ...);
