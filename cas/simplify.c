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
	bool internal;
} rules[] = {
	/* Fundamentals. */
	{ "ku", "uk", NULL, NULL, true },
	{ "k+u", "u+k", NULL, NULL, true },
	{ "k-u", "-u+k", NULL, NULL, true },
	{ "(uk)l", "u(kl)", NULL, NULL, true },
	{ "-a-b", "-(a+b)", NULL, NULL, true },

	/* Some other stuff. */
	{ "(u+l)k", "kl+uk", NULL, NULL, false },
	{ "(-u+l)k", "kl-uk", NULL, NULL, false },
	{ "(u-l)k", "uk-kl", NULL, NULL, false },
	{ "-(u+l)k", "-uk-kl", NULL, NULL, false },

	{ "(-u+l)+k", "-u+(k+l)", NULL, NULL, false },
	{ "(u+l)+k", "u+(k+l)", NULL, NULL, false },
	{ "-(u+l)+k", "-u+(k-l)", NULL, NULL, false },
	{ "(u-l)+k", "u+(k-l)", NULL, NULL, false },

	{ "(uk)v", "(uv)k", NULL, NULL, false },

	{ "\\ln e", "1", NULL, NULL, false },

	{ "aa", "a^2", NULL, NULL, false },
	{ "a^b*a^c", "a^(b+c)", NULL, NULL, false },
	{ "(a^b)^c", "a^(bc)", NULL, NULL, false },

	{ "(ab+bc)/b", "a+c", NULL, NULL, false },
	{ "(ab-bc)/b", "a-c", NULL, NULL, false },

	/* Logarithms. */
	{ "\\ln a-\\ln b", "\\ln{a/b}", NULL, NULL, false },
	{ "\\ln a+\\ln b", "\\ln{ab}", NULL, NULL, false },
	{ "\\ln{a^b}", "b\\ln a", NULL, NULL, false },

	/* Things. */
	{ "\\frac{a}{\\frac{b}{c}}", "\\frac{ac}{b}", NULL, NULL, false },
	{ "a--b", "a+b", NULL, NULL, false },
	{ "(-a)b", "-(ab)", NULL, NULL, false },
	{ "a(-b)", "-(ab)", NULL, NULL, false },
	{ "a+-b", "a-b", NULL, NULL, false },
	{ "a(b+c)", "ab+ac", NULL, NULL, false },
	{ "a(b-c)", "ab-ac", NULL, NULL, false },
	{ "(-a)/(-b)", "a/b", NULL, NULL, false },
	{ "(-a)/b", "-(a/b)", NULL, NULL, false },
	{ "a/(-b)", "-(a/b)", NULL, NULL, false },
	{ "(a/b)/c", "a/(bc)", NULL, NULL, false },

	/* Identities. */
	{ "a+0", "a", NULL, NULL, false },
	{ "a-0", "a", NULL, NULL, false },
	{ "a*0", "0", NULL, NULL, false },
	{ "a*1", "a", NULL, NULL, false },
	{ "a^1", "a", NULL, NULL, false },
	{ "a-a", "0", NULL, NULL, false },
	{ "a/a", "1", NULL, NULL, false },
	{ "--a", "a", NULL, NULL, false },

	/* ??? */
	{ "(ak)/(bk)", "a/b", NULL, NULL, false },
	{ "(ak)/(bk^c)", "a/(bk^(c-1))", NULL, NULL, false },
	{ "(ak^c)/(bk)", "(ak^(c-1))/b", NULL, NULL, false },
	{ "(ak^c)/(bk^d)", "(ak^(c-d))/b", NULL, NULL, false },

	/* foiesjofies */
	{ "(ak)/k", "a", NULL, NULL, false },

	/* Idk. */
	{ "ua+ub", "u(a+b)", NULL, NULL, false },
	{ "u+u", "2u", NULL, NULL, false },
	{ "au+u", "u(a+1)", NULL, NULL, false },
	{ "u+au", "u(a+1)", NULL, NULL, false },

	/* Hmm, some of these should only be done for constants. */
	{ "a/b-c", "\\frac{a-cb}{b}", NULL, NULL, false },
	{ "c-a/b", "\\frac{cb-a}{b}", NULL, NULL, false },
	{ "a/b+k", "\\frac{a+kb}{b}", NULL, NULL, false },
	{ "-(a/b)+k", "\\frac{kb-a}{b}", NULL, NULL, false },
	{ "c+a/b", "\\frac{a+cb}{b}", NULL, NULL, false },
	{ "(a/b)*c", "\\frac{ac}{b}", NULL, NULL, false },
	{ "c*(a/b)", "\\frac{ac}{b}", NULL, NULL, false },
	//{ "", "", NULL, NULL, false },
};

static struct {
	const char a[MAX_RULE_LEN + 1], b[MAX_RULE_LEN + 1];
	struct sym *A, *B;
	bool internal;
} rules2[] = {
	{ "(ab)/b", "a", NULL, NULL, false },
	{ "(u+a)k", "uk+ak", NULL, NULL, false },
	{ "(u-a)k", "uk-ak", NULL, NULL, false },
	{ "(a+b)/c", "a/c+b/c", NULL, NULL, false },
	{ "(a-b)/c", "a/c-b/c", NULL, NULL, false },
	{ "a(bc)", "(ab)c", NULL, NULL, true },
	{ "(ua)/b", "(a/b)u", NULL, NULL, true },
	{ "aa", "a^2", NULL, NULL, false },
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
simplify(sym_env e, sym *s, const sym *original)
{
	//sym_print(e, *s), puts("");
	int modified = 0;

	if (binary(*s)) {
		modified = simplify(e, &(*s)->a, original)
			|| simplify(e, &(*s)->b, original);
	} else if (unary(*s)) {
		modified = simplify(e, &(*s)->a, original);
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
			struct sym *atmp = sym_copy(e, (*s)->a);
			struct sym *btmp = sym_copy(e, (*s)->b);

			struct sym *gcf = sym_gcf(e, atmp, btmp);
			if (!gcf || !sym_cmp(e, gcf, DIGIT(1))) break;
			(*s)->a = sym_div(e, atmp, gcf);
			(*s)->b = sym_div(e, btmp, gcf);
		} break;
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

		if (!rules[i].internal)
			sym_hist_add(PRIO_ALGEBRA, e, *original,
			             "Apply $%s\\Rightarrow %s$",
			             rules[i].a, rules[i].b);

		simplify(e, s, original);
		return 1;
	again:;
	}

	return modified;
}

static int
phase2(sym_env e, sym *s, const sym *original)
{
	int modified = 0;

	if (binary(*s)) {
		modified = phase2(e, &(*s)->a, original)
			|| phase2(e, &(*s)->b, original);
	} else if (unary(*s)) {
		modified = phase2(e, &(*s)->a, original);
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

		if (!rules2[i].internal)
			sym_hist_add(PRIO_ALGEBRA, e, *original,
			             "Rewrite $%s\\Rightarrow %s$",
			             rules2[i].a, rules2[i].b);

		return 1;
	again:;
	}

	return modified;
}

sym
sym_simplify(sym_env e, const struct sym *s)
{
	sym ret = sym_copy(e, s);

	while (simplify(e, &ret, &ret));
	while (phase2(e, &ret, &ret));

	return ret;
}

sym
sym_normalize(sym_env e, const struct sym *s)
{
	(void)e, (void)s;
	return NULL;
}
