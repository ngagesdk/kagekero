/** @file intro.h
 *
 *  A minimalist, cross-platform puzzle-platformer, designed
 *  especially for the Nokia N-Gage.
 *
 *  Copyright (c) 2026, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#ifndef INTRO_H
#define INTRO_H

#include <SDL3/SDL.h>

#include "core.h"
#include "utils.h"

bool load_intro(core_t *nc);
bool update_intro(core_t *nc);
bool handle_intro_button_down(core_t *nc, button_t button);
void unload_intro(core_t *nc);

#endif /* INTRO_H */
