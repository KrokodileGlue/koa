#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "arithmetic.h"
#include "simplify.h"
#include "cas.h"

sym
sym_eval(sym_env e, sym s, enum priority prio)
{
	if (!s) return NULL;
	s = sym_simplify(e, s);

	if (s->type == SYM_NUM) return sym_copy(e, s);

	switch (s->type) {
	case SYM_SUM:
		s = sym_add(e, sym_eval(e, s->a, prio), sym_eval(e, s->b, prio));
		break;
	case SYM_DIFFERENCE:
		s = sym_sub(e, sym_eval(e, s->a, prio), sym_eval(e, s->b, prio));
		break;
	case SYM_CROSS:
	case SYM_PRODUCT:
		s = sym_mul(e, sym_eval(e, s->a, prio), sym_eval(e, s->b, prio));
		break;
	case SYM_RATIO: {
		bool sign = s->sign;
		sym a = sym_eval(e, s->a, prio);
		sym b = sym_eval(e, s->b, prio);
		s = sym_div(e, a, b);
		if (!sign) s->sign = !s->sign;
		return s;
	} break;
	case SYM_POWER:
		s = sym_root(e, s->a, s->b);
		break;
	case SYM_INDETERMINATE:
	case SYM_NUM:
		s = sym_copy(e, s);
		break;
	case SYM_VECTOR:
		for (unsigned i = 0; i < s->len; i++)
			s->vector[i] = sym_eval(e, s->vector[i], prio);
		break;
	default:
		assert(false);
	}

	return s;
}
