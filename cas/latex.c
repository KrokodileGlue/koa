#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

#include "arithmetic.h"
#include "cas.h"

struct op {
	char *body, *body2;

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
	{ "\\left(", "\\right)", 6, LEFT,  GROUP  },
	{ "\\cdot", "n", 5, LEFT,  BINARY },
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

struct op *
get_prefix_op(const char *c)
{
	FOR(i, operators) {
		size_t len = strlen(operators[i].body);
		if (!strncmp(operators[i].body, c, len)
		    && (operators[i].type == PREFIX
			    || operators[i].type == GROUP))
			return operators + i;
	}

	return NULL;
}

int
get_prec(const char *c, int prec)
{
	struct op *op = get_infix_op(c);
	if (op) return op->prec;
	return prec;
}

sym
sym_parse_latex(const char **s, int prec)
{
	sym a = NULL;

	while (isspace(**s)) (*s)++;
	struct op *op = get_prefix_op(*s);

	if (op && op->type == GROUP) {
		*s += strlen(op->body);
		a = sym_parse_latex(s, 0);
		/* TODO: Errors. */
		if (strncmp(*s, op->body2, strlen(op->body2)))
			puts("parse error"), exit(1);
		*s += strlen(op->body2);
	} else if (!strncmp(*s, "\\frac", 5)) {
		*s += 5;
		a = sym_new(SYM_RATIO);
		a->a = sym_parse_latex(s, 6);
		a->b = sym_parse_latex(s, 6);
	} else if (**s == '-' || isdigit(**s)) {
		a = sym_parse_number(s);
	} else {
		puts("wtf"), printf("%c\n", **s);
		exit(1);
	}

	while (isspace(**s)) (*s)++;

	while (prec < get_prec(*s, prec)) {
		struct op *op = get_infix_op(*s);
		if (!op) break;

		if (!strcmp(op->body, "+")) {
			(*s)++;
			sym b = sym_new(SYM_SUM);
			b->a = a;
			b->b = sym_parse_latex(s, op->ass == LEFT ? op->prec : op->prec - 1);
			a = b;
		} else if (!strcmp(op->body, "-")) {
			(*s)++;
			sym b = sym_new(SYM_DIFFERENCE);
			b->a = a;
			b->b = sym_parse_latex(s, op->ass == LEFT ? op->prec : op->prec - 1);
			a = b;
		} else if (!strcmp(op->body, "*")) {
			(*s)++;
			sym b = sym_new(SYM_PRODUCT);
			b->a = a;
			b->b = sym_parse_latex(s, op->ass == LEFT ? op->prec : op->prec - 1);
			a = b;
		} else if (!strcmp(op->body, "\\cdot")) {
			*s += 5;
			sym b = sym_new(SYM_PRODUCT);
			b->a = a;
			b->b = sym_parse_latex(s, op->ass == LEFT ? op->prec : op->prec - 1);
			a = b;
		} else if (!strcmp(op->body, "/")) {
			(*s)++;
			sym b = sym_new(SYM_RATIO);
			b->a = a;
			b->b = sym_parse_latex(s, op->ass == LEFT ? op->prec : op->prec - 1);
			a = b;
		} else {
			break;
		}
	}
	
	return a;
}

sym
sym_parse_number(const char **s)
{
	num x = NULL, num, exp = NULL;
	_Bool sign = true, dec = false;

	while (**s && isspace(**s)) (*s)++;
	if (**s == '-') sign = false, (*s)++;
	while (**s && isspace(**s)) (*s)++;
	while (**s && (isdigit(**s) || **s == '.')) {
		if (**s == '.') {
			dec = 1, (*s)++;
			continue;
		}

		if (dec) math_uinc(&exp);

		x = math_umul(x, &(struct num){10,0,0});
		x = math_uadd(x, &(struct num){**s - '0',0,0});

		(*s)++;
	}

	math_unorm(&x), math_unorm(&exp);

	sym a = sym_new(SYM_NUM);
	a->sig = x, a->exp = exp, a->sign = sign;

	return a;
}

