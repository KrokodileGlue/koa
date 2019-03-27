#pragma once

#include "cas.h"

sym sym_parse_number(sym_env env, const char **s);
sym sym_parse_latex(sym_env env, const char **s, int prec);
