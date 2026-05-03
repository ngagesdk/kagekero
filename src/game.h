/** @file game.h
 *
 *  A minimalist, cross-platform puzzle-platformer, designed
 *  especially for the Nokia N-Gage.
 *
 *  Copyright (c) 2026, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#ifndef GAME_H
#define GAME_H

#include <SDL3/SDL.h>

#include "core.h"
#include "utils.h"

bool load_game(core_t *nc);
bool update_game(core_t *nc);
bool handle_game_button_down(core_t *nc, button_t button);
void unload_game(core_t *nc);

#endif /* GAME_H */
