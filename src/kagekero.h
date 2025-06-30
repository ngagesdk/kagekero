/** @file kagekero.h
 *
 *  A minimalist, cross-platform puzzle-platformer, designed
 *  especially for the Nokia N-Gage.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#ifndef KAGEKERO_H
#define KAGEKERO_H

#include <SDL3/SDL.h>

#include "kero.h"
#include "map.h"

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

    int cam_x;
    int cam_y;

    int display_w;

    unsigned int btn;
    bool has_updated;

} kagekero_t;

bool init_kagekero(kagekero_t **nc);
bool update_kagekero(kagekero_t *nc);
bool draw_kagekero_scene(kagekero_t *nc);
bool handle_kagekero_events(kagekero_t *nc);
void destroy_kagekero(kagekero_t *nc);

#endif // KAGEKERO_H
