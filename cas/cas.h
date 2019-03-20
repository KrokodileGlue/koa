#pragma once

#include <inttypes.h>
#include <stdbool.h>

struct sym {
	enum sym_type {
		SYM_NUM,
		SYM_RATIO,
		SYM_PRODUCT,
		SYM_SUM,
		SYM_DIFFERENCE,
		SYM_VARIABLE,
		SYM_POWER
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

struct sym_env {
	unsigned precision;
	unsigned hist_len;
	unsigned num_eq;

	enum priority {
		PRIO_ALGEBRA,
		PRIO_ANSWER
	} prio;

	struct {
		sym result;
		char msg[1024];
	} *hist;
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
sym sym_simplify(sym_env env, const sym s, int N, int suppress);
sym sym_sqrt(sym_env env, const sym s);
sym sym_root(sym_env env, const sym a, const sym b);
sym sym_parse_number(sym_env env, const char **s);
sym sym_parse_latex(sym_env env, const char **s, int prec);
int sym_cmp(sym_env env, const sym a, const sym b);
void sym_print(sym_env env, const sym s);
void sym_free(sym_env env, sym s);
void sym_print_history(sym_env env);
