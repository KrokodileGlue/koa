#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "arithmetic.h"

int
sym_cmp_buf(const num a, const num b)
{
	for (int i = DIGITS - 1; i >= 0; i--) {
		if (a[i] > b[i]) return 1;
		if (b[i] > a[i]) return -1;
	}

	return 0;
}

void
sym_add_buf(const num a, const num b, num buf)
{
	uint64_t d = 0;

	for (int i = 0; i < DIGITS; i++) {
		uint64_t sum = (uint64_t)a[i] + (uint64_t)b[i] + d;
		buf[i] = sum % BASE;
		d = sum / BASE;
	}
}

void
sym_sub_buf(const num a, const num b, num buf)
{
	int64_t d = 0;

	for (int i = 0; i < DIGITS; i++) {
		int64_t sum = (int64_t)a[i] - (int64_t)b[i] + d;

		if (sum < 0) {
			buf[i] = (BASE + sum) % BASE;
			d = -1;
		} else {
			buf[i] = sum % BASE;
			d = 0;
		}
	}
}

bool
is_buf_zero(const num s)
{
	for (int i = 0; i < DIGITS; i++)
		if (s[i])
			return false;
	return true;
}

void
sym_mul_buf(const num a, const num b, num buf)
{
	memset(buf, 0, BUF_SIZE);

	for (int i = 0; i < DIGITS - 1; i++) {
		uint64_t carry = 0;

		for (int j = 0; j < DIGITS - 1; j++) {
			if (i + j >= DIGITS) break;
			uint64_t product = buf[i + j] + carry + a[i] * b[j];
			carry = product / BASE;
			buf[i + j] = product % BASE;
		}
	}
}

int
sym_len_buf(const num buf)
{
	for (int i = DIGITS - 1; i >= 0; i--)
		if (buf[i])
			return i + 1;
	return 0;
}

void
sym_lshift_buf(num n)
{
	for (int i = DIGITS - 1; i; i--)
		n[i] = n[i - 1];
	n[0] = 0;
}

void
sym_inc_buf(num n)
{
	sym_add_buf(n, (num){1, 0}, n);
}

void
sym_div_buf(const num a, const num b, num q, num r)
{
	memset(q, 0, BUF_SIZE), memset(r, 0, BUF_SIZE);

	for (int i = DIGITS - 1; i >= 0; i--) {
		sym_lshift_buf(r);
		r[0] = a[i];

		while (sym_cmp_buf(r, b) >= 0) {
			sym_sub_buf(r, b, r);
			q[i]++;
		}
	}
}

void
sym_print_buf(const num n)
{
	num N, ten = {10, 0};

	memcpy(N, n, sizeof N);
	char str[10000];
	int i = 0;

	str[0] = 0;

	while (!is_buf_zero(N)) {
		/* printf("[%d]", i), fflush(stdout); */
		num q, r;
		sym_div_buf(N, ten, q, r);

		str[i++] = r[0] + '0';
		str[i] = 0;

		memcpy(N, q, sizeof q);
	}

	if (!str[0]) str[0] = '0', str[1] = 0;

	size_t len = strlen(str);
	for (size_t i = 0; i < len / 2; i++) {
		char tmp = str[i];
		str[i] = str[len - i - 1];
		str[len - i - 1] = tmp;
	}

	printf("%s", str);
}
