/** @file menu.c
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
#include "game.h"
#include "menu.h"
#include "utils.h"

bool load_menu(core_t *nc)
{
    unload_menu(nc);

    if (!load_texture_from_file("splash.png", &nc->temp_a, nc->renderer))
    {
        SDL_Log("Unable to load title screen image: %s", SDL_GetError());
        return false;
    }

    if (!load_texture_from_file("title.png", &nc->temp_b, nc->renderer))
    {
        SDL_Log("Unable to load title screen logo: %s", SDL_GetError());
        return false;
    }

    SDL_GetTextureSize(nc->temp_b, &nc->temp_b_w, &nc->temp_b_h);

    return true;
}

bool update_menu(core_t *nc)
{
    return true;
}

bool handle_menu_button_down(core_t *nc, button_t button)
{
    if (button == BTN_5)
    {
        nc->state = STATE_GAME;

        unload_menu(nc);
        if (!load_game(nc))
        {
            SDL_Log("Failed to load game.");
            return false;
        }
    }

    return true;
}

void unload_menu(core_t *nc)
{
    if (nc->temp_a)
    {
        SDL_DestroyTexture(nc->temp_a);
        nc->temp_a = NULL;
    }

    if (nc->temp_b)
    {
        SDL_DestroyTexture(nc->temp_b);
        nc->temp_b = NULL;
    }
}