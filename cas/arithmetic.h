#pragma once

#define BASE 10
typedef uint8_t digit;

struct num {
	digit x;
	struct num *next, *prev;
};

typedef struct num *num;

num math_uadd(num a, num b);

num math_usub(num a, num b);
void math_usub2(num a, num b);

num math_umul(num a, num b);

void math_udiv(num a, num b, num *q, num *r);
void math_udiv2(num *a, num b, num *r);

int math_ucmp(num a, num b);
void math_ushift(num *n);
void math_unorm(num *n);
void math_uinc(num *n);
void math_udec(num *n);

void math_print(num n);
int math_uzero(num n);
num math_ucopy(num n);
void math_upush(num *dig, int32_t k);
void math_uprepend(num *dig, int32_t k);
