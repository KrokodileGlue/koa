#pragma once

#include <stdbool.h>

#include "cas.h"
#include "arithmetic.h"

bool binary(const sym s);
bool unary(const sym s);
bool sym_is_zero(sym s);
