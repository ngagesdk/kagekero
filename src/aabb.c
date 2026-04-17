/** @file aabb.c
 *
 *  A minimalist, cross-platform puzzle-platformer, designed
 *  especially for the Nokia N-Gage.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#include "aabb.h"

#include <SDL3/SDL.h>

/* Optimized AABB intersection test for Nokia N-Gage. */
static inline bool do_intersect_impl(aabb_t a, aabb_t b)
{
#if defined(__SYMBIAN32__) && (defined(__GNUC__) || defined(__ARMCC__))
    // ARM-optimized AABB test using conditional execution
    // Reduces pipeline stalls from branches
    int result = 1; // Assume intersection

    __asm__ volatile(
        "cmp    %1, %5          \n\t"  // Compare a.right < b.left
        "movlt  %0, #0          \n\t"  // If less, result = 0
        "cmp    %2, %6          \n\t"  // Compare a.left > b.right
        "movgt  %0, #0          \n\t"  // If greater, result = 0
        "cmp    %3, %7          \n\t"  // Compare a.bottom < b.top
        "movlt  %0, #0          \n\t"  // If less, result = 0
        "cmp    %4, %8          \n\t"  // Compare a.top > b.bottom
        "movgt  %0, #0          \n\t"  // If greater, result = 0
        : "+r" (result)
        : "r" (a.right), "r" (a.left), "r" (a.bottom), "r" (a.top),
          "r" (b.left), "r" (b.right), "r" (b.top), "r" (b.bottom)
        : "cc"
    );

    return (bool)result;
#else
    // Separating axis test - early exit on first separation found
    // This is faster than checking all conditions with AND
    if (a.right < b.left)   return false;
    if (a.left > b.right)   return false;
    if (a.bottom < b.top)   return false;
    if (a.top > b.bottom)   return false;

    return true;
#endif
}

bool do_intersect(aabb_t a, aabb_t b)
{
    return do_intersect_impl(a, b);
}
