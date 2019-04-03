#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "antiderivative.h"
#include "derivative.h"
#include "arithmetic.h"
#include "pattern.h"
#include "latex.h"
#include "print.h"
#include "util.h"

#define MAX_RULE_LEN 100

static struct {
	const char a[MAX_RULE_LEN + 1], b[MAX_RULE_LEN + 1];
	struct sym *A, *B;
} rules[] = {
	{ "x", "(1/2)x^2", NULL, NULL },
	{ "x^n", "(1/(n+1))x^(n+1)", NULL, NULL },
	{ "1/x", "\\ln x", NULL, NULL },
	{ "1/(x+a)", "\\ln(x+a)", NULL, NULL },
	{ "1/(ax+b)", "(1/a)\\ln(ax+b)", NULL, NULL },
	{ "1/(x+a)^2", "-1/(x+a)", NULL, NULL },
	{ "(x+a)^n", "(x+a)^n(a/(1+n)+x/(1+n))", NULL, NULL },
	{ "x/(a^2+x^2)", "(x+a)^n(1/(a+n)+x/(a+n))", NULL, NULL },
};

void
sym_init_antidifferentiation(sym_env env)
{
	for (size_t i = 0; i < sizeof rules / sizeof *rules; i++) {
		rules[i].A = sym_parse_latex2(env, rules[i].a);
		rules[i].B = sym_parse_latex2(env, rules[i].b);
	}
}

static int
integrate(sym_env env,
          sym *s,
          const char *x,
          const sym *original)
{
	sym_print(env, *s), puts("");

	for (size_t i = 0; i < sizeof rules / sizeof *rules; i++) {
		struct map *m = sym_match(env, rules[i].A, *s);
		if (!m) continue;

		struct sym *l = sym_match_lookup(env, m, "x");

		if (l && (l->type != SYM_INDETERMINATE
		          || strcmp(l->text, x)))
			continue;

		for (int i = 0; i < m->len; i++) {
			if (!strcmp(m->indeterminate[i], "x"))
				continue;
			if (!strcmp(m->indeterminate[i], "u"))
				continue;
			if (!sym_is_constant(env, m->symb[i]))
				goto again;
		}

		*s = sym_replace(env, m, rules[i].B);

		sym_hist_add(PRIO_ALGEBRA, env, *original,
		             "Apply rule $%s\\Rightarrow %s$",
		             rules[i].a, rules[i].b);
		return 1;
	again:;
	}

	puts("no match");

	if ((*s)->type == SYM_PRODUCT
	    && sym_is_constant(env, (*s)->a)) {
		return integrate(env, &(*s)->b, x, original);
	}

	if ((*s)->type == SYM_RATIO
	    && sym_is_constant(env, (*s)->b)) {
		return integrate(env, &(*s)->a, x, original);
	}

	puts("cannot integrate");
	return 0;
}

sym
sym_integrate(sym_env env, const sym s, const char *x)
{
	struct sym *ret = sym_copy(env, s);
	integrate(env, &ret, x, &ret);
	return ret;
}
