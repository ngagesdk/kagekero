/** @file overclock.h
 *
 *  A minimalist, cross-platform puzzle-platformer, designed
 *  especially for the Nokia N-Gage.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#ifndef OVERCLOCK_H
#define OVERCLOCK_H

#include <SDL3/SDL.h>

void disable_overclock(void);
void enable_overclock(void);
bool is_overclock_enabled(void);

#endif // OVERCLOCK_H
