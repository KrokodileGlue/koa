#include <ctype.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>

#include <stdio.h>
#include <string.h>

#include "cas.h"
#include "arithmetic.h"

#define LINE_BUFFER_LENGTH 10000
char l[LINE_BUFFER_LENGTH];

int main(void)
{
	while (1) {
		printf("> ");
		if (!fgets(l, LINE_BUFFER_LENGTH, stdin)) exit(1);

		num x = NULL, y = NULL;
		num xexp = NULL, yexp = NULL;

		int dec = 0;

		_Bool signx = true, signy = true;

		if (strchr("+-/*s", *l)) {
			char *s = l;
			s++;

			while (*s && isspace(*s)) s++;
			if (*s == '-') signx = false, s++;
			while (*s && isspace(*s)) s++;
			while (*s && (isdigit(*s) || *s == '.')) {
				if (*s == '.') {
					dec = 1, s++;
					continue;
				}

				if (dec) math_uinc(&xexp);

				x = math_umul(x, &(struct num){10,0,0});
				x = math_uadd(x, &(struct num){*s - '0',0,0});

				s++;
			}

			dec = 0;

			while (*s && isspace(*s)) s++;
			if (*s == '-') signy = false, s++;
			while (*s && isspace(*s)) s++;
			while (*s && (isdigit(*s) || *s == '.')) {
				if (*s == '.') {
					dec = 1, s++;
					continue;
				}

				if (dec) math_uinc(&yexp);

				y = math_umul(y, &(struct num){10,0,0});
				y = math_uadd(y, &(struct num){*s - '0',0,0});
				s++;
			}
		}

		math_unorm(&x), math_unorm(&xexp);
		math_unorm(&y), math_unorm(&yexp);

		sym a = sym_new(SYM_NUM),
			b = sym_new(SYM_NUM);
		a->sig = x, a->exp = xexp, a->sign = signx;
		b->sig = y, b->exp = yexp, b->sign = signy;

		num r;
		sym q;

//		math_print(x), printf(", "), math_print(xexp), puts("");
//		math_print(y), printf(", "), math_print(yexp), puts("");

//		sym_print(a), puts("");
//		sym_print(b), puts("");

		switch (*l) {
		case 's':
			sym_print(sym_sqrt(a)), puts("");
			break;
		case '+':
			sym_print(sym_add(a, b)), puts("");
			break;
		case '-': sym_print(sym_sub(a, b)), puts(""); break;
		case '/': q = sym_div(a, b), sym_print(q), puts(""); break;
		case '*':
			sym_print(sym_mul(a, b)), puts("");
			break;
		case 'x': return 0;
		case '\n': continue;
		default: puts("No such command.");
		}
	}

	return 0;
}
