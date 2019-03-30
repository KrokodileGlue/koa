#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "derivative.h"
#include "arithmetic.h"
#include "simplify.h"
#include "cas.h"

sym
sym_new(sym_env e, enum sym_type type)
{
	sym sym = malloc(sizeof *sym);

	(void)e;                /* Suppress warning. */
	memset(sym, 0, sizeof *sym);
	sym->type = type;
	sym->sign = 1;

	return sym;
}

void
sym_free(sym_env e, sym s)
{
	switch (s->type) {
	case SYM_LOGARITHM:
	case SYM_POWER:
	case SYM_SUBSCRIPT:
	case SYM_EQUALITY:
	case SYM_PRODUCT:
	case SYM_CROSS:
	case SYM_DIFFERENCE:
	case SYM_RATIO:
	case SYM_SUM:
		sym_free(e, s->a), sym_free(e, s->b);
		break;
	case SYM_NUM:
		math_free(s->sig), math_free(s->exp);
		break;
	case SYM_INDETERMINATE:
	case SYM_VARIABLE:
		free(s->text);
		break;
	case SYM_VECTOR:
		for (unsigned i = 0; i < s->len; i++)
			sym_free(e, s->vector[i]);
		free(s->vector);
		break;
	case SYM_NEGATIVE:
	case SYM_ABS: sym_free(e, s->a);
	case SYM_CONSTANT: break;
	default:
		printf("Unimplemented free-er: %d\n", s->type);
	}

	free(s);
}

sym_env
sym_env_new(enum priority prio, unsigned precision)
{
	sym_env env = malloc(sizeof *env);

	memset(env, 0, sizeof *env);
	env->precision = precision;
	env->prio = prio;

	/* Initialize global variables. */
	sym_init_differentiation(env);
	sym_init_simplification(env);

	return env;
}

int sym_hist_add(enum priority prio,
                 sym_env env,
                 const sym sym,
                 const char *fmt,
                 ...)
{
	if (prio < env->prio) return -1;

	va_list args;
	va_start(args, fmt);

	env->hist = realloc(env->hist, ++env->hist_len * sizeof *env->hist);
	vsnprintf(env->hist[env->hist_len - 1].msg, 1024, fmt, args);
	env->hist[env->hist_len - 1].result = sym_copy(env, sym);

	va_end(args);

	if (sym) env->num_eq++;

	return env->num_eq;
}

int
sym_cmp(sym_env e, sym a, sym b)
{
	(void)e;                /* Suppress warning. */
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
sym_add(sym_env e, sym a, sym b)
{
	sym sym = sym_new(e, SYM_NUM);

	if (b->type == SYM_RATIO) {
		sym->type = SYM_RATIO;

		if (b->sign) {
			sym->a = sym_add(e, b->a, sym_mul(e, a, b->b));
		} else {
			sym->a = sym_sub(e, sym_mul(e, a, b->b), b->a);
		}

		sym->b = sym_copy(e, b->b);
		return sym;
	}

	if (a->type == SYM_RATIO) {
		sym->type = SYM_RATIO;
		sym->a = sym_add(e, a->a, sym_mul(e, b, a->b));
		sym->b = sym_copy(e, a->b);
		return sym;
	}

	if (a->type == SYM_VECTOR && b->type == SYM_VECTOR) {
		if (a->len < b->len) {
			struct sym *c = a;
			a = b;
			b = c;
		}

		sym = sym_copy(e, a);

		for (unsigned i = 0; i < b->len; i++)
			sym->vector[i] = sym_add(e, a->vector[i], b->vector[i]);

		return sym;
	}

	if (a->type != SYM_NUM || b->type != SYM_NUM) {
		sym->type = SYM_SUM;
		sym->a = sym_copy(e, a);
		sym->b = sym_copy(e, b);
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
		if (sym_cmp(e, a, b) > 0) {
			sym->sig = math_usub(a->sig, b->sig);
			sym->sign = 0;
		} else {
			sym->sig = math_usub(b->sig, a->sig);
		}

		return sym;
	}

	if (a->sign && !b->sign) {
		if (sym_cmp(e, a, b) > 0) {
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
fractionize(sym_env e, sym s)
{
	sym sym = sym_new(e, SYM_RATIO);

	sym->b = sym_new(e, SYM_NUM);
	math_uinc(&sym->b->sig);
	sym->a = s;

	return sym;
}

sym
sym_negate(sym_env e, const sym s)
{
	sym ret = sym_new(e, SYM_NEGATIVE);
	ret->a = sym_copy(e, s);
	return ret;
}

sym
sym_sub(sym_env e, sym a, sym b)
{
	sym sym = sym_new(e, SYM_NUM);

	if ((a->type != SYM_NUM && a->type != SYM_NEGATIVE)
	    || (a->type != SYM_NUM && a->type != SYM_NEGATIVE)) {
		sym->type = SYM_DIFFERENCE;
		sym->a = sym_copy(e, a);
		sym->b = sym_copy(e, b);
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

	/* Fix. */

	bool asign = !(a->type == SYM_NEGATIVE);
	bool bsign = !(b->type == SYM_NEGATIVE);

	if (!asign) a = a->a;
	if (!bsign) b = b->a;

	/* Stuff starts. */

	if (!asign && bsign) {
		sym->sig = math_uadd(a->sig, b->sig);
	}

	if (a->sign && !bsign) {
		sym->sig = math_uadd(a->sig, b->sig);
	}

	if (asign && bsign) {
		if (math_ucmp(a->sig, b->sig) > 0) {
			sym->sig = math_usub(a->sig, b->sig);
		} else {
			sym->sig = math_usub(b->sig, a->sig);
			sym = sym_negate(e, sym);
		}
	}

	if (!asign && !bsign) {
		if (sym_cmp(e, a, b) > 0) {
			sym->sig = math_usub(a->sig, b->sig);
			sym = sym_negate(e, sym);
		} else {
			sym->sig = math_usub(a->sig, b->sig);
		}
	}

	return sym;
}

sym
sym_copy(sym_env e, sym s)
{
	if (!s) return NULL;
	sym sym = sym_new(e, s->type);
	sym->sign = s->sign;

	switch (s->type) {
	case SYM_CONSTANT:
		sym->constant = s->constant;
		break;
	case SYM_LOGARITHM:
	case SYM_PRODUCT:
	case SYM_CROSS:
	case SYM_DIFFERENCE:
	case SYM_SUM:
	case SYM_POWER:
	case SYM_SUBSCRIPT:
	case SYM_EQUALITY:
	case SYM_RATIO:
		sym->a = sym_copy(e, s->a);
		sym->b = sym_copy(e, s->b);
		break;
	case SYM_NUM:
		sym->sig = math_ucopy(s->sig);
		sym->exp = math_ucopy(s->exp);
		break;
	case SYM_INDETERMINATE:
		sym->text = malloc(strlen(s->text) + 1);
		strcpy(sym->text, s->text);
		break;
	case SYM_VECTOR: {
		sym->vector = malloc(s->len * sizeof s->vector);
		for (unsigned i = 0; i < s->len; i++)
			sym->vector[i] = sym_copy(e, s->vector[i]);
		sym->len = s->len;
	} break;
	case SYM_DERIVATIVE:
		sym->wrt = malloc(strlen(s->wrt) + 1);
		strcpy(sym->wrt, s->wrt);
		sym->deriv = sym_copy(e, s->deriv);
		break;
	case SYM_NEGATIVE:
	case SYM_ABS:
		sym->a = sym_copy(e, s->a);
		break;
	default:
		printf("unimplemented copier %d\n", s->type);
		exit(1);
	}

	return sym;
}

sym
sym_dec(sym_env e, sym s)
{
	sym b = sym_new(e, SYM_NUM);
	b->sig->x = 1;
	return sym_sub(e, s, b);
}

sym
sym_mul(sym_env e, const sym a, const sym b)
{
	if (a->type == SYM_RATIO) {
		sym sym = sym_copy(e, a);
		sym->a = sym_mul(e, sym->a, b);
		return sym;
	}

	if (b->type == SYM_RATIO) {
		sym sym = sym_copy(e, b);
		sym->a = sym_mul(e, sym->a, a);
		return sym;
	}

	sym sym = sym_new(e, SYM_NUM);

	if (b->type == SYM_SUM || b->type == SYM_DIFFERENCE) {
		sym->type = b->type;
		sym->a = sym_mul(e, b->a, a);
		sym->b = sym_mul(e, b->b, a);
		return sym;
	} else if (a->type == SYM_SUM || a->type == SYM_DIFFERENCE) {
		sym->type = a->type;
		sym->a = sym_mul(e, a->a, b);
		sym->b = sym_mul(e, a->b, b);
		return sym;
	} else if (a->type == SYM_VECTOR && b->type == SYM_VECTOR) {
		sym = NULL;

		for (unsigned i = 0; i < a->len; i++) {
			if (sym)
				sym = sym_add(e, sym, sym_mul(e, a->vector[i], b->vector[i]));
			else
				sym = sym_mul(e, a->vector[i], b->vector[i]);
		}

		return sym;
	} else if (a->type != SYM_NUM || b->type != SYM_NUM) {
		sym->type = SYM_PRODUCT;
		sym->a = sym_copy(e, a);
		sym->b = sym_copy(e, b);
		return sym;
	}

	sym->exp = math_uadd(a->exp, b->exp);
	sym->sig = math_umul(a->sig, b->sig);

	if (a->sign != b->sign) sym->sign = 0;

	return sym;
}

sym
sym_div_(sym_env e, const sym a, const sym b)
{
	assert(a->type == SYM_NUM && b->type == SYM_NUM);
	sym q = sym_new(e, SYM_NUM);
	num r = NULL;
	unsigned i = 0;
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
	} while (!math_uzero(r) && i < e->precision);

	if (a->sign != b->sign) q->sign = 0;

	return q;
}

sym
sqrt_(sym_env e, const sym s)
{
	sym a = DIGIT(0), b = sym_copy(e, s), c = b;

	for (int i = 0; i < 4; i++) {
		c = sym_div_(e, sym_add(e, a, b), DIGIT(2));
		sym c2 = sym_mul(e, c, c);
		sym a2 = sym_mul(e, a, a);
		if (sym_cmp(e, c2, s) == sym_cmp(e, a2, s))
			a = c;
		else
			b = c;
	}

	return c;
}

sym
sym_sqrt(sym_env e, const sym s)
{
	sym d = sym_copy(e, s), x = sqrt_(e, d);
	for (int i = 0; i < 10; i++)
		x = sym_sub(e, x, sym_div_(e, sym_sub(e, sym_mul(e, x, x), d),
					sym_mul(e, DIGIT(2), x)));
	return x;
}

sym
root_(sym_env e, const sym s, const sym n)
{
	sym a = DIGIT(0), b = sym_copy(e, s), c = b;

	for (int i = 0; i < 8; i++) {
		c = sym_div_(e, sym_add(e, a, b), DIGIT(2));

		sym c2 = sym_pow(e, c, n);
		sym a2 = sym_pow(e, a, n);

		if (sym_cmp(e, c2, s) == sym_cmp(e, a2, s))
			a = c;
		else
			b = c;
	}

	return c;
}

sym
sym_root_(sym_env e, const sym s, const sym n)
{
	sym d = sym_eval(e, s, 0), x = root_(e, d, n);

	for (int i = 0; i < 12; i++) {
//		printf("x0: "), sym_print(e, x), puts("");
//		printf("x1: "), sym_print(e, n), puts("");
//		printf("x2: "), sym_print(e, sym_pow(e, x, n)), puts("");
		x = sym_sub(e, x, sym_div_(e, sym_sub(e, sym_pow(e, x, n), d),
					sym_mul(e, n, sym_pow(e, x,
					sym_sub(e, n, DIGIT(1))))));
	}

	return x;
}

sym
sym_root(sym_env e, const sym a, const sym b)
{
	sym exp = NULL;

	if (b->type != SYM_RATIO) exp = fractionize(e, b);
	else exp = sym_copy(e, b);

	exp = sym_simplify(e, exp);
	sym sym = sym_new(e, SYM_POWER);
	sym->a = sym_eval(e, a, 0);
	sym->b = exp;

	struct sym *tmp = sym_root_(e, a, exp->b);
	return sym_pow(e, tmp, exp->a);
}

sym
sym_div(sym_env e, sym a, sym b)
{
	if (a->type != SYM_NUM || b->type != SYM_NUM) {
		sym sym = sym_new(e, SYM_RATIO);
		sym->a = a, sym->b = b;
		return sym;
	}

	return sym_div_(e, a, b);
#if 0
	sym d0 = sym_copy(e, b);
	sym k = sym_new(e, SYM_NUM);
	k = sym_add(e, k, DIGIT(1));

	while (sym_cmp(e, d0, DIGIT(1)) >= 0) {
		sym_print(e, d0), puts("");
		k = sym_mul(e, k, DIGIT(2));
		d0 = sym_div_(d0, DIGIT(2));
	}

	printf("d0: "), sym_print(e, d0), puts("");

	sym x0 = sym_sub(e, sym_div_(DIGIT(48), DIGIT(17)), sym_mul(e, sym_div_(DIGIT(32), DIGIT(17)), d0));

	for (int i = 0; i < 4; i++) {
		x0 = sym_mul(e, x0, sym_sub(e, DIGIT(2), sym_mul(e, d0, x0)));
	}

	x0 = sym_div_(x0, k);
	x0 = sym_mul(e, x0, a);

	return x0;
#endif
}

sym
sym_gcf(sym_env e, sym a, sym b)
{
	if (a->type != SYM_NUM || b->type != SYM_NUM)
		return NULL;

	if (math_ucmp(a->exp, &(struct num){1,0}) > 0
	 || math_ucmp(b->exp, &(struct num){1,0}) > 0)
		return NULL;

	sym A = sym_copy(e, a), B = sym_copy(e, b);

	if (sym_cmp(e, A, B) < 0) {
		sym tmp = A;
		A = B, B = tmp;
	}

	sym r = sym_new(e, SYM_NUM);

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

/* Wtf is this for? */
static sym
normalize_num(sym_env e, const sym a)
{
	if (!a) return NULL;
	if (!a->sig) return sym_copy(e, a);

	sym sym = sym_copy(e, a);
	math_unorm(&sym->exp);

	while (!sym->sig->x && !math_uzero(sym->exp)) {
		sym->sig = sym->sig->next;
		math_udec(&sym->exp);
	}

	return sym;
}

sym
sym_pow(sym_env e, const sym a, const sym b)
{
	if (a->type != SYM_NUM || b->type != SYM_NUM) {
		struct sym *tmp = sym_new(e, SYM_POWER);
		tmp->a = sym_copy(e, a);
		tmp->b = sym_copy(e, b);
		return tmp;
	}

	/* TODO: Must all be numbers. */

	sym B = normalize_num(e, b);
	num exp = B->sig;
	math_udec(&exp);

	if (1) {
		sym s = sym_copy(e, a);

		while (!math_uzero(exp)) {
			s = sym_mul(e, s, a);
			math_udec(&exp);
		}

		return s;
	}

	return NULL;
}

void
sym_print_history(sym_env env)
{
	puts("\\documentclass{article}");
	puts("\\usepackage{amsmath}");
	puts("\\begin{document}");

	for (int i = 0; i < env->hist_len; i++) {
		puts(env->hist[i].msg);
		if (!env->hist[i].result) continue;
		puts("\\begin{equation}");
		sym_print(env, env->hist[i].result);
		puts("\n\\end{equation}");
	}

	puts("\\end{document}");
}

sym
sym_factor(sym_env e, const sym s)
{
	return NULL;
}

void
sym_add_coefficient(sym_env env,
                    sym poly,
                    const sym coefficient,
                    const sym exponent)
{
	if (!poly->polynomial) {
		poly->polynomial = malloc(sizeof *poly->polynomial);
		memset(poly->polynomial, 0, sizeof *poly->polynomial);
		poly->polynomial->coefficient = sym_copy(env, coefficient);
		poly->polynomial->exponent = sym_copy(env, exponent);
	} else {
		struct polynomial *tmp = poly->polynomial;
		while (tmp->next) tmp = tmp->next;
		tmp->next = malloc(sizeof *tmp);
		tmp = tmp->next;
		memset(tmp, 0, sizeof *tmp);
		tmp->coefficient = sym_copy(env, coefficient);
		tmp->exponent = sym_copy(env, exponent);
	}
}

void
sym_set_coords(sym_env env, const char *coords)
{
	env->coords = malloc(strlen(coords) + 1);
	strcpy(env->coords, coords);
}

void
sym_add_vec(sym_env env, sym vec, unsigned idx, const sym x)
{
	if (idx <= vec->len) {
		vec->vector = realloc(vec->vector,
		                      (idx + 1) * sizeof *vec->vector);
		vec->len = idx + 1;
	}

	vec->vector[idx] = sym_copy(env, x);
}
