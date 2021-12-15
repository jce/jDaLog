#include "tcmath.h"

// https://en.wikipedia.org/wiki/Floating-point_arithmetic
// debug
//#include "stdio.h"

float float16_to_float(float16_t f)
{
        float32_u rv;
        rv.as_uint32 =  SIGN_TO_FLOAT32(     (uint32_t) FLOAT16_TO_SIGN(f) );
        rv.as_uint32 |= EXPONENT_TO_FLOAT32( (uint32_t) FLOAT16_TO_EXPONENT(f) + 127 - 15 );
        rv.as_uint32 |= FRACTION_TO_FLOAT32( (uint32_t) FLOAT16_TO_FRACTION(f) << (24 - 11) );
        return rv.v;
}

double float16_to_double(float16_t f)
{
        //printf("0x%x %u %u %u\n", f, FLOAT16_TO_SIGN(f), FLOAT16_TO_EXPONENT(f), FLOAT16_TO_FRACTION(f));
        float64_u rv;
        rv.as_uint64 =  SIGN_TO_FLOAT64(     (uint64_t) FLOAT16_TO_SIGN(f) );
        rv.as_uint64 |= EXPONENT_TO_FLOAT64( (uint64_t) FLOAT16_TO_EXPONENT(f) + 1023 - 15 );
        rv.as_uint64 |= FRACTION_TO_FLOAT64( (uint64_t) FLOAT16_TO_FRACTION(f) << (53 - 11) );
        //printf("%f\n", rv.v);
        return rv.v;

}


float16_t float_to_float16(float f)
{
        float32_u u = {.v = f};
        float16_t rv;
        rv =  SIGN_TO_FLOAT16(     FLOAT32_TO_SIGN(u.as_uint32) );
        rv |= EXPONENT_TO_FLOAT16( FLOAT32_TO_EXPONENT(u.as_uint32) + 15 - 127 );
        rv |= FRACTION_TO_FLOAT16( FLOAT32_TO_FRACTION(u.as_uint32) >> (24 - 11) );
        return rv;
}

float16_t double_to_float16(double f)
{
        float64_u u = {.v = f};
        float16_t rv;
        rv =  SIGN_TO_FLOAT16(     FLOAT64_TO_SIGN(u.as_uint64) );
        rv |= EXPONENT_TO_FLOAT16( FLOAT64_TO_EXPONENT(u.as_uint64) + 1023 - 15 );
        rv |= FRACTION_TO_FLOAT16( FLOAT64_TO_FRACTION(u.as_uint64) >> (53 - 11) );
        return rv;
}

