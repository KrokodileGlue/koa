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
sym_cmp(const sym a, const sym b)
{
	if (a->type != SYM_INT || b->type != SYM_INT)
		return false;

	return sym_cmp_buf(a->buf, b->buf);
}

sym
sym_add(const sym a, const sym b)
{
	if (a->type != SYM_INT || b->type != SYM_INT)
		return NULL;

	sym sym = sym_new(SYM_INT);

	if (a->sign && b->sign) {
		sym_add_buf(a->buf, b->buf, sym->buf);
		return sym;
	}

	if (!a->sign && !b->sign) {
		sym_add_buf(a->buf, b->buf, sym->buf);
		sym->sign = 0;
		return sym;
	}

	if (!a->sign && b->sign) {
		if (sym_cmp(a, b) > 0) {
			sym_sub_buf(a->buf, b->buf, sym->buf);
			sym->sign = 0;
		} else {
			sym_sub_buf(b->buf, a->buf, sym->buf);
		}

		return sym;
	}

	if (a->sign && !b->sign) {
		if (sym_cmp(a, b) > 0) {
			sym_sub_buf(a->buf, b->buf, sym->buf);
		} else {
			sym_sub_buf(b->buf, a->buf, sym->buf);
			sym->sign = 0;
		}

		return sym;
	}

	return sym;
}

static sym
fractionize(const sym s)
{
	sym sym = sym_new(SYM_RATIO);

	sym->b = sym_new(SYM_INT);
	sym->b->buf[0] = 1;
	sym->a = s;

	return sym;
}

sym
sym_sub(const sym a, const sym b)
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

	if (a->type != SYM_INT || b->type != SYM_INT)
		return NULL;

	sym sym = sym_new(SYM_INT);

	if (!a->sign && b->sign) {
		sym_add_buf(a->buf, b->buf, sym->buf);
		sym->sign = 0;
		return sym;
	}

	if (a->sign && !b->sign) {
		sym_add_buf(a->buf, b->buf, sym->buf);
		sym->sign = 0;
		return sym;
	}

	if (a->sign && b->sign) {
		if (sym_cmp(a, b) > 0) {
			sym_sub_buf(a->buf, b->buf, sym->buf);
		} else {
			sym_sub_buf(b->buf, a->buf, sym->buf);
			sym->sign = 0;
		}
	}

	if (!a->sign && !b->sign) {
		if (sym_cmp(a, b) > 0) {
			sym_sub_buf(a->buf, b->buf, sym->buf);
			sym->sign = 0;
		} else {
			sym_sub_buf(b->buf, a->buf, sym->buf);
		}
	}

	return sym;
}

static bool
sym_is_zero(const sym s)
{
	if (s->type != SYM_INT) return false;

	for (int i = 0; i < DIGITS; i++)
		if (s->buf[i])
			return false;

	return true;
}

sym
sym_copy(const sym s)
{
	sym sym = sym_new(s->type);
	memcpy(sym->buf, s->buf, sizeof s->buf);
	return sym;
}

sym
sym_dec(const sym s)
{
	sym b = sym_new(SYM_INT);
	b->buf[0] = 1;
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

	if (a->type != SYM_INT || b->type != SYM_INT)
		return NULL;

	sym sym = sym_new(SYM_INT);
	sym_mul_buf(a->buf, b->buf, sym->buf);
	if (a->sign != b->sign) sym->sign = 0;

	return sym;
}

sym
sym_div(const sym a, const sym b)
{
	num buf;
	sym_div_buf(a->buf, b->buf, buf, (num){0});

	sym sym = sym_new(SYM_INT);
	memcpy(sym->buf, buf, sizeof buf);

	return sym;
}

#define PRINT_PARENTHESIZED(X)	  \
	if ((X)->type != SYM_INT && (X)->type != SYM_RATIO) { \
		printf("\\left("); \
		sym_print(X); \
		printf("\\right)"); \
	} else { \
		sym_print(X); \
	}

void
sym_print(const sym s)
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
	case SYM_INT:
		if (!sym_is_zero(s) && !s->sign) putchar('-');
		sym_print_buf(s->buf);
		break;
	case SYM_RATIO:
		printf("\\frac{"), sym_print(s->a);
		printf("}{"), sym_print(s->b);
		printf("}");
		break;
	case SYM_VARIABLE:
		printf("%s", s->variable);
		break;
	}
}

sym
sym_gcf(const sym a, const sym b)
{
	if (a->type != SYM_INT || b->type != SYM_INT)
		return NULL;

	sym A = sym_copy(a), B = sym_copy(b);

	if (sym_cmp(A, B) < 0) {
		sym tmp = A;
		A = B, B = tmp;
	}

	sym r = sym_new(SYM_INT);

	while (1) {
		num tmp;
		sym_div_buf(A->buf, B->buf, tmp, r->buf);
		memcpy(A->buf, tmp, BUF_SIZE);
		if (sym_is_zero(r)) return B;
		memcpy(A->buf, B->buf, BUF_SIZE);
		memcpy(B->buf, r->buf, BUF_SIZE);
	}

	return B;
}

static bool
intcmp(const sym s, int k)
{
	if (!s) return false;

	for (int i = DIGITS - 1; i >= 0; i--)
		if (s->buf[i])
			return false;

	if (s->buf[0] == k)
		return true;

	return false;
}

sym
sym_simplify(const sym s)
{
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

		if (s->a->type != SYM_INT || s->b->type != SYM_INT) {
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

	return sym;
}

sym
sym_eval(const sym s)
{
	sym sym = NULL;

	if (!s) return NULL;

	switch (s->type) {
	case SYM_SUM:
		sym = sym_add(sym_eval(s->a), sym_eval(s->b));
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
	case SYM_INT:
		sym = s;
		break;
	default:
		assert(false);
	}

	return sym_simplify(sym);
}
