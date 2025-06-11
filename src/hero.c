/** @file hero.c
 *
 *  A cross-platform engine with native Nokia N-Gage compatibility.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#include "SDL3/SDL.h"

#include "config.h"
#include "fixedp.h"
#include "hero.h"
#include "utils.h"

void destroy_hero(hero_t *hero)
{
    if (hero) {
        if (hero->render_canvas) {
            SDL_DestroySurface(hero->render_canvas);
            hero->render_canvas = NULL;
        }

        if (hero->temp_canvas) {
            SDL_DestroySurface(hero->temp_canvas);
            hero->temp_canvas = NULL;
        }

        if (hero->sprite) {
            SDL_DestroySurface(hero->sprite);
            hero->sprite = NULL;
        }
    }
}

bool load_hero(hero_t **hero, float pos_x, float pos_y)
{
    *hero = (hero_t *)SDL_calloc(1, sizeof(hero_t));
    if (!*hero) {
        SDL_Log("Failed to allocate memory for hero");
        return false;
    }

    if (!load_surface_from_file("hero.png", &(*hero)->sprite)) {
        SDL_Log("Failed to load hero sprite");
        return false;
    }

    (*hero)->render_canvas = SDL_CreateSurface(32, 32, SDL_PIXELFORMAT_XRGB4444);
    if (!(*hero)->render_canvas) {
        SDL_Log("Error creating temporary surface: %s", SDL_GetError());
        return false;
    }

    (*hero)->temp_canvas = SDL_CreateSurface(32, 32, SDL_PIXELFORMAT_XRGB4444);
    if (!(*hero)->temp_canvas) {
        SDL_Log("Error creating temporary surface: %s", SDL_GetError());
        return false;
    }

    (*hero)->pos_x = pos_x;
    (*hero)->pos_y = pos_y;
    (*hero)->anim_fps = 1;

    set_hero_state(*hero, STATE_IDLE);

    return true;
}

void update_hero(hero_t *hero, map_t *map, unsigned int *btn)
{
    hero->time_b = hero->time_a;
    hero->time_a = SDL_GetTicks();

    if (hero->time_a > hero->time_b) {
        hero->delta_time = hero->time_a - hero->time_b;
    } else {
        hero->delta_time = hero->time_b - hero->time_a;
    }

    hero->time_since_last_frame += hero->delta_time;
    if (hero->time_since_last_frame >= (1000 / hero->anim_fps)) {
        hero->time_since_last_frame = 0;

        hero->current_frame += 1;
        if (hero->current_frame >= hero->anim_length - 1) {
            hero->current_frame = 0;
        }
    }

    if (check_bit(*btn, BTN_LEFT)) {
        hero->heading = 0;
    } else if (check_bit(*btn, BTN_RIGHT)) {
        hero->heading = 1;
    }

    if (hero->heading) {
        hero->sprite_offset = 0;
    } else {
        hero->sprite_offset = 96;
    }

    if (!*btn) {
        hero->state = STATE_IDLE;
    }

    if (hero->state == STATE_IDLE) {
        hero->anim_fps = 15;
        hero->anim_length = 11;
        hero->anim_offset = 0;
        return;
    }

    // Tbd.
}

bool render_hero(hero_t *hero, map_t *map)
{
    SDL_Rect src;
    src.x = (int)hero->pos_x - 8;
    src.y = (int)hero->pos_y - 8;
    src.w = 32;
    src.h = 32;

    SDL_BlitSurface(map->render_canvas, &src, hero->temp_canvas, NULL);

    src.x = hero->current_frame * 32;
    src.y = hero->anim_offset + hero->sprite_offset;

    SDL_BlitSurface(hero->sprite, &src, hero->temp_canvas, NULL);
    SDL_BlitSurface(hero->temp_canvas, NULL, hero->render_canvas, NULL);

    return true;
}

void set_hero_state(hero_t *hero, hero_state_t state)
{
    hero->prev_state = hero->state;

    if (STATE_IDLE == state) {
        hero->state = STATE_IDLE;
    } else {
        hero->state |= state;
    }

    if (hero->state != hero->prev_state) {
        hero->current_frame = 0;
        hero->time_since_last_frame = 0;
    }
}
