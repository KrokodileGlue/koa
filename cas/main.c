#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "derivative.h"
#include "simplify.h"
#include "pattern.h"
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
		s = sym_parse_latex(env, &(const char *){l + 1});
		sym_print(env, sym_simplify(env, s)), puts("");
		sym_free(env, s);
		break;
	case 'e':
		s = sym_parse_latex(env, &(const char *){l + 1});
		sym_print(env, s), puts("");
		sym_free(env, s);
		break;
	case 'S':
		sym_print_history(env);
		break;
	case 'p':
		sscanf(l + 1, "%u", &env->precision);
		break;
	case 'i': {
		char x[2] = {0};
		x[0] = l[1];
		s = sym_parse_latex(env, &(const char *){l + 2});
		sym_print(env, s), puts("");
		puts(sym_is_function_of(env, s, x) ? "true" : "false");
		sym_free(env, s);
	} break;
	case 'c': {
		s = sym_parse_latex(env, &(const char *){l + 1});
		sym_print(env, s), puts("");
		puts(sym_is_constant(env, s) ? "true" : "false");
		sym_free(env, s);
	} break;
	case 'm': {
		const char *text = l + 1;

		s = sym_parse_latex(env, &text);
		text++;

		sym b = sym_parse_latex(env, &text);

		printf("a: "), sym_print(env, s), puts("");
		printf("b: "), sym_print(env, b), puts("");

		struct map *m = sym_match(env, s, b);

		if (!m) { puts("no match!"); break; }

		for (int i = 0; i < m->len; i++) {
			printf("%s = ", m->indeterminate[i]);
			sym_print(env, m->symb[i]), puts("");
		}
	} break;
	case 'r': {
		const char *text = l + 1;

		s = sym_parse_latex(env, &text);
		text++;
		sym b = sym_parse_latex(env, &text);
		text++;
		sym c = sym_parse_latex(env, &text);

		printf("a: "), sym_print(env, s), puts("");
		printf("b: "), sym_print(env, b), puts("");
		printf("c: "), sym_print(env, c), puts("");

		struct map *m = sym_match(env, s, b);

		if (!m) { puts("no match!"); break; }

		for (int i = 0; i < m->len; i++) {
			printf("%s = ", m->indeterminate[i]);
			sym_print(env, m->symb[i]), puts("");
		}

		struct sym *ret = sym_replace(env, m, c);
		printf("result: "), sym_print(env, ret), puts("");
	} break;
	case 'd': {
		char x[2] = {0};
		x[0] = l[1];
		s = sym_parse_latex2(env, l + 2);
		sym_print(env, s), puts("");
		sym_print(env, sym_simplify(env, sym_differentiate(env, s, x))), puts("");
		sym_free(env, s);
	} break;
	default:
		s = sym_parse_latex(env, &(const char *){l});
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
