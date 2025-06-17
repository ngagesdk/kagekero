/** @file kagekeru.h
 *
 *  A minimalist, cross-platform puzzle-platformer, designed
 *  especially for the Nokia N-Gage.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#ifndef KAGEKERU_H
#define KAGEKERU_H

#include <SDL3/SDL.h>

#include "kero.h"
#include "map.h"

typedef struct
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Event *event;

#ifndef __SYMBIAN32__
    SDL_Texture *frame;
#endif
    map_t *map;
    kero_t *kero;

    int cam_x;
    int cam_y;

    unsigned int btn;

} kagekero_t;

bool init_kagekero(kagekero_t **nc);
bool update_kagekero(kagekero_t *nc);
bool draw_kagekero_scene(kagekero_t *nc);
bool handle_kagekero_events(kagekero_t *nc);
void destroy_kagekero(kagekero_t *nc);

#endif // KAGEKERU_H
