/** @file aabb.c
 *
 *  A cross-platform engine with native Nokia N-Gage compatibility.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#include "aabb.h"

#include <SDL3/SDL.h>

bool do_intersect(aabb_t a, aabb_t b)
{
    float a_x = b.left - a.right;
    float a_y = b.top - a.bottom;
    float b_x = a.left - b.right;
    float b_y = a.top - b.bottom;

    if (0.f < a_x || 0.f < a_y) {
        return false;
    }

    if (0.f < b_x || 0.f < b_y) {
        return false;
    }

    return true;
}
