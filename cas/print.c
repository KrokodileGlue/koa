#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "arithmetic.h"
#include "util.h"
#include "cas.h"

#define print(X) sym_print(e, X)

#define paren(X) (printf("\\left("),	\
                  sym_print(e, X),	\
                  printf("\\right)"))

static bool
weird(const sym s)
{
	if (binary(s)) {
		if (s->type == SYM_POWER
		    && s->a->type == SYM_INDETERMINATE)
			return false;
		return true;
	}

	return false;
}

void
sym_print(sym_env e, sym s)
{
	if (!s) return;

	switch (s->type) {
	case SYM_NULL:
		break;
	case SYM_NEGATIVE:
		printf("-");
		print(s->a);
		break;
	case SYM_CONSTANT:
		switch (s->constant) {
		case CONST_E: printf("e"); break;
		case CONST_PI: printf("\\pi "); break;
		}
		break;
	case SYM_ABS:
		printf("\\left|");
		print(s->a);
		printf("\\right|");
		break;
	case SYM_LOGARITHM:
		if (s->a->type == SYM_CONSTANT
		    && s->a->constant == CONST_E) {
			printf("\\ln ");
		} else {
			printf("\\log_{");
			print(s->a);
			printf("}");
		}

		if (weird(s->b)) paren(s->b);
		else print(s->b);
		break;
	case SYM_DERIVATIVE:
		printf("\\frac{d}{d%s}{", s->wrt);
		print(s->deriv);
		printf("}");
		break;
	case SYM_PRODUCT:
		if (weird(s->a)
		    && s->a->type != SYM_PRODUCT
		    && s->a->type != SYM_POWER)
			paren(s->a);
		else print(s->a);

		if ((weird(s->a) && !weird(s->b))
		    || (s->a->type == SYM_PRODUCT
		        && s->a->b->type == SYM_NUM)) {
			printf("\\cdot ");
		}

		print(s->b);
		break;
	case SYM_CROSS:
		print(s->a);
		printf("\\times");
		print(s->b);
		break;
	case SYM_SUM:
		print(s->a);
		printf("+");
		print(s->b);
		break;
	case SYM_DIFFERENCE:
		print(s->a);
		printf("-");
		print(s->b);
		break;
	case SYM_POWER:
		printf("{");

		if (weird(s->a)) paren(s->a);
		else print(s->a);

		printf("}^{");
		print(s->b);
		printf("}");
		break;
	case SYM_SUBSCRIPT:
		printf("{");
		print(s->a);
		printf("}_{");
		print(s->b);
		printf("}");
		break;
	case SYM_EQUALITY:
		printf("{");
		print(s->a);
		printf("=");
		print(s->b);
		printf("}");
		break;
	case SYM_RATIO:
		printf("\\frac{"), print(s->a);
		printf("}{"), print(s->b);
		printf("}");
		break;
	case SYM_VARIABLE:
		printf("%s", s->text);
		break;
	case SYM_INDETERMINATE:
		printf("%s", s->text);
		break;
	case SYM_POLYNOMIAL: {
		struct polynomial *c = s->polynomial;

		while (c) {
			print(c->coefficient);

			printf(strlen(s->indeterminate) != 1
			       ? "\\text{%s}^{" : "%s^{",
			       s->indeterminate);

			print(c->exponent);
			printf("}");
			c = c->next;
			if (c) printf("+");
		}
	} break;
	case SYM_VECTOR: {
		printf("<");

		for (unsigned i = 0; i < s->len; i++) {
			print(s->vector[i]);
			if (i != s->len - 1) printf(" ");
		}

		printf(">");
	} break;
	case SYM_NUM: {
		num tmp = math_ucopy(s->exp);
		num whole = s->sig;

		while (!math_uzero(tmp)) {
			if (whole) whole = whole->next;
			math_udec(&tmp);
		}

		math_print(whole);

		num div = NULL;
		for (num i = math_ucopy(s->exp);
		     !math_uzero(i);
		     math_udec(&i))
			math_upush(&div, 0);
		math_upush(&div, 1);

		num fraction = NULL;
		if (whole) whole = whole->prev;

		if (!whole) {
			whole = s->sig;
			while (whole && whole->next) whole = whole->next;
		}

		while (whole) {
			math_uprepend(&fraction, whole->x);
			whole = whole->prev;
		}

		num r = NULL;
		math_udiv2(&fraction, div, &r);

		if (r) putchar('.');

		unsigned i = 0;

		while (!math_uzero(r) && i < e->precision) {
			num tmp = NULL, tmp2 = NULL;
			r = math_umul(r, &(struct num){10,0,0});
			math_udiv(r, div, &tmp, &tmp2);
			math_print(tmp);
			r = tmp2, i++;
			fflush(stdout);
		}
	} break;
	}
}
