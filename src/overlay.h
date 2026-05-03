/** @file overlay.h
 *
 *  A minimalist, cross-platform puzzle-platformer, designed
 *  especially for the Nokia N-Gage.
 *
 *  Copyright (c) 2026, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#ifndef OVERLAY_H
#define OVERLAY_H

#include <SDL3/SDL.h>

#include "map.h"

typedef enum menu_selection
{
    MENU_NONE = 0,
    MENU_RESUME,
    MENU_SETTINGS,
    MENU_QUIT,
    MENU_MHZ,
    MENU_BACK

} menu_selection_t;

typedef struct overlay
{
    SDL_Texture *surface; // overlay.png as GPU texture
    SDL_Texture *digits;

    SDL_Texture *coin_count_canvas;
    SDL_Texture *life_count_canvas;
    SDL_Texture *menu_canvas;
    SDL_Texture *dialogue_canvas;

    menu_selection_t prev_selection;
    menu_selection_t menu_selection;
    Uint64 time_a;
    Uint64 time_b;
    Uint64 delta_time;
    Uint64 time_since_last_frame;

    int current_frame;
    int menu_canvas_offset;
    bool is_settings_menu;

#if defined(__SYMBIAN32__)
    // Dirty flags to track which UI elements need redraw.
    bool coin_count_dirty;
    bool life_count_dirty;
    bool menu_dirty;
    bool dialogue_dirty;
#endif

} overlay_t;

void destroy_overlay(overlay_t *ui);
bool load_overlay(map_t *map, overlay_t **ui, SDL_Renderer *renderer);
bool render_overlay(int coins_left, int coins_max, int life_count, map_t *map, overlay_t *ui, SDL_Renderer *renderer);
bool render_text(const char *text, bool alt_portrait, map_t *map, overlay_t *ui, SDL_Renderer *renderer);
bool render_text_ex(const char *text, bool alt_portrait, int portrait_x, int portrait_y, map_t *map, overlay_t *ui, SDL_Renderer *renderer);

#endif // OVERLAY_H
