#pragma once

#include "cas.h"

void sym_init_antidifferentiation(sym_env env);

sym sym_integrate(sym_env env, const sym s, const char *x);
