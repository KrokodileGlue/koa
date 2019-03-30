#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "derivative.h"
#include "arithmetic.h"
#include "simplify.h"
#include "pattern.h"
#include "latex.h"
#include "cas.h"

#define MAX_RULE_LEN 100

static struct rule {
	const char a[MAX_RULE_LEN + 1], b[MAX_RULE_LEN + 1];
	struct sym *A, *B;
} rules[] = {
	/* Some other stuff. */
	{ "k(la)", "(kl)a", NULL, NULL },
	{ "k(l+u)", "kl+ku", NULL, NULL },
	{ "k(u+l)", "kl+ku", NULL, NULL },
	{ "k(l-u)", "kl-ku", NULL, NULL },
	{ "k(u-l)", "ku-kl", NULL, NULL },
	{ "(l+u)k", "kl+ku", NULL, NULL },
	{ "(u+l)k", "kl+ku", NULL, NULL },
	{ "(l-u)k", "kl-ku", NULL, NULL },
	{ "(u-l)k", "ku-kl", NULL, NULL },
	/* Things. */
	{ "\\frac{a}{\\frac{b}{c}}", "\\frac{ac}{b}", NULL, NULL },
	{ "a--b", "a+b", NULL, NULL },
	{ "(-a)b", "-(ab)", NULL, NULL },
	{ "a(-b)", "-(ab)", NULL, NULL },
	{ "a+-b", "a-b", NULL, NULL },
	{ "a(b+c)", "ab+ac", NULL, NULL },
	{ "a(b-c)", "ab-ac", NULL, NULL },
	{ "(-a)/(-b)", "a/b", NULL, NULL },
	{ "(-a)/b", "-(a/b)", NULL, NULL },
	{ "a/(-b)", "-(a/b)", NULL, NULL },
	{ "\\frac{\\frac{a}{b}}{c}", "a/(bc)", NULL, NULL },
	{ "a+0", "a", NULL, NULL },
	{ "0+a", "a", NULL, NULL },
	{ "a-0", "a", NULL, NULL },
	{ "0-a", "-a", NULL, NULL },
	{ "0*a", "0", NULL, NULL },
	{ "a*0", "0", NULL, NULL },
	{ "a*1", "a", NULL, NULL },
	{ "1*a", "a", NULL, NULL },
	/* Hmm, some of these should only be done for constants. */
	{ "a/b-c/d", "\\frac{ad-bc}{bd}", NULL, NULL },
	{ "a/b-c", "\\frac{a-cb}{b}", NULL, NULL },
	{ "c-a/b", "\\frac{cb-a}{b}", NULL, NULL },
	{ "a/b+c/d", "\\frac{ad+bc}{bd}", NULL, NULL },
	{ "a/b+c", "\\frac{a+cb}{b}", NULL, NULL },
	{ "c+a/b", "\\frac{a+cb}{b}", NULL, NULL },
	{ "(a/b)*c", "\\frac{ac}{b}", NULL, NULL },
	{ "c*(a/b)", "\\frac{ac}{b}", NULL, NULL },
	//{ "", "", NULL, NULL },
};

void
sym_init_simplification(sym_env env)
{
	for (size_t i = 0; i < sizeof rules / sizeof *rules; i++) {
		rules[i].A = sym_parse_latex2(env, rules[i].a);
		rules[i].B = sym_parse_latex2(env, rules[i].b);
	}
}

static int
simplify(sym_env e, sym *s)
{
	int modified = 0;

	if (binary(*s)) {
		modified = simplify(e, &(*s)->a)
			|| simplify(e, &(*s)->b);
	} else if (unary(*s)) {
		modified = simplify(e, &(*s)->a);
	}

	if (binary(*s)
	    && (*s)->a->type == SYM_NUM
	    && (*s)->b->type == SYM_NUM) {
		switch ((*s)->type) {
		case SYM_SUM:
			*s = sym_add(e, (*s)->a, (*s)->b);
			modified = 1;
			break;
		case SYM_DIFFERENCE:
			*s = sym_sub(e, (*s)->a, (*s)->b);
			modified = 1;
			break;
		case SYM_PRODUCT:
			*s = sym_mul(e, (*s)->a, (*s)->b);
			modified = 1;
			break;
		case SYM_RATIO: {
			struct sym *gcf = sym_gcf(e, (*s)->a, (*s)->b);
			if (!gcf || !sym_cmp(e, gcf, DIGIT(1))) break;
			(*s)->a = sym_div(e, (*s)->a, gcf);
			(*s)->b = sym_div(e, (*s)->b, gcf);
			modified = 1;
		} break;
		}
	}

	for (size_t i = 0; i < sizeof rules / sizeof *rules; i++) {
		struct map *m = sym_match(e, rules[i].A, *s);
		if (!m) continue;

		for (char c = 'k'; c < 'z'; c++) {
			if (sym_match_lookup(e, m, (char []){c, 0})
			    && !sym_is_constant(e, sym_match_lookup(e, m, (char []){c, 0})))
				continue;
		}

		if (sym_match_lookup(e, m, "u")
		    && sym_is_constant(e, sym_match_lookup(e, m, "u")))
			continue;

		*s = sym_replace(e, m, rules[i].B);
		return simplify(e, s);
	}

	return modified;
}

sym
sym_simplify(sym_env e, const sym s)
{
	sym ret = sym_copy(e, s);
	while (simplify(e, &ret));
	return ret;
}

sym
sym_normalize(sym_env e, const sym s)
{

}
