#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "cas.h"
#include "arithmetic.h"

sym
sym_new(enum sym_type type)
{
	sym sym = malloc(sizeof *sym);

	memset(sym, 0, sizeof *sym);
	sym->type = type;
	sym->sign = 1;

	return sym;
}

int
sym_cmp(sym a, sym b)
{
	if (a->type != SYM_NUM || b->type != SYM_NUM)
		return false;

	while (math_ucmp(a->exp, b->exp) < 0) {
 		math_uinc(&a->exp);
		math_ushift(&a->sig);
	}

	while (math_ucmp(a->exp, b->exp) > 0) {
		math_uinc(&b->exp);
		math_ushift(&b->sig);
	}

	return math_ucmp(a->sig, b->sig);
}

sym
sym_add(sym a, sym b)
{
	sym sym = sym_new(SYM_NUM);

	if (a->type != SYM_NUM || b->type != SYM_NUM) {
		sym->type = SYM_SUM;
		sym->a = sym_copy(a);
		sym->b = sym_copy(b);
		return sym;
	}

	while (math_ucmp(a->exp, b->exp) < 0) {
 		math_uinc(&a->exp);
		math_ushift(&a->sig);
	}

	while (math_ucmp(a->exp, b->exp) > 0) {
		math_uinc(&b->exp);
		math_ushift(&b->sig);
	}

	sym->exp = math_ucopy(a->exp);

	if (a->sign && b->sign) {
		sym->sig = math_uadd(a->sig, b->sig);
		return sym;
	}

	if (!a->sign && !b->sign) {
		sym->sig = math_uadd(a->sig, b->sig);
		sym->sign = 0;
		return sym;
	}

	if (!a->sign && b->sign) {
		if (sym_cmp(a, b) > 0) {
			sym->sig = math_usub(a->sig, b->sig);
			sym->sign = 0;
		} else {
			sym->sig = math_usub(a->sig, b->sig);
		}

		return sym;
	}

	if (a->sign && !b->sign) {
		if (sym_cmp(a, b) > 0) {
			sym->sig = math_usub(a->sig, b->sig);
		} else {
			sym->sig = math_usub(a->sig, b->sig);
			sym->sign = 0;
		}

		return sym;
	}

	return sym;
}

static sym
fractionize(sym s)
{
	sym sym = sym_new(SYM_RATIO);

	sym->b = sym_new(SYM_NUM);
	sym->b->sig->x = 1;
	sym->a = s;

	return sym;
}

sym
sym_sub(sym a, sym b)
{
	if (a->type == SYM_RATIO || b->type == SYM_RATIO) {
		sym A = sym_copy(a), B = sym_copy(b);

		if (A->type != SYM_RATIO)
			A = fractionize(A);

		if (B->type != SYM_RATIO)
			B = fractionize(B);

		sym tmp = sym_new(SYM_RATIO);

		tmp->b = sym_mul(A->b, B->b);
		tmp->a = sym_sub(sym_mul(A->a, B->b), sym_mul(B->a, A->b));

		return tmp;
	}

	sym sym = sym_new(SYM_NUM);

	if (a->type != SYM_NUM || b->type != SYM_NUM) {
		sym->type = SYM_DIFFERENCE;
		sym->a = sym_copy(a);
		sym->b = sym_copy(b);
		return sym;
	}

	while (math_ucmp(a->exp, b->exp) < 0) {
		math_uinc(&a->exp);
		math_ushift(&a->sig);
	}

	while (math_ucmp(b->exp, a->exp) < 0) {
		math_uinc(&b->exp);
		math_ushift(&b->sig);
	}

	sym->exp = math_ucopy(a->exp);

	if (!a->sign && b->sign) {
		sym->sig = math_uadd(a->sig, b->sig);
		sym->sign = 0;
		return sym;
	}

	if (a->sign && !b->sign) {
		sym->sig = math_uadd(a->sig, b->sig);
		sym->sign = 1;
		return sym;
	}

	if (a->sign && b->sign) {
		if (sym_cmp(a, b) > 0) {
			sym->sig = math_usub(a->sig, b->sig);
		} else {
			sym->sig = math_usub(b->sig, a->sig);
			sym->sign = 0;
		}
	}

	if (!a->sign && !b->sign) {
		if (sym_cmp(a, b) > 0) {
			sym->sig = math_usub(a->sig, b->sig);
			sym->sign = 0;
		} else {
			sym->sig = math_usub(a->sig, b->sig);
		}
	}

	return sym;
}

static bool
sym_is_zero(sym s)
{
	if (s->type != SYM_NUM) return false;
	return math_uzero(s->sig);
}

sym
sym_copy(sym s)
{
	sym sym = sym_new(s->type);
	sym->sig = math_ucopy(s->sig);
	return sym;
}

sym
sym_dec(sym s)
{
	sym b = sym_new(SYM_NUM);
	b->sig->x = 1;
	return sym_sub(s, b);
}

sym
sym_mul(sym a, sym b)
{
	if (a->type == SYM_RATIO) {
		sym sym = sym_copy(a);
		sym->a = sym_mul(sym->a, b);
		return sym;
	}

	if (b->type == SYM_RATIO) {
		sym sym = sym_copy(b);
		sym->a = sym_mul(sym->a, a);
		return sym;
	}

	sym sym = sym_new(SYM_NUM);

	if (a->type != SYM_NUM || b->type != SYM_NUM) {
		sym->type = SYM_PRODUCT;
		sym->a = sym_copy(a);
		sym->b = sym_copy(b);
		return sym;
	}

	sym->exp = math_uadd(a->exp, b->exp);

	sym->sig = math_umul(a->sig, b->sig);
	if (a->sign != b->sign) sym->sign = 0;

	return sym;
}

sym
sym_div_(sym a, sym b)
{
	sym q = sym_new(SYM_NUM);
	num r = NULL;
	int i = 0;
	q->exp = math_usub(a->exp, b->exp);
	math_udiv(a->sig, b->sig, &q->sig, &r);

	do {
		r = math_umul(r, &(struct num){0,&(struct num){1,0},0});
		num tmp = NULL, d = NULL;
		math_udiv(r, b->sig, &d, &tmp);
		r = tmp;
		i++;
		math_uprepend(&q->sig, d ? d->x : 0);
		math_uinc(&q->exp);
	} while (!math_uzero(r) && i < 10);
	
	if (a->sign != b->sign) q->sign = 0;

	return q;
}

#define DIGIT(X) &(struct sym){SYM_NUM,1,.sig=&(struct num){X},0}

sym sqrt_(const sym s)
{
	sym a = DIGIT(0), b = sym_copy(s), c = b;

	for (int i = 0; i < 4; i++) {
//		puts("{");
//		sym_print(a), puts("");
//		sym_print(b), puts("");
//		puts("}");
		c = sym_div_(sym_add(a, b), DIGIT(2));
		sym c2 = sym_mul(c, c);
		sym a2 = sym_mul(a, a);
		if (sym_cmp(c2, s) == sym_cmp(a2, s))
			a = c;
		else
			b = c;
	}

	return c;
}

sym sym_sqrt(const sym s)
{
	sym d = sym_copy(s), x = sqrt_(d);
	printf("guess: "), sym_print(x), puts("");
	for (int i = 0; i < 4; i++) {
		x = sym_sub(x, sym_div_(sym_sub(sym_mul(x, x), d), sym_mul(DIGIT(2), x)));
	}
	return x;
}

sym
sym_div(sym a, sym b)
{
	return sym_div_(a, b);
	sym d0 = sym_copy(b);
	sym k = sym_new(SYM_NUM);
	k = sym_add(k, DIGIT(1));
	
	while (sym_cmp(d0, DIGIT(1)) >= 0) {
		sym_print(d0), puts("");
		k = sym_mul(k, DIGIT(2));
		d0 = sym_div_(d0, DIGIT(2));
	}
	
	printf("d0: "), sym_print(d0), puts("");

	sym x0 = sym_sub(sym_div_(DIGIT(48), DIGIT(17)), sym_mul(sym_div_(DIGIT(32), DIGIT(17)), d0));

	for (int i = 0; i < 4; i++) {
		x0 = sym_mul(x0, sym_sub(DIGIT(2), sym_mul(d0, x0)));
	}

	x0 = sym_div_(x0, k);
	x0 = sym_mul(x0, a);

	return x0;
}

#define PRINT_PARENTHESIZED(X)	  \
	if ((X)->type != SYM_NUM && (X)->type != SYM_RATIO) { \
		printf("\\left("); \
		sym_print(X); \
		printf("\\right)"); \
	} else { \
		sym_print(X); \
	}

void
sym_print(sym s)
{
	if (!s) {
		puts("");
		return;
	}

	switch (s->type) {
	case SYM_PRODUCT:
		PRINT_PARENTHESIZED(s->a);
		printf("\\cdot");
		PRINT_PARENTHESIZED(s->b);
		break;
	case SYM_SUM:
		PRINT_PARENTHESIZED(s->a);
		printf("+");
		PRINT_PARENTHESIZED(s->b);
		break;
	case SYM_DIFFERENCE:
		PRINT_PARENTHESIZED(s->a);
		printf("-");
		PRINT_PARENTHESIZED(s->b);
		break;
	case SYM_RATIO:
		printf("\\frac{"), sym_print(s->a);
		printf("}{"), sym_print(s->b);
		printf("}");
		break;
	case SYM_VARIABLE:
		printf("%s", s->variable);
		break;
	case SYM_NUM: {
//		puts("oifseojisef");
//		math_print(s->sig), puts("");
//		puts("oifseojisef");
		num tmp = math_ucopy(s->exp);
		num whole = s->sig;

		while (!math_uzero(tmp)) {
			if (whole) whole = whole->next;
			math_udec(&tmp);
		}

		if (whole && !s->sign) putchar('-');
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

		while (!math_uzero(r)) {
			num tmp = NULL, tmp2 = NULL;
			r = math_umul(r, &(struct num){10,0,0});
			math_udiv(r, div, &tmp, &tmp2);
			math_print(tmp);
			r = tmp2;
		}
#if 0
		num fraction = NULL;
		memcpy(fraction, s->sig, s->exp[0]);
		num div = NULL;
		div[s->exp[0]] = 1;

		num whole = NULL;
		memcpy(whole, s->sig + s->exp[0], BUF_SIZE - s->exp[0]);

		num r = NULL;
		sym_div_buf(fraction, div, &(num){0}, &r);

		sym_print_buf(whole);

		if (!math_uzero(r)) putchar('.');

		while (!math_uzero(r)) {
			num tmp = NULL, tmp2 = NULL;
			sym_mul_buf(r, (num){10,0}, tmp);
			memcpy(r, tmp, BUF_SIZE);
			sym_div_buf(r, div, tmp, tmp2);
			sym_print_buf(tmp);
			memcpy(r, tmp2, BUF_SIZE);
		}
#endif
	} break;
	}
}

sym
sym_gcf(sym a, sym b)
{
	if (a->type != SYM_NUM || b->type != SYM_NUM)
		return NULL;

	sym A = sym_copy(a), B = sym_copy(b);

	if (sym_cmp(A, B) < 0) {
		sym tmp = A;
		A = B, B = tmp;
	}

	sym r = sym_new(SYM_NUM);

	while (1) {
		num tmp = NULL;
		math_udiv(A->sig, B->sig, &tmp, &r->sig);
		A->sig = tmp;
		if (sym_is_zero(r)) return B;
		A->sig = B->sig;
		B->sig = r->sig;
	}

	return B;
}

sym
sym_simplify(sym s)
{
#if 0
	sym sym = NULL;

	switch (s->type) {
	case SYM_RATIO: {
		if (s->a->type == SYM_RATIO) {
			sym = sym_copy(s->a);
			sym->b = sym_mul(sym->b, s->b);
			return sym_simplify(sym);
		}

		if (s->b->type == SYM_RATIO) {
			sym = sym_new(SYM_RATIO);
			sym->a = sym_mul(s->a, s->b->b);
			sym->b = sym_copy(s->b->a);
			return sym_simplify(sym);
		}

		if (s->a->type != SYM_NUM || s->b->type != SYM_NUM) {
			sym = sym_copy(s);
			break;
		}

		struct sym *gcf = sym_gcf(s->a, s->b);

		if (intcmp(gcf, 1)) {
			sym = sym_copy(s->a);
			break;
		}

		sym = sym_new(SYM_RATIO);
		sym->a = sym_div(s->a, gcf);
		sym->b = sym_div(s->b, gcf);
	} break;
	default: sym = s;
	}
#endif

	//return sym;
	return s;
}

sym
sym_eval(sym s)
{
	sym sym = NULL;

	if (!s) return NULL;

	switch (s->type) {
	case SYM_SUM:
		//sym = sym_add(sym_eval(s->a), sym_eval(s->b));
		break;
	case SYM_DIFFERENCE:
		sym = sym_sub(sym_eval(s->a), sym_eval(s->b));
		break;
	case SYM_PRODUCT:
		sym = sym_mul(sym_eval(s->a), sym_eval(s->b));
		break;
	case SYM_RATIO:
		sym = sym_new(SYM_RATIO);

		sym->a = sym_eval(s->a);
		sym->b = sym_eval(s->b);
		break;
	case SYM_NUM:
		sym = s;
		break;
	default:
		assert(false);
	}

	return sym_simplify(sym);
}
