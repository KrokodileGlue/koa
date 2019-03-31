#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "derivative.h"
#include "arithmetic.h"
#include "simplify.h"
#include "pattern.h"
#include "latex.h"
#include "util.h"
#include "cas.h"

#define MAX_RULE_LEN 100

static struct {
	const char a[MAX_RULE_LEN + 1], b[MAX_RULE_LEN + 1];
	struct sym *A, *B;
} rules[] = {
	/* Fundamentals. */
	{ "ku", "uk", NULL, NULL },
	{ "k+u", "u+k", NULL, NULL },
	{ "k-u", "-u+k", NULL, NULL },
	{ "(uk)l", "u(kl)", NULL, NULL },
	{ "-a-b", "-(a+b)", NULL, NULL },

	/* Some other stuff. */
	{ "(u+l)k", "kl+uk", NULL, NULL },
	{ "(-u+l)k", "kl-uk", NULL, NULL },
	{ "(u-l)k", "uk-kl", NULL, NULL },
	{ "-(u+l)k", "-uk-kl", NULL, NULL },

	{ "(-u+l)+k", "-u+(k+l)", NULL, NULL },
	{ "(u+l)+k", "u+(k+l)", NULL, NULL },
	{ "-(u+l)+k", "-u+(k-l)", NULL, NULL },
	{ "(u-l)+k", "u+(k-l)", NULL, NULL },

	{ "(uk)v", "(uv)k", NULL, NULL },

	{ "\\ln e", "1", NULL, NULL },

	{ "aa", "a^2", NULL, NULL },
	/* { "a^b*a^c", "a^(b+c)", NULL, NULL }, */
	/* { "(a^b)^c", "a^(bc)", NULL, NULL }, */

	{ "(ab+bc)/b", "a+c", NULL, NULL },
	{ "(ab-bc)/b", "a-c", NULL, NULL },

	/* Logarithms. */
	{ "\\ln a-\\ln b", "\\ln{a/b}", NULL, NULL },
	{ "\\ln a+\\ln b", "\\ln{ab}", NULL, NULL },
	{ "\\ln{a^b}", "b\\ln a", NULL, NULL },

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
	{ "(a/b)/c", "a/(bc)", NULL, NULL },

	/* Identities. */
	{ "a+0", "a", NULL, NULL },
	{ "a-0", "a", NULL, NULL },
	{ "a*0", "0", NULL, NULL },
	{ "a*1", "a", NULL, NULL },
	{ "a^1", "a", NULL, NULL },
	{ "a-a", "0", NULL, NULL },
	{ "a/a", "1", NULL, NULL },

	/* ??? */
	{ "(ak)/(bk)", "a/b", NULL, NULL },
	{ "(ak)/(bk^c)", "a/(bk^(c-1))", NULL, NULL },
	{ "(ak^c)/(bk)", "(ak^(c-1))/b", NULL, NULL },
	{ "(ak^c)/(bk^d)", "(ak^(c-d))/b", NULL, NULL },

	/* foiesjofies */
	{ "(ak)/k", "a", NULL, NULL },

	/* Idk. */
	{ "ua+ub", "u(a+b)", NULL, NULL },
	{ "u+u", "2u", NULL, NULL },
	{ "au+u", "u(a+1)", NULL, NULL },
	{ "u+au", "u(a+1)", NULL, NULL },

	/* Hmm, some of these should only be done for constants. */
	{ "a/b-c", "\\frac{a-cb}{b}", NULL, NULL },
	{ "c-a/b", "\\frac{cb-a}{b}", NULL, NULL },
	{ "a/b+k", "\\frac{a+kb}{b}", NULL, NULL },
	{ "-(a/b)+k", "\\frac{kb-a}{b}", NULL, NULL },
	{ "c+a/b", "\\frac{a+cb}{b}", NULL, NULL },
	{ "(a/b)*c", "\\frac{ac}{b}", NULL, NULL },
	{ "c*(a/b)", "\\frac{ac}{b}", NULL, NULL },
	//{ "", "", NULL, NULL },
};

static struct {
	const char a[MAX_RULE_LEN + 1], b[MAX_RULE_LEN + 1];
	struct sym *A, *B;
} rules2[] = {
	/* Fundamentals. */
	{ "(u+a)k", "uk+ak", NULL, NULL },
	{ "(u-a)k", "uk-ak", NULL, NULL },
	{ "(a+b)/c", "a/c+b/c", NULL, NULL },
	{ "(a-b)/c", "a/c-b/c", NULL, NULL },
};

void
sym_init_simplification(sym_env env)
{
	for (size_t i = 0; i < sizeof rules / sizeof *rules; i++) {
		rules[i].A = sym_parse_latex2(env, rules[i].a);
		rules[i].B = sym_parse_latex2(env, rules[i].b);
	}

	for (size_t i = 0; i < sizeof rules2 / sizeof *rules2; i++) {
		rules2[i].A = sym_parse_latex2(env, rules2[i].a);
		rules2[i].B = sym_parse_latex2(env, rules2[i].b);
	}
}

static int
simplify(sym_env e, sym *s)
{
	//sym_print(e, *s), puts("");
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
		/* case SYM_RATIO: { */
		/* 	struct sym *atmp = sym_copy(e, (*s)->a); */
		/* 	struct sym *btmp = sym_copy(e, (*s)->b); */

		/* 	struct sym *gcf = sym_gcf(e, atmp, btmp); */
		/* 	if (!gcf || !sym_cmp(e, gcf, DIGIT(1))) break; */
		/* 	(*s)->a = sym_div(e, atmp, gcf); */
		/* 	(*s)->b = sym_div(e, btmp, gcf); */
		/* } break; */
		default:;
		}
	}

	for (size_t i = 0; i < sizeof rules / sizeof *rules; i++) {
		struct map *m = sym_match(e, rules[i].A, *s);
		if (!m) continue;

		for (char c = 'k'; c < 'z'; c++) {
			if (c == 'u' || c == 'v') continue;
			struct sym *con = sym_match_lookup(e, m, (char []){c, 0});
			if (!con) continue;
			if (con && !sym_is_constant(e, con))
				goto again;
		}

		if (sym_match_lookup(e, m, "u")
		    && sym_is_constant(e, sym_match_lookup(e, m, "u")))
			continue;

		if (sym_match_lookup(e, m, "v")
		    && sym_is_constant(e, sym_match_lookup(e, m, "v")))
			continue;

		*s = sym_replace(e, m, rules[i].B);

		simplify(e, s);
		return 1;
	again:;
	}

	return modified;
}

static int
phase2(sym_env e, sym *s)
{
	int modified = 0;

	if (binary(*s)) {
		modified = phase2(e, &(*s)->a)
			|| phase2(e, &(*s)->b);
	} else if (unary(*s)) {
		modified = phase2(e, &(*s)->a);
	}

	for (size_t i = 0; i < sizeof rules2 / sizeof *rules2; i++) {
		struct map *m = sym_match(e, rules2[i].A, *s);
		if (!m) continue;

		for (char c = 'k'; c < 'z'; c++) {
			if (c == 'u' || c == 'v') continue;
			struct sym *con = sym_match_lookup(e, m, (char []){c, 0});
			if (!con) continue;
			if (con && !sym_is_constant(e, con))
				goto again;
		}

		if (sym_match_lookup(e, m, "u")
		    && sym_is_constant(e, sym_match_lookup(e, m, "u")))
			continue;

		if (sym_match_lookup(e, m, "v")
		    && sym_is_constant(e, sym_match_lookup(e, m, "v")))
			continue;

		*s = sym_replace(e, m, rules2[i].B);

		return phase2(e, s);
	again:;
	}

	return modified;
}

static sym
dothing(sym_env e, const sym s)
{
	struct sym *tmp = sym_copy(e, s);

	if (s->type == SYM_SUM || s->type == SYM_DIFFERENCE) {
		tmp->a = dothing(e, s->a);
		tmp->b = dothing(e, s->b);
	} else {
		while (simplify(e, &tmp));
	}

	return tmp;
}

sym
sym_simplify(sym_env e, const sym s)
{
	sym ret = sym_copy(e, s);
	while (simplify(e, &ret));
	while (phase2(e, &ret));
	//while (simplify(e, &ret));
	return dothing(e, ret);
	//return ret;
}

sym
sym_normalize(sym_env e, const sym s)
{
	(void)e, (void)s;
	return NULL;
}
