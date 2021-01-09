#include "tcmath.h"

// https://en.wikipedia.org/wiki/Floating-point_arithmetic


float float16_to_float(float16_t f)
{
	float16_u u = {.v = f};
	float32_u rv;
	rv.m.sign = u.m.sign;
	rv.m.exponent = u.m.exponent + 127 - 15;
	rv.m.fraction = ((uint32_t) u.m.fraction) << (24 - 11);
	return rv.v;
}

double float16_to_double(float16_t f)
{
	float16_u u = {.v = f};
	float64_u rv;
	rv.m.sign = u.m.sign;
	rv.m.exponent = u.m.exponent + 1023 - 15;
	rv.m.fraction = ((uint64_t) u.m.fraction) << (53 - 11);
	return rv.v;
}

float16_t float_to_float16(float f)
{
	float32_u u = {.v = f};
	float16_u rv;
	rv.m.sign = u.m.sign;
	rv.m.exponent = u.m.exponent + 15 - 127;
	rv.m.fraction = u.m.fraction >> (24 - 11);
	return rv.v;
}

float16_t double_to_float16(double f)
{
	float64_u u = {.v = f};
	float16_u rv;
	rv.m.sign = u.m.sign;
	rv.m.exponent = u.m.exponent + 15 - 1023;
	rv.m.fraction = u.m.fraction >> (53 - 11);
	return rv.v;
}
