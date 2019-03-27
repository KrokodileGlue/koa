#include <ctype.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "arithmetic.h"

void math_free(num n)
{
	while (n) {
		num tmp = n->next;
		free(n);
		n = tmp;
	}
}

void math_upush(num *dig, int32_t k)
{
	if (!*dig) {
		num tmp = malloc(sizeof *tmp);
		tmp->next = NULL;
		tmp->prev = NULL;
		tmp->x = k;
		*dig = tmp;
		return;
	}

	num tmp = *dig;
	while (tmp->next) tmp = tmp->next;

	tmp->next = malloc(sizeof *tmp);
	tmp->next->next = NULL;
	tmp->next->prev = tmp;
	tmp->next->x = k;
}

void math_uprepend(num *dig, int32_t k)
{
	if (!*dig) {
		math_upush(dig, k);
		return;
	}

	num tmp = malloc(sizeof *tmp);
	tmp->next = *dig;
	tmp->prev = NULL;
	tmp->x = k;
	(*dig)->prev = tmp;
	*dig = tmp;
}

int math_ulen(num n)
{
	int len = 0;

	if (!n) return 0;
	while (n->next) n = n->next;

	while (n && n->x == 0) {
		n = n->prev;
	}

	while (n) {
		n = n->prev;
		len++;
	}

	return len;
}

num math_uadd(num a, num b)
{
	uint32_t d = 0;
	num buf = NULL;

	while (a && b) {
		uint64_t sum = (uint64_t)a->x + (uint64_t)b->x + d;
		math_upush(&buf, sum % BASE);
		d = sum / BASE;
		a = a->next, b = b->next;
	}

	while (a) {
		uint64_t sum = (uint64_t)a->x + d;
		math_upush(&buf, sum % BASE);
		d = sum / BASE;
		a = a->next;
	}

	while (b) {
		uint64_t sum = (uint64_t)b->x + d;
		math_upush(&buf, sum % BASE);
		d = sum / BASE;
		b = b->next;
	}

	while (d) {
		math_upush(&buf, d % BASE);
		d /= BASE;
	}

	return buf;
}

num math_usub(num a, num b)
{
	num buf = NULL;
	int64_t d = 0;

	while (a && b) {
		int64_t sum = (int64_t)a->x - (int64_t)b->x + d;

		if (sum < 0) {
			math_upush(&buf, BASE + sum);
			d = -1;
		} else {
			math_upush(&buf, sum % BASE);
			d = 0;
		}

		a = a->next, b = b->next;
	}

	while (a) {
		int64_t sum = (int64_t)a->x + d;

		if (sum < 0) {
			math_upush(&buf, BASE + sum);
			d = -1;
		} else {
			math_upush(&buf, sum % BASE);
			d = 0;
		}

		a = a->next;
	}

	while (b) {
		int64_t sum = (int64_t)b->x + d;

		if (sum < 0) {
			math_upush(&buf, BASE + sum);
			d = -1;
		} else {
			math_upush(&buf, sum % BASE);
			d = 0;
		}

		b = b->next;
	}

	return buf;
}

void math_usub2(num a, num b)
{
	int64_t d = 0;

	while (a && b) {
		int64_t sum = (int64_t)a->x - (int64_t)b->x + d;

		if (sum < 0) {
			a->x = (BASE + sum) % BASE;
			d = -1;
		} else {
			a->x = sum % BASE;
			d = 0;
		}

		a = a->next, b = b->next;
	}

	while (a) {
		int64_t sum = (int64_t)a->x + d;

		if (sum < 0) {
			a->x = (BASE + sum) % BASE;
			d = -1;
		} else {
			a->x = sum % BASE;
			d = 0;
		}

		a = a->next;
	}

	while (b) {
		int64_t sum = (int64_t)b->x + d;

		if (sum < 0) {
			a->x = (BASE + sum) % BASE;
			d = -1;
		} else {
			a->x = sum % BASE;
			d = 0;
		}

		b = b->next;
	}
}

void math_ushift(num *n)
{
	math_uprepend(n, 0);
}

int math_ucmp(num a, num b)
{
	if (!a && !b) return 0;
	if (!a) return -1;
	if (!b) return 1;
	if (math_ulen(a) > math_ulen(b)) return 1;
	if (math_ulen(a) < math_ulen(b)) return -1;
	while (a->next) a = a->next;
	while (b->next) b = b->next;
	while (a && a->x == 0) a = a->prev;
	while (b && b->x == 0) b = b->prev;
	while (a && b) {
		if (a->x > b->x) return 1;
		if (a->x < b->x) return -1;
		a = a->prev, b = b->prev;
	}
	return 0;
}

void math_unorm(num *n)
{
	num tmp = *n;

	if (!tmp) return;
	while (tmp->next) tmp = tmp->next;

	while (tmp->x == 0) {
		tmp = tmp->prev;

		if (!tmp) {
			*n = NULL;
			return;
		}

		tmp->next = NULL;
	}
}

void math_uinc(num *n)
{
	uint64_t d = 1;

	if (!*n) {
		math_upush(n, 1);
		return;
	}

	num tmp = *n;

	while (d) {
		uint64_t sum = tmp->x + d;

		if (sum >= BASE) {
			sum -= BASE;
			d = sum + 1;
		} else {
			d = 0;
		}

		tmp->x = sum;
		if (!tmp->next && d) math_upush(&tmp, 0);
		tmp = tmp->next;
	}
}

void math_udec(num *n)
{
	uint64_t d = 1;
	if (!*n) return;
	num tmp = *n;

	while (d) {
		int64_t sum = tmp->x - d;

		if (sum < 0) {
			sum += BASE;
			d = BASE - sum;
		} else {
			d = 0;
		}

		tmp->x = sum;
		tmp = tmp->next;
	}
}

void math_udiv(num a, num b, num *q, num *r)
{
	*q = NULL, *r = NULL;

	if (!a) return;
	while (a->next) a = a->next;

	while (a) {
		math_uprepend(r, a->x);
		math_ushift(q);

		while (math_ucmp(*r, b) >= 0) {
//			math_usub2(*r, b);
			*r = math_usub(*r, b);
			math_uinc(q);
		}

		a = a->prev;
	}

	math_unorm(q), math_unorm(r);
}

void math_udiv2(num *a, num b, num *r)
{
	*r = NULL;

	num q = *a;

	if (!q) return;
	while (q->next) q = q->next;

	while (q) {
		math_uprepend(r, q->x);
		q->x = 0;

		while (math_ucmp(*r, b) >= 0) {
			math_usub2(*r, b);
			q->x++;
		}

		q = q->prev;
	}

	math_unorm(a), math_unorm(r);
}

num math_ucopy(num n)
{
	num tmp = NULL;

	while (n) {
		math_upush(&tmp, n->x);
		n = n->next;
	}

	return tmp;
}

void math_print(num n)
{
	if (!n) {
		putchar('0');
		return;
	}

#if 0
	int i = 1;
	while (n->next) n = n->next;
	while (n && n->x == 0) n = n->prev;
	while (n) {
		putchar(n->x + '0');
		if (n->prev && (math_ulen(n) - i++) % 3 == 0)
			putchar(',');
		n = n->prev;
	}
#else
	num tmp = math_ucopy(n);

	char buf[2000];
	int i = 0;

	while (math_ucmp(tmp, NULL)) {
		num r = NULL;
		math_udiv2(&tmp, &(struct num){10,0,0}, &r);
		buf[i++] = (r ? r->x : 0) + '0';
		buf[i] = 0;
	}

	for (int i = 0; i < (int)strlen(buf); i++)
		putchar(buf[strlen(buf) - i - 1]);
#endif
}

num math_umul(num a, num b)
{
	num p = NULL, p1;
	math_upush(&p, 0), p1 = p;

	while (a) {
		uint64_t carry = 0;
		num btmp = b, p2 = p1;

		do {
			uint64_t product = p2->x + carry
				+ a->x * (b ? b->x : 0);
			carry = product / BASE;
			p2->x = product % BASE;
			if (b) b = b->next;
			if (!p2->next) math_upush(&p, 0);
			p2 = p2->next;
		} while (carry || b);

		b = btmp, a = a->next, p1 = p1->next;
	}

	math_unorm(&p);
	return p;
}

int math_uzero(num n)
{
	for (; n; n = n->next)
		if (n->x)
			return 0;
	return 1;
}
