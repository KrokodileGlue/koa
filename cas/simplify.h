#pragma once

#include "cas.h"

void sym_init_simplification(sym_env env);
sym sym_simplify(sym_env env, const sym s);
sym sym_normalize(sym_env env, const sym s);
