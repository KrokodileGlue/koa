#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "simplify.h"
#include "latex.h"
#include "cas.h"

#define LINE_BUFFER_LENGTH 10000
static char l[LINE_BUFFER_LENGTH];

static int
line(sym_env env)
{
	printf("> ");
	if (!fgets(l, LINE_BUFFER_LENGTH, stdin)) return 1;

	sym s = NULL;

	switch (*l) {
	case '\n': break;
	case 'x': return 0;
	case 's':
		s = sym_parse_latex(env, &(const char *){l + 1}, 0);
		sym_print(env, sym_simplify(env, s)), puts("");
		sym_free(env, s);
		break;
	case 'e':
		s = sym_parse_latex(env, &(const char *){l + 1}, 0);
		sym_print(env, s), puts("");
		sym_free(env, s);
		break;
	case 'S':
		sym_print_history(env);
		break;
	case 'p':
		sscanf(l + 1, "%d", &env->precision);
		break;
	default:
		s = sym_parse_latex(env, &(const char *){l}, 0);
		sym_print(env, sym_eval(env, s, PRIO_ANSWER)), puts("");
		sym_free(env, s);
	}

	return 1;
}

int main(void)
{
	sym_env env = sym_env_new(0, 10);
	while (line(env));
}
