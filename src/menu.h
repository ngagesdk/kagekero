/** @file menu.h
 *
 *  A minimalist, cross-platform puzzle-platformer, designed
 *  especially for the Nokia N-Gage.
 *
 *  Copyright (c) 2026, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#ifndef MENU_H
#define MENU_H

#include <SDL3/SDL.h>

#include "core.h"
#include "utils.h"

bool load_menu(core_t *nc);
bool update_menu(core_t *nc);
bool handle_menu_button_down(core_t *nc, button_t button);
void unload_menu(core_t *nc);

#endif /* MENU_H */
