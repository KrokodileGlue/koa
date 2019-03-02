#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "cas.h"
#include "arithmetic.h"

#define DIGIT(X) &(struct sym){SYM_NUM,1,.sig=&(struct num){X},0}

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

//	printf("adding: "), sym_print(a), printf("\t"), sym_print(b), puts("");

	if (b->type == SYM_RATIO) {
		sym->type = SYM_RATIO;
		sym->a = sym_add(b->a, sym_mul(a, b->b));
		sym->b = sym_copy(b->b);
		return sym;
	}

	if (a->type == SYM_RATIO) {
		sym->type = SYM_RATIO;
		sym->a = sym_add(a->a, sym_mul(b, a->b));
		sym->b = sym_copy(a->b);
		return sym;
	}

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
			sym->sig = math_usub(b->sig, a->sig);
		}

		return sym;
	}

	if (a->sign && !b->sign) {
		if (sym_cmp(a, b) > 0) {
			sym->sig = math_usub(a->sig, b->sig);
		} else {
			sym->sig = math_usub(b->sig, a->sig);
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
	math_uinc(&sym->b->sig);
	sym->a = s;

	return sym;
}

sym
sym_sub(sym a, sym b)
{
	if (a->type == SYM_RATIO || b->type == SYM_RATIO) {
		sym A = sym_copy(a), B = sym_copy(b);

		if (A->type != SYM_RATIO) A = fractionize(A);
		if (B->type != SYM_RATIO) B = fractionize(B);

		sym tmp = sym_new(SYM_RATIO);

		tmp->b = sym_mul(A->b, B->b);
		tmp->a = sym_sub(sym_mul(A->a, B->b),
				sym_mul(B->a, A->b));

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
	if (!s) return NULL;
	sym sym = sym_new(s->type);

//	printf("copying: "), sym_print(s), puts("");
	
	switch (s->type) {
	case SYM_PRODUCT:
	case SYM_DIFFERENCE:
	case SYM_SUM:
	case SYM_POWER:
	case SYM_RATIO:
		sym->a = sym_copy(s->a);
		sym->b = sym_copy(s->b);
		break;
	case SYM_NUM:
		sym->sig = math_ucopy(s->sig);
		sym->exp = math_ucopy(s->exp);
		sym->sign = s->sign;
		break;
	default:
		printf("unimplemented copier %d\n", s->type);
		exit(1);
	}


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
sym_mul(const sym a, const sym b)
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

	if (b->type == SYM_SUM || b->type == SYM_DIFFERENCE) {
		sym->type = b->type;
		sym->a = sym_mul(b->a, a);
		sym->b = sym_mul(b->b, a);
		return sym;
	} else if (a->type == SYM_SUM || a->type == SYM_DIFFERENCE) {
		sym->type = a->type;
		sym->a = sym_mul(a->a, b);
		sym->b = sym_mul(a->b, b);
		return sym;
	} else if (a->type != SYM_NUM || b->type != SYM_NUM) {
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
sym_div_(const sym a, const sym b)
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
	} while (!math_uzero(r) && i < 5);
	
	if (a->sign != b->sign) q->sign = 0;

	return q;
}

sym
sqrt_(const sym s)
{
	sym a = DIGIT(0), b = sym_copy(s), c = b;

	for (int i = 0; i < 4; i++) {
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

sym
sym_sqrt(const sym s)
{
	sym d = sym_copy(s), x = sqrt_(d);
	for (int i = 0; i < 10; i++)
		x = sym_sub(x, sym_div_(sym_sub(sym_mul(x, x), d),
					sym_mul(DIGIT(2), x)));
	return x;
}

sym
root_(const sym s, const sym n)
{
	sym a = DIGIT(0), b = sym_copy(s), c = b;

	for (int i = 0; i < 8; i++) {
		c = sym_div_(sym_add(a, b), DIGIT(2));

		sym c2 = sym_pow(c, n);
		sym a2 = sym_pow(a, n);
	
		if (sym_cmp(c2, s) == sym_cmp(a2, s))
			a = c;
		else
			b = c;
	}

	return c;
}

sym
sym_root_(const sym s, const sym n)
{
	sym d = sym_eval(s), x = root_(d, n);

	for (int i = 0; i < 12; i++) {
//		printf("x0: "), sym_print(x), puts("");
//		printf("x1: "), sym_print(n), puts("");
//		printf("x2: "), sym_print(sym_pow(x, n)), puts("");
		x = sym_sub(x, sym_div_(sym_sub(sym_pow(x, n), d),
					sym_mul(n, sym_pow(x,
					sym_sub(n, DIGIT(1))))));
	}
	
	return x;
}

sym
sym_root(const sym a, const sym b)
{
	sym exp = NULL;

	if (b->type != SYM_RATIO) exp = fractionize(b);
	else exp = sym_copy(b);

	exp = sym_simplify(exp);
	sym sym = sym_new(SYM_POWER);
	sym->a = sym_eval(a);
	sym->b = exp;

	struct sym *tmp = sym_root_(a, exp->b);
	return sym_pow(tmp, exp->a);
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
	if (!s) return;

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
	case SYM_POWER:
		PRINT_PARENTHESIZED(s->a);
		printf("^{");
		sym_print(s->b);
		printf("}");
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
		num tmp = math_ucopy(s->exp);
		num whole = s->sig;

		while (!math_uzero(tmp)) {
			if (whole) whole = whole->next;
			math_udec(&tmp);
		}

		if (!s->sign) putchar('-');
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
			fflush(stdout);
		}
	} break;
	}
}

sym
sym_gcf(sym a, sym b)
{
	if (a->type != SYM_NUM || b->type != SYM_NUM)
		return NULL;

	if (math_ucmp(a->exp, &(struct num){1,0}) > 0
	 || math_ucmp(b->exp, &(struct num){1,0}) > 0)
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
//	printf("simplifying: "), sym_print(s), puts("");
	sym sym = s;

	if (s->type != SYM_NUM) {
		s->a = sym_simplify(s->a);
		s->b = sym_simplify(s->b);
	}

	switch (s->type) {
	case SYM_NUM: break;
	case SYM_POWER: return sym_copy(s);
	case SYM_SUM: return sym_add(s->a, s->b);
	case SYM_DIFFERENCE: return sym_sub(s->a, s->b);
	case SYM_PRODUCT:
		sym = sym_mul(s->a, s->b);
		if (sym->type == SYM_PRODUCT)
			return sym;
		else
			return sym_simplify(sym);
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
			sym = sym_new(SYM_RATIO);
			sym->a = sym_simplify(s->a);
			sym->b = sym_simplify(s->b);
			break;
		}

		while (!math_uzero(s->a->exp)) {
			math_udec(&s->a->exp);
			math_ushift(&s->b->sig);
		}

		while (!math_uzero(s->b->exp)) {
			math_udec(&s->b->exp);
			math_ushift(&s->a->sig);
		}

		math_unorm(&s->b->exp);
		math_unorm(&s->a->exp);

		struct sym *gcf = sym_gcf(s->a, s->b);

		if (!gcf) break;

		sym = sym_new(SYM_RATIO);
		sym->a = sym_div(s->a, gcf);
		sym->b = sym_div(s->b, gcf);

		if (sym_cmp(sym->b, DIGIT(1)) == 0)
			return sym->a;
	} break;
	default: printf("unimplemented: %d\n", s->type);
	}

	return sym;
}

static sym
sym_normalize(const sym a)
{
	if (!a) return NULL;
	if (!a->sig) return sym_copy(a);

	sym sym = sym_copy(a);
	math_unorm(&sym->exp);

	while (!sym->sig->x && !math_uzero(sym->exp)) {
		sym->sig = sym->sig->next;
		math_udec(&sym->exp);
	}

	return sym;
}

sym
sym_pow(const sym a, const sym b)
{
	/* TODO: Must all be numbers. */

	sym B = sym_normalize(b);
	num exp = B->sig;
	math_udec(&exp);

	if (1) {
		sym s = sym_copy(a);

		while (!math_uzero(exp)) {
			s = sym_mul(s, a);
			math_udec(&exp);
		}

		return s;
	}

	return NULL;
}

sym
sym_eval(sym s)
{
	if (!s) return NULL;
	s = sym_simplify(s);

	switch (s->type) {
	case SYM_SUM:
		return sym_add(sym_eval(s->a), sym_eval(s->b));
	case SYM_DIFFERENCE:
		return sym_sub(sym_eval(s->a), sym_eval(s->b));
	case SYM_PRODUCT:
		return sym_mul(sym_eval(s->a), sym_eval(s->b));
	case SYM_RATIO:
		return sym_div(sym_eval(s->a), sym_eval(s->b));
	case SYM_POWER:
//		return sym_root(sym_eval(s->a), sym_eval(s->b));
		return sym_root(s->a, s->b);
	case SYM_NUM:
		return sym_copy(s);
	default:
		assert(false);
	}
	
	return NULL;
}
