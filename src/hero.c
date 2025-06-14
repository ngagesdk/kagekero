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

typedef enum hero_state
{
    STATE_IDLE = 0,
    STATE_RUN,
    STATE_HURT,
    STATE_JUMP,
    STATE_CLIMB,
    STATE_CROUCH,
    STATE_FALL

} hero_state_t;

static void update_hero_timing(hero_t *hero)
{
    hero->time_b = hero->time_a;
    hero->time_a = SDL_GetTicks();
    hero->delta_time = (hero->time_a > hero->time_b)
                           ? hero->time_a - hero->time_b
                           : hero->time_b - hero->time_a;
}

static void update_hero_animation(hero_t *hero)
{
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
}

static void apply_gravity(hero_t *hero)
{
    hero->velocity_y += fp_mul(GRAVITY, (float)hero->delta_time);
    if (hero->velocity_y > MAX_FALLING_SPEED)
    {
        hero->velocity_y = MAX_FALLING_SPEED;
    }
}

static void set_hero_state(hero_t *hero, hero_state_t state)
{
    hero->prev_state = hero->state;
    hero->state = state;

    if (hero->state != hero->prev_state)
    {
        hero->current_frame = 0;
        hero->time_since_last_frame = 0;
    }
}

static void handle_jump(hero_t *hero, unsigned int *btn)
{
    if (check_bit(*btn, BTN_5))
    {
        if (hero->prev_state != STATE_JUMP && hero->state != STATE_JUMP)
        {
            hero->velocity_y = -JUMP_VELOCITY;
            set_hero_state(hero, STATE_JUMP);
        }
    }
}

static void clamp_hero_position(hero_t *hero, map_t *map)
{
    if (hero->pos_y <= 0.f + HERO_HALF)
    {
        hero->pos_y = 0.f + HERO_HALF;
        hero->velocity_y = 0.f;
    }
    if (hero->pos_x <= HERO_HALF)
    {
        hero->pos_x = HERO_HALF;
    }
    else if (hero->pos_x >= map->width - HERO_HALF)
    {
        hero->pos_x = (float)(map->width - HERO_HALF);
    }
}

static void reset_hero_on_out_of_bounds(hero_t *hero, map_t *map)
{
    set_hero_state(hero, STATE_IDLE);
    hero->pos_x = (float)map->spawn_x;
    hero->pos_y = (float)map->spawn_y;
    hero->velocity_x = 0.f;
    hero->velocity_y = 0.f;
}

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
    update_hero_timing(hero);
    update_hero_animation(hero);

    // Check ground status.
    int index = get_tile_index((int)hero->pos_x, (int)hero->pos_y, map);
    index += map->handle->width;
    bool on_solid_ground = map->tile_desc[index].is_solid && hero->state != STATE_JUMP;
    bool at_bottom = hero->pos_y > map->height - HERO_HALF;

    // Vertical movement.
    if (at_bottom)
    {
        apply_gravity(hero);
    }
    else if (on_solid_ground)
    {
        if (STATE_FALL == hero->prev_state || STATE_JUMP == hero->prev_state)
        {
            hero->velocity_x = 0.f; // Stop horizontal movement when landing.
        }
        hero->velocity_y = 0.f;
        handle_jump(hero, btn);
    }
    else
    {
        apply_gravity(hero);
    }

    // Update Y position.
    if (hero->velocity_y != 0.f)
    {
        hero->pos_y += fp_mul(hero->velocity_y, (float)hero->delta_time);
    }
    else
    {
        hero->pos_y = (float)((int)(hero->pos_y / map->handle->tileheight) * map->handle->tileheight);
        hero->pos_y += map->tile_desc[index].offset_top;
    }

    // Out of bounds check.
    if (hero->pos_y >= map->height + HERO_HALF)
    {
        reset_hero_on_out_of_bounds(hero, map);
    }

    // Horizontal input and state.
    if (check_bit(*btn, BTN_LEFT))
    {
        hero->heading = 0;
        set_hero_state(hero, STATE_RUN);
    }
    else if (check_bit(*btn, BTN_RIGHT))
    {
        hero->heading = 1;
        set_hero_state(hero, STATE_RUN);
    }
    else if (hero->velocity_x <= 0.f)
    {
        set_hero_state(hero, STATE_IDLE);
    }

    // Horizontal movement and sprite offset.
    float move = fp_mul(hero->velocity_x, (float)hero->delta_time);
    if (hero->heading)
    {
        hero->sprite_offset = 0;
        hero->pos_x += (hero->velocity_x > 0.f) ? move : -move;
    }
    else
    {
        hero->sprite_offset = 96;
        hero->pos_x += (hero->velocity_x > 0.f) ? -move : move;
    }

    clamp_hero_position(hero, map);

    // Animation state.
    if (hero->velocity_y < 0.f)
    {
        set_hero_state(hero, STATE_JUMP);
        hero->anim_fps = 1;
        hero->anim_length = 0;
        hero->anim_offset_x = 0;
        hero->anim_offset_y = 64;
    }
    else if (hero->velocity_y > 0.f)
    {
        set_hero_state(hero, STATE_FALL);
        hero->anim_fps = 1;
        hero->anim_length = 0;
        hero->anim_offset_x = 1;
        hero->anim_offset_y = 64;
    }
    else if (STATE_IDLE == hero->state)
    {
        hero->anim_fps = 15;
        hero->anim_length = 11;
        hero->anim_offset_x = 0;
        hero->anim_offset_y = 0;
        return;
    }

    // Running state.
    if (STATE_RUN == hero->state || (hero->velocity_y != 0.f))
    {
        hero->anim_fps = 15;
        hero->anim_length = 12;
        hero->anim_offset_x = 0;
        hero->anim_offset_y = 32;

        if (check_bit(*btn, BTN_LEFT) || check_bit(*btn, BTN_RIGHT))
        {
            hero->velocity_x += fp_mul(ACCELERATION, (float)hero->delta_time);
            if (hero->velocity_x > MAX_SPEED)
            {
                hero->velocity_x = MAX_SPEED;
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

    src.x = (hero->current_frame + hero->anim_offset_x) * HERO_SIZE;
    src.y = hero->anim_offset_y + hero->sprite_offset;

    SDL_BlitSurface(hero->sprite, &src, hero->temp_canvas, NULL);
    SDL_BlitSurface(hero->temp_canvas, NULL, hero->render_canvas, NULL);

    return true;
}
