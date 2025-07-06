/** @file overlay.h
 *
 *  A minimalist, cross-platform puzzle-platformer, designed
 *  especially for the Nokia N-Gage.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#ifndef OVERLAY_H
#define OVERLAY_H

#include <SDL3/SDL.h>

typedef enum menu_selection
{
    MENU_NONE = 0,
    MENU_RESUME,
    MENU_SETTINGS,
    MENU_QUIT

} menu_selection_t;

typedef struct overlay
{
    SDL_Surface *image;
    SDL_Surface *digits;
    SDL_Surface *font;
    SDL_Surface *coin_sprite;

    SDL_Surface *coin_count_canvas;
    SDL_Surface *life_count_canvas;
    SDL_Surface *menu_canvas;

    menu_selection_t menu_selection;
    Uint64 time_a;
    Uint64 time_b;
    Uint64 delta_time;
    Uint64 time_since_last_frame;

    int current_frame;

} overlay_t;

void destroy_overlay(overlay_t *ui);
bool load_overlay(overlay_t **ui);
bool render_overlay(int coins_left, int coins_max, int life_count, overlay_t *ui);

#endif // OVERLAY_H
