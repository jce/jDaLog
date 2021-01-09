#ifndef HAVE_TCMATH_H
#define HAVE_TCMATH_H

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef uint16_t float16_t;

typedef union float16_u
{
	float16_t v;
	struct
	{
		uint16_t sign:1;
		uint16_t exponent:5;
		uint16_t fraction:10;
	} m;
} float16_u;

typedef union float32_u
{
	float v;
	uint32_t as_uint32;
	struct
	{
		uint16_t sign:1;
		uint16_t exponent:8;
		uint32_t fraction:23;
	} m;
} float32_u;

typedef union float64_u
{
	double v;
	uint64_t as_uint64;
	struct
	{
		uint16_t sign:1;
		uint16_t exponent:11;
		uint64_t fraction:53;
	} m;
} float64_u;

float float16_to_float(float16_t);
double float16_to_double(float16_t);

float16_t float_to_float16(float);
float16_t double_to_float16(double);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // HAVE_TCMATH_H
