/** @file kero.h
 *
 *  A minimalist, cross-platform puzzle-platformer, designed
 *  especially for the Nokia N-Gage.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#ifndef KERO_H
#define KERO_H

#include <SDL3/SDL.h>

#include "map.h"

#define KERO_SIZE 32
#define KERO_HALF 16

typedef struct kero
{
    SDL_Surface *sprite;
    SDL_Surface *render_canvas;
    SDL_Surface *temp_canvas;

    Uint64 time_a;
    Uint64 time_b;
    Uint64 delta_time;
    Uint64 time_since_last_frame;
    Uint64 power_up_timeout;

    Uint32 state;
    Uint32 prev_state;

    float pos_x;
    float pos_y;
    float warp_x;
    float warp_y;
    float velocity_x;
    float velocity_y;

    int current_frame;
    int anim_fps;
    int anim_length;
    int anim_offset_x;
    int anim_offset_y;
    int sprite_offset;
    int heading;

    bool repeat_anim;
    bool jump_lock;

} kero_t;

void destroy_kero(kero_t *kero);
bool load_kero(kero_t **kero, map_t *map);
void update_kero(kero_t *kero, map_t *map, unsigned int *btn);
bool render_kero(kero_t *kero, map_t *map);

#endif // KERO_H
