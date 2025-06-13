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
    if (hero)
    {
        if (hero->render_canvas)
        {
            SDL_DestroySurface(hero->render_canvas);
            hero->render_canvas = NULL;
        }

        if (hero->temp_canvas)
        {
            SDL_DestroySurface(hero->temp_canvas);
            hero->temp_canvas = NULL;
        }

        if (hero->sprite)
        {
            SDL_DestroySurface(hero->sprite);
            hero->sprite = NULL;
        }
    }
}

bool load_hero(hero_t **hero, map_t *map)
{
    *hero = (hero_t *)SDL_calloc(1, sizeof(hero_t));
    if (!*hero)
    {
        SDL_Log("Failed to allocate memory for hero");
        return false;
    }

    if (!load_surface_from_file("hero.png", &(*hero)->sprite))
    {
        SDL_Log("Failed to load hero sprite");
        return false;
    }

    (*hero)->render_canvas = SDL_CreateSurface(HERO_SIZE, HERO_SIZE, SDL_PIXELFORMAT_XRGB4444);
    if (!(*hero)->render_canvas)
    {
        SDL_Log("Error creating temporary surface: %s", SDL_GetError());
        return false;
    }

    (*hero)->temp_canvas = SDL_CreateSurface(HERO_SIZE, HERO_SIZE, SDL_PIXELFORMAT_XRGB4444);
    if (!(*hero)->temp_canvas)
    {
        SDL_Log("Error creating temporary surface: %s", SDL_GetError());
        return false;
    }

    (*hero)->pos_x = (float)map->spawn_x;
    (*hero)->pos_y = (float)map->spawn_y;
    (*hero)->anim_fps = 1;

    set_hero_state(*hero, STATE_IDLE);

    return true;
}

void update_hero(hero_t *hero, map_t *map, unsigned int *btn)
{
    hero->time_b = hero->time_a;
    hero->time_a = SDL_GetTicks();

    if (hero->time_a > hero->time_b)
    {
        hero->delta_time = hero->time_a - hero->time_b;
    }
    else
    {
        hero->delta_time = hero->time_b - hero->time_a;
    }

    hero->time_since_last_frame += hero->delta_time;
    if (hero->time_since_last_frame >= (1000 / hero->anim_fps))
    {
        hero->time_since_last_frame = 0;

        hero->current_frame += 1;
        if (hero->current_frame >= hero->anim_length - 1)
        {
            hero->current_frame = 0;
        }
    }

    // Check whether the hero is on solid ground.
    int index = get_tile_index((int)hero->pos_x, (int)hero->pos_y, map);
    index += map->handle->width;

    if (map->tile_desc[index].is_solid)
    {
        hero->velocity_y = 0.f;
    }
    else
    {
        hero->velocity_y += fp_mul(GRAVITY, (float)hero->delta_time);
    }

    if (hero->velocity_y > 0.f)
    {
        hero->pos_y += fp_mul(hero->velocity_y, (float)hero->delta_time);
    }
    else
    {
        hero->pos_y = (float)((int)(hero->pos_y / METER_IN_PIXEL) * (int)METER_IN_PIXEL);
    }

    // Out of bounds.
    if (hero->pos_y >= map->height + HERO_HALF)
    {
        hero->pos_x = (float)map->spawn_x;
        hero->pos_y = (float)map->spawn_y;
    }

    if (check_bit(*btn, BTN_LEFT))
    {
        hero->heading = 0;
        hero->state = STATE_RUN;
    }
    else if (check_bit(*btn, BTN_RIGHT))
    {
        hero->heading = 1;
        hero->state = STATE_RUN;
    }
    else if (hero->velocity_x <= 0.f)
    {
        hero->state = STATE_IDLE;
    }

    if (hero->heading)
    {
        hero->sprite_offset = 0;

        if (hero->velocity_x > 0.f)
        {
            hero->pos_x += fp_mul(hero->velocity_x, (float)hero->delta_time);
        }
        else
        {
            hero->pos_x -= fp_mul(hero->velocity_x, (float)hero->delta_time);
        }
    }
    else
    {
        hero->sprite_offset = 96;

        if (hero->velocity_x > 0.f)
        {
            hero->pos_x -= fp_mul(hero->velocity_x, (float)hero->delta_time);
        }
        else
        {
            hero->pos_x += fp_mul(hero->velocity_x, (float)hero->delta_time);
        }
    }

    if (hero->pos_x <= HERO_HALF)
    {
        hero->pos_x = HERO_HALF;
    }
    else if (hero->pos_x >= map->width - HERO_HALF)
    {
        hero->pos_x = (float)(map->width - HERO_HALF);
    }

    if (STATE_IDLE == hero->state)
    {
        hero->anim_fps = 15;
        hero->anim_length = 11;
        hero->anim_offset = 0;
        return;
    }

    if (STATE_RUN == hero->state)
    {
        hero->anim_fps = 15;
        hero->anim_length = 12;
        hero->anim_offset = 32;

        if (check_bit(*btn, BTN_LEFT) || check_bit(*btn, BTN_RIGHT))
        {
            hero->velocity_x += fp_mul(ACCELERATION, (float)hero->delta_time);

            float max_speed = MAX_SPEED;

            if (check_bit(*btn, BTN_7))
            {
                max_speed = fp_mul(max_speed, 2.f);
            }

            if (hero->velocity_x > max_speed)
            {
                hero->velocity_x = max_speed;
            }
        }
        else
        {
            if (hero->velocity_x > 0.f)
            {
                hero->velocity_x -= fp_mul(DECELERATION, (float)hero->delta_time);
            }
            if (hero->velocity_x < 0.f)
            {
                hero->velocity_x = 0.f;
            }
        }
    }
}

bool render_hero(hero_t *hero, map_t *map)
{
    SDL_Rect src;
    src.x = (int)hero->pos_x - HERO_HALF;
    src.y = (int)hero->pos_y - HERO_HALF;
    src.w = HERO_SIZE;
    src.h = HERO_SIZE;

    if (src.x <= 0)
    {
        src.x = 0;
    }

    SDL_BlitSurface(map->render_canvas, &src, hero->temp_canvas, NULL);

    src.x = hero->current_frame * HERO_SIZE;
    src.y = hero->anim_offset + hero->sprite_offset;

    SDL_BlitSurface(hero->sprite, &src, hero->temp_canvas, NULL);
    SDL_BlitSurface(hero->temp_canvas, NULL, hero->render_canvas, NULL);

    return true;
}

void set_hero_state(hero_t *hero, hero_state_t state)
{
    hero->prev_state = hero->state;

    if (STATE_IDLE == state)
    {
        hero->state = STATE_IDLE;
    }
    else
    {
        hero->state |= state;
    }

    if (hero->state != hero->prev_state)
    {
        hero->current_frame = 0;
        hero->time_since_last_frame = 0;
    }
}
