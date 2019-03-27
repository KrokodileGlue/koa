#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "arithmetic.h"
#include "simplify.h"
#include "cas.h"

static bool
binary(const sym s)
{
	switch (s->type) {
	case SYM_NUM:
	case SYM_LIST:
	case SYM_VARIABLE:
	case SYM_POLYNOMIAL:
	case SYM_INDETERMINATE:
	case SYM_VECTOR:
		return false;
	default:
		return true;
	}
}

static sym
p1(sym_env e, const sym s)
{
	sym sym = s;

	if (s->type == SYM_NUM) return s;

	if (binary(s)) {
		s->a = sym_simplify(e, s->a);
		s->b = sym_simplify(e, s->b);
	}

	switch (s->type) {
	case SYM_INDETERMINATE:
	case SYM_NUM: break;
	case SYM_VECTOR:
		for (unsigned i = 0; i < s->len; i++)
			s->vector[i] = sym_simplify(e, s->vector[i]);
		break;
	case SYM_LIST:
		sym = NULL;

		for (unsigned i = 0; i < s->len; i++)
			s->vector[i] = sym_simplify(e, s->vector[i]);

		for (unsigned i = 0; i < s->len; i++)
			sym = sym ? sym_add(e, sym, s->vector[i]) : s->vector[i];

		sym = p1(e, sym);
		break;
	case SYM_EQUALITY:
	case SYM_SUBSCRIPT:
	case SYM_POWER: return s;
	case SYM_SUM: return sym_add(e, s->a, s->b);
	case SYM_DIFFERENCE: return sym_sub(e, s->a, s->b);
	case SYM_CROSS:
	case SYM_PRODUCT:
		if (s->a->type == SYM_PRODUCT) {
			if (s->a->b->type == SYM_INDETERMINATE) {
				sym = sym_copy(e, s->a);
				sym->a = sym_mul(e, sym->a, s->b);
			} else {
				sym = sym_copy(e, s->a);
				sym->b = sym_mul(e, sym->b, s->b);
			}

			return sym;
		}

		if (s->a->type == SYM_INDETERMINATE) {
			struct sym *tmp = s->b;
			s->b = s->a;
			s->a = tmp;
		}

		sym = sym_mul(e, s->a, s->b);
		if (!s->sign) sym->sign = !sym->sign;

		if (sym->type != SYM_PRODUCT)
			sym = sym_simplify(e, sym);
		break;
	case SYM_RATIO: {
		if (s->a->sign != s->b->sign) {
			s->a->sign = 1;
			s->b->sign = 1;
			s->sign = !s->sign;
		}

		if (s->a->type == SYM_RATIO) {
			sym = sym_copy(e, s->a);
			sym->sign ^= s->sign;
			sym->sign = !sym->sign;
			sym->b = sym_mul(e, sym->b, s->b);
			sym = sym_simplify(e, sym);
			break;
		}

		if (s->b->type == SYM_RATIO) {
			sym = sym_new(e, SYM_RATIO);
			sym->a = sym_mul(e, s->a, s->b->b);
			sym->b = sym_copy(e, s->b->a);
			sym->sign ^= s->sign;
			sym->sign = !sym->sign;
			sym = sym_simplify(e, sym);
			break;
		}

		if (s->a->type != SYM_NUM || s->b->type != SYM_NUM)
			break;

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

		struct sym *gcf = sym_gcf(e, s->a, s->b);

		if (!gcf || sym_cmp(e, gcf, DIGIT(1)) == 0) break;

		sym = sym_copy(e, s);

		sym->a = sym_div(e, sym->a, gcf);
		sym->b = sym_div(e, sym->b, gcf);

		if (sym_cmp(e, sym->b, DIGIT(1)) == 0)
			sym = sym->a, sym->sign = s->sign;
	} break;
	default: printf("unimplemented: %d\n", s->type);
	}

	return sym;
}

static sym
p2(sym_env e, const sym s)
{
	struct sym *sym = NULL;

	if (s->type == SYM_SUM || s->type == SYM_DIFFERENCE) {
		sym = sym_new(e, SYM_LIST);
		struct sym *a = p2(e, s->a), *b = p2(e, s->b);

		if (a->type == SYM_LIST) {
			for (unsigned i = 0; i < a->len; i++)
				sym_add_vec(e, sym, i, a->vector[i]);
		} else sym_add_vec(e, sym, 0, a);

		if (b->type == SYM_LIST) {
			for (unsigned i = 0; i < b->len; i++) {
				sym_add_vec(e, sym, sym->len, b->vector[i]);

				if (s->type == SYM_DIFFERENCE)
					sym->vector[sym->len - 1]->sign
						= !sym->vector[sym->len - 1]->sign;
			}
		} else {
			sym_add_vec(e, sym, sym->len, b);
			if (s->type == SYM_DIFFERENCE)
				sym->vector[sym->len - 1]->sign
					= !sym->vector[sym->len - 1]->sign;
		}

		return sym;
	}

	sym = sym_copy(e, s);

	if (binary(s)) {
		sym->a = p2(e, sym->a);
		sym->b = p2(e, sym->b);
	}

	return sym;
}

static sym
p3(sym_env e, const sym s)
{
	struct sym *sym = NULL;

	if (s->type == SYM_RATIO && s->a->type == SYM_LIST) {
		sym = sym_copy(e, s->a);

		for (unsigned i = 0; i < sym->len; i++) {
			struct sym *tmp = sym_new(e, SYM_RATIO);
			tmp->a = sym_copy(e, sym->vector[i]);
			tmp->b = sym_copy(e, s->b);
			sym->vector[i] = p1(e, tmp);
		}

		return sym;
	}

	sym = sym_copy(e, s);

	if (binary(s)) {
		sym->a = p3(e, sym->a);
		sym->b = p3(e, sym->b);
	}

	return sym;
}

sym
sym_normalize(sym_env e, const sym s)
{

}

sym
sym_simplify(sym_env e, const sym s)
{
	//return p3(e, p2(e, p1(e, s)));
	return p2(e, p1(e, s));
}
