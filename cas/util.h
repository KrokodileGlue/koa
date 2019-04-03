#pragma once

#include <stdbool.h>

#include "cas.h"
#include "arithmetic.h"

bool binary(const struct sym *s);
bool unary(const struct sym *s);
bool sym_is_zero(const struct sym *s);
