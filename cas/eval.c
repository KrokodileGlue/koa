#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "arithmetic.h"
#include "simplify.h"
#include "cas.h"

sym
sym_eval(sym_env e, const struct sym *s, enum priority prio)
{
	if (!s) return NULL;
	struct sym *S = sym_simplify(e, s);

	if (S->type == SYM_NUM) return S;

	switch (S->type) {
	case SYM_SUM:
		S = sym_add(e, sym_eval(e, S->a, prio), sym_eval(e, S->b, prio));
		break;
	case SYM_DIFFERENCE:
		S = sym_sub(e, sym_eval(e, S->a, prio), sym_eval(e, S->b, prio));
		break;
	case SYM_CROSS:
	case SYM_PRODUCT:
		S = sym_mul(e, sym_eval(e, S->a, prio), sym_eval(e, S->b, prio));
		break;
	case SYM_RATIO: {
		sym a = sym_eval(e, S->a, prio);
		sym b = sym_eval(e, S->b, prio);
		S = sym_div(e, a, b);
	} break;
	case SYM_POWER:
		S = sym_root(e, S->a, S->b);
		break;
	case SYM_INDETERMINATE:
	case SYM_NUM:
		S = sym_copy(e, S);
		break;
	case SYM_VECTOR:
		for (unsigned i = 0; i < S->len; i++)
			S->vector[i] = sym_eval(e, S->vector[i], prio);
		break;
	default:
		assert(false);
	}

	return S;
}
