/** @file fixedp.cpp
 *
 *  A cross-platform engine with native Nokia N-Gage compatibility.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#ifdef __SYMBIAN32__
#include <3dtypes.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "fixedp.h"

float fp_div(float a, float b)
{
#ifdef __SYMBIAN32__
    TFixed fa = Real2Fix(a);
    TFixed fb = Real2Fix(b);

    if (!fb) {
        return 0.0f;
    }

    TFixed result = FixDiv(fa, fb);
    return Fix2Real(result);
#else
    if (!b) {
        return 0.0f;
    }
    return a / b;
#endif
}

float fp_mul(float a, float b)
{
#ifdef __SYMBIAN32__
    TFixed fa = Real2Fix(a);
    TFixed fb = Real2Fix(b);
    TFixed result = FixMul(fa, fb);
    return Fix2Real(result);
#else
    return a * b;
#endif
}

#ifdef __cplusplus
}
#endif
