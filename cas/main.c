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

int main(void)
{
	static char l[LINE_BUFFER_LENGTH];
	
	while (1) {
		printf("> ");
		if (!fgets(l, LINE_BUFFER_LENGTH, stdin)) return 1;
		sym s = sym_parse_latex(&(const char *){l}, 0);
//		sym_print(s), puts("");
		sym_print(sym_simplify(s)), puts("");
		sym_print(sym_eval(s)), puts("");
	}

	return 0;
}
