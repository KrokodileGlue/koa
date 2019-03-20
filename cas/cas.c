#include <stdarg.h>
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
sym_new(sym_env e, enum sym_type type)
{
	sym sym = malloc(sizeof *sym);

	memset(sym, 0, sizeof *sym);
	sym->type = type;
	sym->sign = 1;

	return sym;
}

void
sym_free(sym_env e, sym s)
{
	switch (s->type) {
	case SYM_POWER:
	case SYM_PRODUCT:
	case SYM_DIFFERENCE:
	case SYM_RATIO:
	case SYM_SUM:
		sym_free(e, s->a), sym_free(e, s->b);
		break;
	case SYM_NUM:
		math_free(s->sig), math_free(s->exp);
		break;
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

	return env;
}

static int sym_hist_add(enum priority prio,
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

//	printf("adding: "), sym_print(e, a), printf("\t"), sym_print(e, b), puts("");

	if (b->type == SYM_RATIO) {
		sym->type = SYM_RATIO;
		sym->a = sym_add(e, b->a, sym_mul(e, a, b->b));
		sym->b = sym_copy(e, b->b);
		return sym;
	}

	if (a->type == SYM_RATIO) {
		sym->type = SYM_RATIO;
		sym->a = sym_add(e, a->a, sym_mul(e, b, a->b));
		sym->b = sym_copy(e, a->b);
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
sym_sub(sym_env e, sym a, sym b)
{
	if (a->type == SYM_RATIO || b->type == SYM_RATIO) {
		sym A = sym_copy(e, a), B = sym_copy(e, b);

		if (A->type != SYM_RATIO) A = fractionize(e, A);
		if (B->type != SYM_RATIO) B = fractionize(e, B);

		sym tmp = sym_new(e, SYM_RATIO);

		tmp->b = sym_mul(e, A->b, B->b);
		tmp->a = sym_sub(e, sym_mul(e, A->a, B->b),
				sym_mul(e, B->a, A->b));

		return tmp;
	}

	sym sym = sym_new(e, SYM_NUM);

	if (a->type != SYM_NUM || b->type != SYM_NUM) {
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
		if (sym_cmp(e, a, b) > 0) {
			sym->sig = math_usub(a->sig, b->sig);
		} else {
			sym->sig = math_usub(b->sig, a->sig);
			sym->sign = 0;
		}
	}

	if (!a->sign && !b->sign) {
		if (sym_cmp(e, a, b) > 0) {
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
sym_copy(sym_env e, sym s)
{
	if (!s) return NULL;
	sym sym = sym_new(e, s->type);

//	printf("copying: "), sym_print(e, s), puts("");

	switch (s->type) {
	case SYM_PRODUCT:
	case SYM_DIFFERENCE:
	case SYM_SUM:
	case SYM_POWER:
	case SYM_RATIO:
		sym->a = sym_copy(e, s->a);
		sym->b = sym_copy(e, s->b);
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

	exp = sym_simplify(e, exp, -1, 0);
	sym sym = sym_new(e, SYM_POWER);
	sym->a = sym_eval(e, a, 0);
	sym->b = exp;

	struct sym *tmp = sym_root_(e, a, exp->b);
	return sym_pow(e, tmp, exp->a);
}

sym
sym_div(sym_env e, sym a, sym b)
{
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

#define PRINT_PARENTHESIZED(X)	  \
	if ((X)->type != SYM_NUM && (X)->type != SYM_RATIO) { \
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
		sym_print(e, s->b);
		printf("}");
		break;
	case SYM_RATIO:
		printf("\\frac{"), sym_print(e, s->a);
		printf("}{"), sym_print(e, s->b);
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

static bool
trivial(const sym s)
{
	switch (s->type) {
	case SYM_DIFFERENCE:
	case SYM_SUM: return trivial(s->a) && trivial(s->b);
	case SYM_NUM: return true;
	case SYM_RATIO: return s->a->type == SYM_NUM && s->b->type == SYM_NUM;
	default: return false;
	}
}

sym
sym_simplify(sym_env e, sym s, int N, int suppress)
{
//	printf("simplifying: "), sym_print(s), puts("");
	sym sym = s;

	if (s->type == SYM_NUM) return s;

	int num = N;

	int triv = trivial(s);
	if (!triv && N < 0 && !suppress) {
		num = sym_hist_add(PRIO_ALGEBRA, e, s, "We wish to simplify (%d):", e->num_eq + 1);
	} else if (!triv && !suppress) {
		num = sym_hist_add(PRIO_ALGEBRA, e, s, "In calculating (%d), we need to simplify (%d).", N, e->num_eq + 1);
	}

	char *A = "numerator", *B = "denominator";
	if (s->type != SYM_RATIO)
		A = "lefthand side", B = "righthand side";
	struct sym *a = sym_simplify(e, s->a, num, 0);
	if (s->a != a && !trivial(s->a)) {
		sym_hist_add(0, e, a, "Simplify the %s of (%d).", A, num);
	}

	struct sym *b = sym_simplify(e, s->b, num, 0);
	if (s->b != b && !trivial(s->b)) {
		sym_hist_add(0, e, b, "Simplify the %s of (%d).", B, num);
	}

	s->a = a, s->b = b;

	switch (s->type) {
	case SYM_NUM: break;
	case SYM_POWER: return s;
	case SYM_SUM: return sym_add(e, s->a, s->b);
	case SYM_DIFFERENCE: return sym_sub(e, s->a, s->b);
	case SYM_PRODUCT:
		sym = sym_mul(e, s->a, s->b);
		if (sym->type != SYM_PRODUCT)
			sym = sym_simplify(e, sym, num, 0);
		break;
	case SYM_RATIO: {
		if (s->a->type == SYM_RATIO) {
			sym = sym_copy(e, s->a);
			sym->b = sym_mul(e, sym->b, s->b);
			sym = sym_simplify(e, sym, num, 0);
			break;
		}

		if (s->b->type == SYM_RATIO) {
			sym = sym_new(e, SYM_RATIO);
			sym->a = sym_mul(e, s->a, s->b->b);
			sym->b = sym_copy(e, s->b->a);
			sym = sym_simplify(e, sym, num, 0);
			break;
		}

		if (s->a->type != SYM_NUM || s->b->type != SYM_NUM) {
			sym = sym_new(e, SYM_RATIO);
			sym->a = sym_simplify(e, s->a, num, 0);
			sym->b = sym_simplify(e, s->b, num, 0);
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

		struct sym *gcf = sym_gcf(e, s->a, s->b);

		if (!gcf || sym_cmp(e, gcf, DIGIT(1)) == 0) break;

		sym = sym_new(e, SYM_RATIO);
		sym->a = sym_div(e, s->a, gcf);
		sym->b = sym_div(e, s->b, gcf);

		/* num = sym_hist_add(0, e, s, "We wish to simplify:"); */
		/* sym_hist_add(0, e, sym, "The ratio (%d) may be simplified:", num); */

		if (sym_cmp(e, sym->b, DIGIT(1)) == 0)
			sym = sym->a;
	} break;
	default: printf("unimplemented: %d\n", s->type);
	}

	if (!triv && sym != s)
		sym_hist_add(0, e, sym, "(%d) simplifies to (%d).", num, e->num_eq + 1);

	return sym;
}

static sym
sym_normalize(sym_env e, const sym a)
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
	/* TODO: Must all be numbers. */

	sym B = sym_normalize(e, b);
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

sym
sym_eval(sym_env e, sym s, enum priority prio)
{
	if (!s) return NULL;

	if (s->type == SYM_NUM) return sym_copy(e, s);

	bool triv = 0;

	int num = -1;

	if (!triv || prio == PRIO_ANSWER) {
		num = sym_hist_add(prio, e, s, "Our goal is to evaluate (%d).", e->num_eq + 1);
		s = sym_simplify(e, s, num, 1);
		num = sym_hist_add(prio, e, s, "The simplified form of (%d) is (%d).", num, e->num_eq + 1);
	}

	switch (s->type) {
	case SYM_SUM:
		s = sym_add(e, sym_eval(e, s->a, prio), sym_eval(e, s->b, prio));
		break;
	case SYM_DIFFERENCE:
		s = sym_sub(e, sym_eval(e, s->a, prio), sym_eval(e, s->b, prio));
		break;
	case SYM_PRODUCT:
		s = sym_mul(e, sym_eval(e, s->a, prio), sym_eval(e, s->b, prio));
		break;
	case SYM_RATIO: {
		sym a = sym_eval(e, s->a, prio);
		sym b = sym_eval(e, s->b, prio);
		s = sym_div(e, a, b);
		sym_hist_add(prio, e, s, "Straightforward evaluation of (%d) yields (%d).", num, e->num_eq + 1);
		return s;
	} break;
	case SYM_POWER:
		s = sym_root(e, s->a, s->b);
		break;
	case SYM_NUM:
		s = sym_copy(e, s);
		break;
	default:
		assert(false);
	}

	if (!triv)
		sym_hist_add(prio, e, s, "Our numeric solution for (%d) is:", num);

	return s;
}

void sym_print_history(sym_env env)
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
