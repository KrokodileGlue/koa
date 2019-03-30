#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "pattern.h"

struct sym *
sym_replace(sym_env env, struct map *m, const sym s)
{
	struct sym *ret = sym_copy(env, s);

	if (binary(ret)) {
		ret->a = sym_replace(env, m, ret->a);
		ret->b = sym_replace(env, m, ret->b);
	}

	if (unary(ret))
		ret->a = sym_replace(env, m, ret->a);

	if (ret->type != SYM_INDETERMINATE) return ret;

	struct sym *n = sym_match_lookup(env, m, ret->text);
	return n ? sym_copy(env, n) : ret;
}

void
sym_map_set(sym_env env, struct map *m, const char *x, sym s)
{
	if (sym_match_lookup(env, m, x)) return;
	m->indeterminate = realloc(m->indeterminate,
	                           (m->len + 1) * sizeof *m->indeterminate);
	m->symb = realloc(m->symb, ++m->len * sizeof *m->symb);
	m->indeterminate[m->len - 1] = malloc(strlen(x) + 1);
	strcpy(m->indeterminate[m->len - 1], x);
	m->symb[m->len - 1] = s;
}

static bool
match(sym_env env, struct map *m, const sym a, const sym b)
{
	if (a->type == SYM_INDETERMINATE) {
		struct sym *lookup = sym_match_lookup(env, m, a->text);
		if (lookup && !sym_cmp(env, lookup, b))
			return false;
		sym_map_set(env, m, a->text, sym_copy(env, b));
		return true;
	}

	if (a->type != b->type) return false;

	if (binary(a))
		return match(env, m, a->a, b->a)
			&& match(env, m, a->b, b->b);

	if (unary(a))
		return match(env, m, a->a, b->a);

	return sym_cmp(env, a, b) == 0;
}

struct map *
sym_match(sym_env env, const sym a, const sym b)
{
	struct map *m = malloc(sizeof *m);
	memset(m, 0, sizeof *m);
	if (match(env, m, a, b)) return m;
	return free(m), NULL;
}

struct sym *
sym_match_lookup(sym_env env, struct map *m, const char *x)
{
	for (int i = 0; i < m->len; i++)
		if (!strcmp(m->indeterminate[i], x))
			return m->symb[i];
	return NULL;
}
