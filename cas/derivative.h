#pragma once

#include <stdbool.h>
#include "cas.h"

void sym_init_differentiation(sym_env env);

bool sym_is_constant(sym_env env, const sym s);
bool sym_is_function_of(sym_env env, const sym s, const char *x);
sym sym_differentiate(sym_env env, const sym s, const char *x);
