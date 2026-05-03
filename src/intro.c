/** @file intro.c
 *
 *  A minimalist, cross-platform puzzle-platformer, designed
 *  especially for the Nokia N-Gage.
 *
 *  Copyright (c) 2026, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#include <SDL3/SDL.h>

#include "core.h"
#include "intro.h"
#include "menu.h"
#include "utils.h"

bool load_intro(core_t *nc)
{
    return true;
}

bool update_intro(core_t *nc)
{
    if (!load_menu(nc))
    {
        return false;
    }
    nc->state = STATE_MENU;
    return true;
}

bool handle_intro_button_down(core_t *nc, button_t button)
{
    return true;
}

void unload_intro(core_t *nc)
{
}
