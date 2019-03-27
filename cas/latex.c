#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

#include "arithmetic.h"
#include "latex.h"
#include "cas.h"

struct op {
	enum sym_type sym_type;
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
	{ SYM_NULL, "-", "n", 6, RIGHT, PREFIX },
	{ SYM_NULL, "{", "}", 6, LEFT, GROUP },
	{ SYM_NULL, "\\left(", "\\right)", 6, LEFT, GROUP },
	{ SYM_NULL, "(", ")", 6, LEFT, GROUP },
	{ SYM_NULL, "[", "]", 6, LEFT, GROUP },
	{ SYM_NULL, "\\left[", "\\right]", 6, LEFT, GROUP },
	{ SYM_SUBSCRIPT, "_", "n", 9, LEFT, BINARY },
	{ SYM_POWER, "^", "n", 5, LEFT, BINARY },
	{ SYM_PRODUCT, "\\cdot", "n", 5, LEFT, BINARY },
	{ SYM_PRODUCT, "*", "n", 5, LEFT, BINARY },
	{ SYM_CROSS, "\\times", "n", 5, LEFT, BINARY },
	{ SYM_RATIO, "/", "n", 5, LEFT, BINARY },
	{ SYM_SUM, "+", "n", 4, LEFT, BINARY },
	{ SYM_DIFFERENCE, "-", "n", 4, LEFT, BINARY },
	{ SYM_EQUALITY, "=", "n", 2, LEFT, BINARY }
};

#define FOR(I,X)	  \
	for (size_t I = 0; \
	     I < sizeof (X) / sizeof (X)[0]; \
	     i++)

struct op *
get_infix_op(const char *c)
{
	FOR(i, operators) {
		size_t len = strlen(operators[i].body);
		if (!strncmp(operators[i].body, c, len)
		    && operators[i].type != GROUP
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
sym_parse_latex(sym_env env, const char **s, int prec)
{
	sym a = NULL;

	while (isspace(**s)) (*s)++;
	struct op *op = get_prefix_op(*s);

	unsigned i = 0;

	if (**s == '<') {
		a = sym_new(env, SYM_VECTOR);
		(*s)++;
		while (1) {
			struct sym *tmp = sym_parse_latex(env, s, prec);

			if (!tmp) {
				if (**s != '>') {
					puts("parse error");
					exit(1);
				} else {
					(*s)++;
					break;
				}
			}

			sym_add_vec(env, a, i++, tmp);
			if (**s == ',') (*s)++;
		}
	} else if (op && op->type == GROUP) {
		*s += strlen(op->body);
		a = sym_parse_latex(env, s, 0);
		/* TODO: Errors. */
		if (strncmp(*s, op->body2, strlen(op->body2)))
			puts("parse error"), exit(1);
		*s += strlen(op->body2);
	} else if (op) {
		*s += strlen(op->body);
		a = sym_parse_latex(env, s, op->prec);
		if (!strcmp(op->body, "-"))
			a->sign = 0;
	} else if (!strncmp(*s, "\\frac", 5)) {
		*s += 5;
		a = sym_new(env, SYM_RATIO);
		a->a = sym_parse_latex(env, s, 6);
		a->b = sym_parse_latex(env, s, 6);
	} else if (isdigit(**s)) {
		a = sym_parse_number(env, s);
	} else if ((**s >= 'a' && **s <= 'z') || (**s >= 'A' && **s <= 'Z')) {
		a = sym_new(env, SYM_INDETERMINATE);
		a->text = malloc(2);
		a->text[0] = **s;
		a->text[1] = 0;
		(*s)++;
	} else {
		return NULL;
	}

	while (isspace(**s)) (*s)++;

	struct sym *vec = NULL;
	i = 0;

	while (prec < get_prec(*s, prec) || (!get_infix_op(*s) && **s)) {
		int flag = 0;
		struct op *op = get_infix_op(*s);
		if (!op) {
			flag = 1;
			op = get_infix_op("*");
			if (prec >= op->prec) break;
		} else {
			*s += strlen(op->body);
		}

		const char *x = *s;
		sym tmp = sym_parse_latex(env, s, op->ass == LEFT ? op->prec : op->prec - 1);

		if (!tmp) break;
		if (tmp->type == SYM_NUM && flag) {
			*s = x;
			break;
		}

		sym b = sym_new(env, SYM_RATIO);
		b->a = a;
		b->b = tmp;

		b->type = op->sym_type;

		if (b->type == SYM_SUM || b->type == SYM_DIFFERENCE) {
			if (b->type == SYM_DIFFERENCE)
				b->b->sign = !b->b->sign;

			if (!vec) {
				vec = sym_new(env, SYM_LIST);
				sym_add_vec(env, vec, i++, b->a);
				sym_add_vec(env, vec, i++, b->b);
			} else {
				sym_add_vec(env, vec, i++, b->b);
			}

			a = vec;
		} else {
			a = b;
		}
	}

	return a;
}

sym
sym_parse_number(sym_env env, const char **s)
{
	num x = NULL, num, exp = NULL;
	_Bool sign = true, dec = false;

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

	sym a = sym_new(env, SYM_NUM);
	a->sig = x, a->exp = exp, a->sign = sign;

	return a;
}
