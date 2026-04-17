/** @file fixedp.c
 *
 *  A minimalist, cross-platform puzzle-platformer, designed
 *  especially for the Nokia N-Gage.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#include "fixedp.h"
#include "fix32.h"

// Optimized wrapper functions for fixed-point operations.
// These reduce conversion overhead on Nokia N-Gage.

#if defined(__SYMBIAN32__)
// Mark as inline and use restrict for better optimization.
static inline float fp_div_impl(float a, float b) __attribute__((always_inline));
static inline float fp_mul_impl(float a, float b) __attribute__((always_inline));

static inline float fp_div_impl(float a, float b)
{
    // Direct conversion without intermediate variables
    return fix32_to_float(fix32_div(fix32_from_float(a), fix32_from_float(b)));
}

static inline float fp_mul_impl(float a, float b)
{
    // Direct conversion without intermediate variables
    return fix32_to_float(fix32_mul(fix32_from_float(a), fix32_from_float(b)));
}

float fp_div(float a, float b)
{
    return fp_div_impl(a, b);
}

float fp_mul(float a, float b)
{
    return fp_mul_impl(a, b);
}
#else
float fp_div(float a, float b)
{
    fix32_t fa = fix32_from_float(a);
    fix32_t fb = fix32_from_float(b);
    fix32_t r = fix32_div(fa, fb);

    return fix32_to_float(r);
}

float fp_mul(float a, float b)
{
    fix32_t fa = fix32_from_float(a);
    fix32_t fb = fix32_from_float(b);
    fix32_t r = fix32_mul(fa, fb);

    return fix32_to_float(r);
}
#endif
