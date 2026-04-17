/** @file aabb.h
 *
 *  A minimalist, cross-platform puzzle-platformer, designed
 *  especially for the Nokia N-Gage.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#ifndef AABB_H
#define AABB_H

#include <SDL3/SDL.h>

// Optimized AABB structure for ARM cache efficiency.
// 16 bytes total, fits in a single cache line segment.
typedef struct aabb
{
    float left;   // 4 bytes; ordered for logical comparison flow
    float right;  // 4 bytes
    float top;    // 4 bytes
    float bottom; // 4 bytes

} aabb_t
#ifdef __SYMBIAN32__
    __attribute__((aligned(16))) // 16-byte alignment for efficient SIMD-like operations.
#endif
    ;

bool do_intersect(aabb_t a, aabb_t b);

#endif /* AABB_H */
