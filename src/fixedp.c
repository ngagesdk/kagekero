/** @file fixedp.c
 *
 *  A cross-platform engine with native Nokia N-Gage compatibility.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#include "fixedp.h"
#include "fix32.h"

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
