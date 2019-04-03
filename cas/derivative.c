#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "derivative.h"
#include "arithmetic.h"
#include "pattern.h"
#include "print.h"
#include "latex.h"
#include "util.h"

/* TODO: Priorities are wrong. */

#define MAX_RULE_LEN 100

static struct derivative {
	const char a[MAX_RULE_LEN + 1], b[MAX_RULE_LEN + 1];
	struct sym *A, *B;
} rules[] = {
	{ "c", "0", NULL, NULL },
	{ "cu", "c\\frac{d}{dx}u", NULL, NULL },
	{ "uv", "(\\frac{d}{dx}u)v+u\\frac{d}{dx}v", NULL, NULL },
	{ "u/c", "(1/c)\\frac{d}{dx}u", NULL, NULL },
	{ "u/v", "((\\frac{d}{dx}u)v-u\\frac{d}{dx}v)/(v^2)", NULL, NULL },
	{ "u+v", "\\frac{d}{dx}u+\\frac{d}{dx}v", NULL, NULL },
	{ "u-v", "\\frac{d}{dx}u-\\frac{d}{dx}v", NULL, NULL },
	{ "u=v", "\\frac{d}{dx}u=\\frac{d}{dx}v", NULL, NULL },
	{ "e^u", "e^u*\\frac{d}{dx}u", NULL, NULL },
	{ "c^u", "(\\ln c)c^u\\frac{d}{dx}u", NULL, NULL },
	{ "u^n", "nu^(n-1)*\\frac{d}{dx}u", NULL, NULL },
	{ "\\ln u", "(1/u)*\\frac{d}{dx}u", NULL, NULL },
	{ "\\log_c u", "\\frac{\\frac{d}{dx}u}{\\ln c*u}", NULL, NULL },
	{ "\\left|u\\right|", "\\frac{u}{\\left|u\\right|}*\\frac{d}{dx}u", NULL, NULL }
};

void
sym_init_differentiation(sym_env env)
{
	for (size_t i = 0; i < sizeof rules / sizeof *rules; i++) {
		rules[i].A = sym_parse_latex2(env, rules[i].a);
		rules[i].B = sym_parse_latex2(env, rules[i].b);
	}
}

bool
sym_is_constant(sym_env env, const sym s)
{
	if (binary(s))
		return sym_is_constant(env, s->a)
			&& sym_is_constant(env, s->b);
	if (unary(s))
		return sym_is_constant(env, s->a);
	if (s->type == SYM_INDETERMINATE)
		return false;
	return true;
}

bool
sym_is_function_of(sym_env env, const sym s, const char *x)
{
	if (binary(s))
		return sym_is_function_of(env, s->a, x)
			|| sym_is_function_of(env, s->b, x);
	if (s->type == SYM_INDETERMINATE && !strcmp(s->text, x))
		return true;
	return false;
}

static void
differentiate(sym_env env,
              sym *s,
              const char *x,
              const sym *original);

static void
sym_do_derivatives(sym_env env,
                   sym *s,
                   const char *x,
                   const sym *original)
{
	if (binary(*s)) {
		sym_do_derivatives(env, &(*s)->a, x, original);
		sym_do_derivatives(env, &(*s)->b, x, original);
	}

	if ((*s)->type == SYM_DERIVATIVE && !strcmp((*s)->wrt, x)) {
		char *wrt = (*s)->wrt;
		*s = (*s)->deriv;
		differentiate(env, s, wrt, original);
		return;
	}

	if (unary(*s)) {
		sym_do_derivatives(env, &(*s)->a, x, original);
	}
}

static void
differentiate(sym_env env,
              sym *s,
              const char *x,
              const sym *original)
{
	if ((*s)->type == SYM_INDETERMINATE
	    && !strcmp((*s)->text, x)) {
		*s = sym_copy(env, DIGIT(1));
		return;
	} else if ((*s)->type == SYM_INDETERMINATE) {
		char buf[32];
		sprintf(buf, "\\frac{d}{dx}{%s}", (*s)->text);
		*s = sym_parse_latex2(env, buf);
		return;
	}

	sym ind = sym_new(env, SYM_INDETERMINATE);
	ind->text = malloc(strlen(x) + 1);
	strcpy(ind->text, x);

	for (size_t i = 0; i < sizeof rules / sizeof *rules; i++) {
		struct map *m = sym_match(env, rules[i].A, *s);
		if (!m) continue;

		if (sym_match_lookup(env, m, "c")
		    && !sym_is_constant(env, sym_match_lookup(env, m, "c")))
			continue;

		sym_map_set(env, m, "x", ind);
		*s = sym_replace(env, m, rules[i].B);

		sym_hist_add(PRIO_ALGEBRA, env, *original,
		             "Differentiate $%s\\Rightarrow %s$",
		             rules[i].a, rules[i].b);

		sym_do_derivatives(env, s, x, original);
		return;
	}
}

sym
sym_differentiate(sym_env env, const sym s, const char *x)
{
	sym_hist_add(PRIO_ALGEBRA, env, s, "Differentiate:");
	sym ret = sym_copy(env, s);
	differentiate(env, &ret, x, &ret);
	return ret;
}
