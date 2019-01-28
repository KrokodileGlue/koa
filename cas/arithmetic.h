#pragma once

#include "cas.h"

#define WORD(X) ((num){X,0})

int sym_cmp_buf(const num a, const num b);
void sym_add_buf(const num a, const num b, num buf);
void sym_sub_buf(const num a, const num b, num buf);
bool is_buf_zero(const num s);
void sym_mul_buf(const num a, const num b, num buf);
int sym_len_buf(const num buf);
void sym_mod_buf(const num a, const num b, num c);
void sym_inc_buf(num n);
void sym_div_buf(const num a, const num b, num q, num r);
void sym_print_buf(const num n);
