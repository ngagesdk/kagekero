/** @file core.h
 *
 *  A minimalist, cross-platform puzzle-platformer, designed
 *  especially for the Nokia N-Gage.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#ifndef CORE_H
#define CORE_H

#include <SDL3/SDL.h>

#include "kero.h"
#include "map.h"
#include "overlay.h"

typedef struct
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Event *event;

#if !defined __SYMBIAN32__
    SDL_Texture *frame;
    int frame_offset_x;
    int frame_offset_y;
    int screen_offset_x;
    int screen_offset_y;
#endif
    map_t *map;
    kero_t *kero;
    overlay_t *ui;

    int cam_x;
    int cam_y;

    int display_w;

    unsigned int btn;
    bool has_updated;

} core_t;

bool init(core_t **nc);
bool update(core_t *nc);
bool draw_scene(core_t *nc);
bool handle_events(core_t *nc);
void destroy(core_t *nc);

#endif // CORE_H
