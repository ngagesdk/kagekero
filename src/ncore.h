/** @file ncore.h
 *
 *  A cross-platform engine with native Nokia N-Gage compatibility.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#ifndef NCORE_H
#define NCORE_H

#include <SDL3/SDL.h>

#include "hero.h"
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
    hero_t *hero;

    int cam_x;
    int cam_y;

    unsigned int btn;

} ncore_t;

bool init_ncore(ncore_t **nc);
bool update_ncore(ncore_t *nc);
bool draw_ncore_scene(ncore_t *nc);
bool handle_ncore_events(ncore_t *nc);
void destroy_ncore(ncore_t *nc);

#endif // NCORE_H
