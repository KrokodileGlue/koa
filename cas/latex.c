#include <stdlib.h>
#include <string.h>

#include "cas.h"

struct op {
	const char *body, *body2;

	int prec;

	enum {
		LEFT,
		RIGHT
	} ass;

	enum {
		PREFIX,
		GROUP,
		POSTFIX,
		BINARY,
		MEMBER,
		TERNARY
	} type;
} operators[] = {
	{ "+", "n", 6, RIGHT, PREFIX },
	{ "-", "n", 6, RIGHT, PREFIX },
	{ "{", "}", 6, LEFT,  GROUP  },
	{ "*", "n", 5, LEFT,  BINARY },
	{ "/", "n", 5, LEFT,  BINARY },
	{ "+", "n", 4, LEFT,  BINARY },
	{ "-", "n", 4, LEFT,  BINARY },
	{ "=", "n", 2, LEFT,  BINARY },
	{ ",", "n", 1, RIGHT, BINARY }
};

#define FOR(I,X) \
for (size_t I = 0; \
     I < sizeof (X) / sizeof (X)[0]; \
     i++)

struct op *get_infix_op(const char *c)
{
	FOR(i, operators) {
		size_t len = strlen(operators[i].body);
		if (!strncmp(operators[i].body, c, len)
		    && operators[i].type != PREFIX)
			return operators + i;
	}

	return NULL;
}

struct op *get_prefix_op(const char *c)
{
	FOR(i, operators) {
		size_t len = strlen(operators[i].body);
		if (!strncmp(operators[i].body, c, len)
		    && operators[i].type == PREFIX)
			return operators + i;
	}

	return NULL;
}

int get_prec(const char *c, int prec)
{
	struct op *op = get_infix_op(c);
	if (op) return op->prec;
	return prec;
}

sym
sym_parse_latex(const char *s)
{
	return NULL;
}
