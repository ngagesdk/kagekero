/** @file aabb.h
 *
 *  A cross-platform engine with native Nokia N-Gage compatibility.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#ifndef AABB_H
#define AABB_H

#include <SDL3/SDL.h>

typedef struct aabb
{
    float bottom;
    float left;
    float right;
    float top;

} aabb_t;

bool do_intersect(aabb_t a, aabb_t b);

#endif /* AABB_H */
