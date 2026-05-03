/** @file fixedp.h
 *
 *  A minimalist, cross-platform puzzle-platformer, designed
 *  especially for the Nokia N-Gage.
 *
 *  Copyright (c) 2026, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#ifndef FIXEDP_H
#define FIXEDP_H

#include "config.h"
#include "fix32.h"

// Direct fixed-point macros for hot paths (eliminates function call + conversions)
// These work directly with float constants by converting at compile time

// Precomputed fixed-point constants (available on all platforms)
// Use these for frequently used constants in hot loops
#define GRAVITY_FP           ((fix32_t)(GRAVITY * 65536.0f))
#define ACCELERATION_FP      ((fix32_t)(ACCELERATION * 65536.0f))
#define DECELERATION_FP      ((fix32_t)(DECELERATION * 65536.0f))
#define JUMP_VELOCITY_FP     ((fix32_t)(JUMP_VELOCITY * 65536.0f))
#define MAX_FALLING_SPEED_FP ((fix32_t)(MAX_FALLING_SPEED * 65536.0f))
#define MAX_SPEED_FP         ((fix32_t)(MAX_SPEED * 65536.0f))
#define ACCELERATION_DASH_FP ((fix32_t)(ACCELERATION_DASH * 65536.0f))

#ifdef __SYMBIAN32__
// On N-Gage, use inline macros to avoid conversion overhead
// Format: FP_MUL_CONST(value, constant_as_float)
#define FP_MUL_CONST(val, const_f) \
    fix32_to_float(fix32_mul(fix32_from_float(val), fix32_from_float(const_f)))

#define FP_DIV_CONST(val, const_f) \
    fix32_to_float(fix32_div(fix32_from_float(val), fix32_from_float(const_f)))

// Fast multiply with precomputed fixed-point constant
#define FP_MUL_FP_CONST(val_f, const_fp) \
    fix32_to_float(fix32_mul(fix32_from_float(val_f), const_fp))

// Even faster: if value is already in integer form (like delta_time)
#define FP_MUL_INT_CONST(int_val, const_fp) \
    fix32_to_float(fix32_mul(fix32_from_int(int_val), const_fp))

#else
// Standard versions for other platforms (use native float operations)
#define FP_MUL_CONST(val, const_f)          ((val) * (const_f))
#define FP_DIV_CONST(val, const_f)          ((val) / (const_f))

// On non-Symbian platforms, just use the float constant directly
#define FP_MUL_FP_CONST(val_f, const_fp)    ((val_f) * fix32_to_float(const_fp))
#define FP_MUL_INT_CONST(int_val, const_fp) ((float)(int_val) * fix32_to_float(const_fp))
#endif

// Original functions kept for compatibility
float fp_div(float a, float b);
float fp_mul(float a, float b);

#endif // FIXEDP_H
