#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "derivative.h"
#include "arithmetic.h"
#include "pattern.h"
#include "latex.h"
#include "util.h"

#define MAX_RULE_LEN 100

static struct derivative {
	const char a[MAX_RULE_LEN + 1], b[MAX_RULE_LEN + 1];
	struct sym *A, *B;
} rules[] = {
	{ "c", "0", NULL, NULL },
	{ "cu", "c\\frac{d}{dx}u", NULL, NULL },
	{ "uv", "(\\frac{d}{dx}u)v+u\\frac{d}{dx}v", NULL, NULL },
	{ "u/v", "((\\frac{d}{dx}u)v-u\\frac{d}{dx}v)/(v^2)", NULL, NULL },
	{ "u+v", "\\frac{d}{dx}u+\\frac{d}{dx}v", NULL, NULL },
	{ "u-v", "\\frac{d}{dx}u-\\frac{d}{dx}v", NULL, NULL },
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

static sym
sym_do_derivatives(sym_env env, const sym s, const char *x)
{
	struct sym *r = sym_copy(env, s);

	if (binary(r)) {
		r->a = sym_do_derivatives(env, r->a, x);
		r->b = sym_do_derivatives(env, r->b, x);
	}

	if (r->type == SYM_DERIVATIVE && !strcmp(s->wrt, x))
		return sym_differentiate(env, s->deriv, s->wrt);

	if (unary(r)) {
		r->a = sym_do_derivatives(env, r->a, x);
	}

	return r;
}

sym
sym_differentiate(sym_env env, const sym s, const char *x)
{
	if (s->type == SYM_INDETERMINATE && !strcmp(s->text, x))
		return sym_copy(env, DIGIT(1));

	sym ind = sym_new(env, SYM_INDETERMINATE);
	ind->text = malloc(strlen(x) + 1);
	strcpy(ind->text, x);

	for (size_t i = 0; i < sizeof rules / sizeof *rules; i++) {
		struct map *m = sym_match(env, rules[i].A, s);
		if (!m) continue;

		if (sym_match_lookup(env, m, "c")
		    && !sym_is_constant(env, sym_match_lookup(env, m, "c")))
			continue;

		sym_map_set(env, m, "x", ind);
		struct sym *ret = sym_replace(env, m, rules[i].B);
		return sym_do_derivatives(env, ret, x);
	}

	return NULL;
}
