#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "arithmetic.h"
#include "cas.h"

#define PRINT_PARENTHESIZED(X)	  \
	if ((X)->type != SYM_NUM && (X)->type != SYM_RATIO && (X)->type != SYM_PRODUCT && (X)->type != SYM_SUBSCRIPT && (X)->type != SYM_INDETERMINATE) { \
		printf("\\left("); \
		sym_print(e, X); \
		printf("\\right)"); \
	} else { \
		sym_print(e, X); \
	}

void
sym_print(sym_env e, sym s)
{
	if (!s) return;

	switch (s->type) {
	case SYM_NEGATIVE:
		printf("-");
		PRINT_PARENTHESIZED(s->a);
		break;
	case SYM_CONSTANT:
		switch (s->constant) {
		case CONST_E: printf("e"); break;
		case CONST_PI: printf("\\pi "); break;
		}
		break;
	case SYM_ABS:
		printf("\\left|");
		sym_print(e, s->a);
		printf("\\right|");
		break;
	case SYM_LOGARITHM:
		if (s->a->type == SYM_CONSTANT
		    && s->a->constant == CONST_E) {
			printf("\\ln");
		} else {
			printf("\\log_{");
			sym_print(e, s->a);
			printf("}");
		}

		printf("\\left(");
		sym_print(e, s->b);
		printf("\\right)");
		break;
	case SYM_DERIVATIVE:
		printf("\\frac{d}{d%s}{", s->wrt);
		sym_print(e, s->deriv);
		printf("}");
		break;
	case SYM_PRODUCT:
		PRINT_PARENTHESIZED(s->a);
		if (s->b->type != SYM_POWER
		    || (s->b->type == SYM_POWER
		        && s->b->a->type != SYM_INDETERMINATE)) {
			printf("\\cdot ");
		}
		PRINT_PARENTHESIZED(s->b);
		break;
	case SYM_CROSS:
		PRINT_PARENTHESIZED(s->a);
		printf("\\times");
		PRINT_PARENTHESIZED(s->b);
		break;
	case SYM_SUM:
		sym_print(e, s->a);
		printf("+");
		sym_print(e, s->b);
		break;
	case SYM_DIFFERENCE:
		sym_print(e, s->a);
		printf("-");
		PRINT_PARENTHESIZED(s->b);
		break;
	case SYM_POWER:
		printf("{");

		if (s->a->type != SYM_INDETERMINATE) {
			printf("\\left(");
			sym_print(e, s->a);
			printf("\\right)");
		} else {
			sym_print(e, s->a);
		}

		printf("}^{");
		sym_print(e, s->b);
		printf("}");
		break;
	case SYM_SUBSCRIPT:
		printf("{");
		sym_print(e, s->a);
		printf("}_{");
		sym_print(e, s->b);
		printf("}");
		break;
	case SYM_EQUALITY:
		printf("{");
		sym_print(e, s->a);
		printf("=");
		sym_print(e, s->b);
		printf("}");
		break;
	case SYM_RATIO:
		printf("\\frac{"), sym_print(e, s->a);
		printf("}{"), sym_print(e, s->b);
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
			sym_print(e, c->coefficient);

			printf(strlen(s->indeterminate) != 1
			       ? "\\text{%s}^{" : "%s^{",
			       s->indeterminate);

			sym_print(e, c->exponent);
			printf("}");
			c = c->next;
			if (c) printf("+");
		}
	} break;
	case SYM_VECTOR: {
		printf("<");
		for (unsigned i = 0; i < s->len; i++) {
			sym_print(e, s->vector[i]);
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

		int i = 0;

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
