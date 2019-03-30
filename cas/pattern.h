#pragma once

#include "cas.h"

struct map {
	int len;
	char **indeterminate;
	struct sym **symb;
};

struct map *sym_match(sym_env env, const sym a, const sym b);
struct sym *sym_match_lookup(sym_env env, struct map *m, const char *x);
struct sym *sym_replace(sym_env env, struct map *m, const sym s);
void sym_map_set(sym_env env, struct map *m, const char *x, sym s);
