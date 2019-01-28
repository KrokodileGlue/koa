#include <stdio.h>

#include "cas.h"
#include "arithmetic.h"

void test(void)
{
	struct sym *sym = sym_parse_latex("\\frac{\\left(42-\\frac{50}{20}\
\\right)\\cdot5}{\\frac{\\frac{\\frac{2}{5}}{7}}{2}}");
	sym_print(sym);
	struct sym *ret = sym_eval(sym);
	sym_print(ret);
}

int main(void)
{
	test();
	return 0;

	sym a = sym_new(SYM_INT);
	a->buf[0] = 50;
	sym b = sym_new(SYM_INT);
	b->buf[0] = 20;

	sym sum = sym_add(a, b);
	sym dif = sym_sub(a, b);
	sym pro = sym_mul(a, b);
	sym quo = sym_div(a, b);
	/* sym mod = sym_mod(a, b); */

	puts("Finished computations.");

	sym_print(a), puts("");
	sym_print(b), puts("");

	printf("a is %s than b.\n", sym_cmp(a, b) < 0
	       ? "smaller than" : sym_cmp(a, b) == 0
	       ? "equal to" : "greater than");

	printf("sum: "), sym_print(sum), puts("");
	printf("dif: "), sym_print(dif), puts("");
	printf("pro: "), sym_print(pro), puts("");
	printf("quo: "), sym_print(quo), puts("");
	/* printf("mod: "), sym_print(mod); */

	return 0;
}
