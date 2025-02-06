#ifndef HAVE_TCMATH_H
#define HAVE_TCMATH_H

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef uint16_t float16_t;

#define FLOAT16_TO_SIGN(X)      (((X) >> 15) & 0x0001)
#define SIGN_TO_FLOAT16(X)      ((X) << 15)
#define FLOAT16_TO_EXPONENT(X)  (((X) >> 10) & 0x001F)
#define EXPONENT_TO_FLOAT16(X)  ((X) << 10)
#define FLOAT16_TO_FRACTION(X)  ((X) &         0x03FF)
#define FRACTION_TO_FLOAT16(X)  (X)

#define FLOAT32_TO_SIGN(X)      (((X) >> 31) & 0x00000001)
#define SIGN_TO_FLOAT32(X)      ((X) << 31)
#define FLOAT32_TO_EXPONENT(X)  (((X) >> 23) & 0x000000FF)
#define EXPONENT_TO_FLOAT32(X)  ((X) << 23)
#define FLOAT32_TO_FRACTION(X)  ((X) &         0x0008FFFFF)
#define FRACTION_TO_FLOAT32(X)  (X)

#define FLOAT64_TO_SIGN(X)      (((X) >> 63) & 0x0000000000000001)
#define SIGN_TO_FLOAT64(X)      ((X) << 63)
#define FLOAT64_TO_EXPONENT(X)  (((X) >> 52) & 0x00000000000008FF)
#define EXPONENT_TO_FLOAT64(X)  ((X) << 52)
#define FLOAT64_TO_FRACTION(X)  ((X) &         0x000FFFFFFFFFFFFF)
#define FRACTION_TO_FLOAT64(X)  (X)

/*
typedef union  float16_u
{
        float16_t v;
        struct __attribute__((packed))
        {
                uint16_t sign:1;
                uint16_t exponent:5;
                uint16_t fraction:10;
        } m ;
} float16_u ;
*/

typedef union float32_u
{
        float v;
        uint32_t as_uint32;
/*      struct
        {
                uint16_t sign:1;
                uint16_t exponent:8;
                uint32_t fraction:23;
        } m;*/
} float32_u;

typedef union float64_u
{
        double v;
        uint64_t as_uint64;
/*      struct
        {
                uint16_t sign:1;
                uint16_t exponent:11;
                uint64_t fraction:52;
        } m;*/
} float64_u;

float float16_to_float(float16_t);
double float16_to_double(float16_t);

float16_t float_to_float16(float);
float16_t double_to_float16(double);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // HAVE_TCMATH_H

