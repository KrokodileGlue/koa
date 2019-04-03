#include "util.h"
#include "cas.h"

bool
binary(const struct sym *s)
{
	switch (s->type) {
	case SYM_RATIO:
	case SYM_PRODUCT:
	case SYM_CROSS:
	case SYM_SUM:
	case SYM_DIFFERENCE:
	case SYM_POWER:
	case SYM_SUBSCRIPT:
	case SYM_EQUALITY:
	case SYM_LOGARITHM:
		return true;
	default:
		return false;
	}
}

bool
unary(const struct sym *s)
{
	switch (s->type) {
	case SYM_DERIVATIVE:
	case SYM_NEGATIVE:
	case SYM_ABS:
		return true;
	default:
		return false;
	}
}

bool
sym_is_zero(const struct sym *s)
{
	if (s->type != SYM_NUM) return false;
	return math_uzero(s->sig);
}
