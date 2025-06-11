/** @file hero.h
 *
 *  A cross-platform engine with native Nokia N-Gage compatibility.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#ifndef HERO_H
#define HERO_H

#include <SDL3/SDL.h>

#include "map.h"

typedef enum hero_state
{
    STATE_IDLE = 0x00000000,
    STATE_RUN = 0x00000001,
    STATE_HURT = 0x00000002,
    STATE_JUMP = 0x00000004,
    STATE_CLIMB = 0x0000008,
    STATE_CROUCH = 0x00000010

} hero_state_t;

typedef struct hero
{
    SDL_Surface *sprite;
    SDL_Surface *render_canvas;
    SDL_Surface *temp_canvas;

    Uint64 time_a;
    Uint64 time_b;
    Uint64 delta_time;
    Uint64 time_since_last_frame;

    Uint32 state;
    Uint32 prev_state;

    float pos_x;
    float pos_y;
    float accel_x;
    float accel_y;
    float velocity_x;
    float velocity_y;
    float max_velocity_x;

    int current_frame;
    int anim_fps;
    int anim_length;
    int anim_offset;
    int sprite_offset;
    int heading;

} hero_t;

void destroy_hero(hero_t *hero);
bool load_hero(hero_t **hero, float pos_x, float pos_y);
void update_hero(hero_t *hero, map_t *map, unsigned int *btn);
bool render_hero(hero_t *hero, map_t *map);
void set_hero_state(hero_t *hero, hero_state_t state);

#endif // HERO_H
