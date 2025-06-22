/** @file kero.c
 *
 *  A minimalist, cross-platform puzzle-platformer, designed
 *  especially for the Nokia N-Gage.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#include "SDL3/SDL.h"

#include "config.h"
#include "fixedp.h"
#include "kero.h"
#include "utils.h"

typedef enum kero_state
{
    STATE_IDLE = 0,
    STATE_RUN,
    STATE_JUMP,
    STATE_FALL,
    STATE_POWER_UP

} kero_state_t;

static void update_kero_timing(kero_t *kero)
{
    kero->time_b = kero->time_a;
    kero->time_a = SDL_GetTicks();
    kero->delta_time = (kero->time_a > kero->time_b)
                           ? kero->time_a - kero->time_b
                           : kero->time_b - kero->time_a;
}

static void update_kero_animation(kero_t *kero)
{
    kero->time_since_last_frame += kero->delta_time;
    if (kero->time_since_last_frame >= (1000 / kero->anim_fps))
    {
        kero->time_since_last_frame = 0;
        kero->current_frame += 1;
        if (kero->current_frame >= kero->anim_length - 1)
        {
            if (kero->repeat_anim)
            {
                kero->current_frame = 0;
            }
            else
            {
                kero->current_frame = kero->anim_length - 1;
            }
        }
    }
}

static void apply_gravity(kero_t *kero)
{
    kero->velocity_y += fp_mul(GRAVITY, (float)kero->delta_time);
    if (kero->velocity_y > MAX_FALLING_SPEED)
    {
        kero->velocity_y = MAX_FALLING_SPEED;
    }
}

static void set_kero_state(kero_t *kero, kero_state_t state)
{
    kero->prev_state = kero->state;
    kero->state = state;

    if (kero->state != kero->prev_state)
    {
        kero->current_frame = 0;
        kero->time_since_last_frame = 0;
    }
}

static void handle_jump(kero_t *kero, unsigned int *btn)
{
    if (check_bit(*btn, BTN_7) && !kero->jump_lock)
    {
        if (kero->prev_state != STATE_JUMP && kero->state != STATE_JUMP)
        {
            kero->velocity_y = -JUMP_VELOCITY;
            set_kero_state(kero, STATE_JUMP);
        }
        kero->jump_lock = true;
    }
}

static void handle_interaction(kero_t *kero, map_t *map, unsigned int *btn)
{
    int index = get_tile_index((int)kero->pos_x, (int)kero->pos_y, map);

    if (check_bit(*btn, BTN_UP))
    {
        if (map->tile_desc[index].is_door)
        {
            // tbd.
        }
    }
}

static void handle_pickup(kero_t *kero, map_t *map)
{
    int index = get_tile_index((int)kero->pos_x, (int)kero->pos_y, map);

    // if (map->tile_desc[index].is_coin)
    {
        // SDL_Log("Picked up a coin at index %d", index);
        // kero->wears_mask = true;
    }
}

static bool handle_power_up(kero_t *kero, unsigned int *btn)
{
    if (STATE_POWER_UP == kero->state)
    {
        if (check_bit(*btn, BTN_LEFT))
        {
            kero->pos_x = kero->warp_x - 64.f;
        }
        else if (check_bit(*btn, BTN_RIGHT))
        {
            kero->pos_x = kero->warp_x + 64.f;
        }

        // Timeout for power-up state.
        if (SDL_GetTicks() - kero->power_up_timeout >= POWER_UP_TIMEOUT)
        {
            clear_bit(btn, BTN_5);
            set_kero_state(kero, kero->prev_state);
        }
        else
        {
            return false;
        }
    }

    if (check_bit(*btn, BTN_5))
    {
        set_kero_state(kero, STATE_POWER_UP);

        if (kero->prev_state != STATE_POWER_UP)
        {
            kero->current_frame = 0;
            kero->time_since_last_frame = 0;
            kero->warp_x = kero->pos_x;
            kero->warp_y = kero->pos_y;
        }

        kero->anim_fps = 15;
        kero->anim_length = 7;
        kero->anim_offset_x = 2;
        kero->anim_offset_y = 64;
        kero->repeat_anim = false;

        kero->power_up_timeout = SDL_GetTicks();
        return false;
    }

    kero->repeat_anim = true;
    return true;
}

static void clamp_kero_position(kero_t *kero, map_t *map)
{
    if (kero->pos_y <= 0.f + KERO_HALF)
    {
        kero->pos_y = 0.f + KERO_HALF;
        kero->velocity_y = 0.f;
    }
    if (kero->pos_x <= KERO_HALF)
    {
        kero->pos_x = KERO_HALF;
    }
    else if (kero->pos_x >= map->width - KERO_HALF)
    {
        kero->pos_x = (float)(map->width - KERO_HALF);
    }
    else
    {
        bool blocked_by_wall;
        int index = get_tile_index((int)kero->pos_x, (int)kero->pos_y, map);

        if (kero->heading)
        {
            index += 1;
        }
        else
        {
            index -= 1;
        }

        blocked_by_wall = map->tile_desc[index].is_wall;
        if (blocked_by_wall)
        {
            if (kero->heading)
            {
                kero->pos_x = (float)(index % map->handle->width) * map->handle->tilewidth - KERO_HALF;
            }
            else
            {
                kero->pos_x = (float)((index % map->handle->width) + 1) * map->handle->tilewidth + KERO_HALF;
            }
            kero->velocity_x = 0.f;
        }
    }
}

static void reset_kero_on_out_of_bounds(kero_t *kero, map_t *map)
{
    set_kero_state(kero, STATE_IDLE);
    kero->pos_x = (float)map->spawn_x;
    kero->pos_y = (float)map->spawn_y;
    kero->velocity_x = 0.f;
    kero->velocity_y = 0.f;
}

void destroy_kero(kero_t *kero)
{
    if (kero)
    {
        if (kero->render_canvas)
        {
            SDL_DestroySurface(kero->render_canvas);
            kero->render_canvas = NULL;
        }

        if (kero->temp_canvas)
        {
            SDL_DestroySurface(kero->temp_canvas);
            kero->temp_canvas = NULL;
        }

        if (kero->sprite)
        {
            SDL_DestroySurface(kero->sprite);
            kero->sprite = NULL;
        }
    }
}

bool load_kero(kero_t **kero, map_t *map)
{
    *kero = (kero_t *)SDL_calloc(1, sizeof(kero_t));
    if (!*kero)
    {
        SDL_Log("Failed to allocate memory for kero");
        return false;
    }

    if (!load_surface_from_file("kero.png", &(*kero)->sprite))
    {
        SDL_Log("Failed to load kero sprite");
        return false;
    }

    (*kero)->render_canvas = SDL_CreateSurface(KERO_SIZE, KERO_SIZE, SDL_PIXELFORMAT_XRGB4444);
    if (!(*kero)->render_canvas)
    {
        SDL_Log("Error creating temporary surface: %s", SDL_GetError());
        return false;
    }

    (*kero)->temp_canvas = SDL_CreateSurface(KERO_SIZE, KERO_SIZE, SDL_PIXELFORMAT_XRGB4444);
    if (!(*kero)->temp_canvas)
    {
        SDL_Log("Error creating temporary surface: %s", SDL_GetError());
        return false;
    }

    (*kero)->pos_x = (float)map->spawn_x;
    (*kero)->pos_y = (float)map->spawn_y;
    (*kero)->anim_fps = 1;
    (*kero)->heading = 1;

    set_kero_state(*kero, STATE_IDLE);

    return true;
}

void update_kero(kero_t *kero, map_t *map, unsigned int *btn)
{
    update_kero_timing(kero);
    update_kero_animation(kero);

    if (!handle_power_up(kero, btn))
    {
        return;
    }

    int index = get_tile_index((int)kero->pos_x, (int)kero->pos_y, map);
    if (map->tile_desc[index].is_wall && STATE_POWER_UP == kero->state)
    {
        // Reset kero position if teleport ended inside a wall.
        reset_kero_on_out_of_bounds(kero, map);
        return;
    }

    // Check ground status.
    index += map->handle->width;
    bool on_solid_ground = map->tile_desc[index].is_solid && kero->state != STATE_JUMP;
    bool at_bottom = kero->pos_y > map->height - KERO_HALF;

    // Vertical movement.
    if (at_bottom)
    {
        apply_gravity(kero);
    }
    else if (on_solid_ground)
    {
        if (STATE_FALL == kero->prev_state || STATE_JUMP == kero->prev_state)
        {
            kero->velocity_x = 0.f; // Stop horizontal movement when landing.
        }
        kero->velocity_y = 0.f;

        handle_pickup(kero, map);
        handle_interaction(kero, map, btn);

        if (!check_bit(*btn, BTN_7))
        {
            kero->jump_lock = false;
        }
        handle_jump(kero, btn);
    }
    else
    {
        apply_gravity(kero);
    }

    // Update Y position.
    if (kero->velocity_y != 0.f)
    {
        kero->pos_y += fp_mul(kero->velocity_y, (float)kero->delta_time);
    }
    else
    {
        kero->pos_y = (float)((int)(kero->pos_y / map->handle->tileheight) * map->handle->tileheight);
        kero->pos_y += map->tile_desc[index].offset_top;
    }

    // Out of bounds check.
    if (kero->pos_y >= map->height + KERO_HALF)
    {
        reset_kero_on_out_of_bounds(kero, map);
    }

    // Horizontal input and state.
    if (check_bit(*btn, BTN_LEFT))
    {
        kero->heading = 0;
        set_kero_state(kero, STATE_RUN);
    }
    else if (check_bit(*btn, BTN_RIGHT))
    {
        kero->heading = 1;
        set_kero_state(kero, STATE_RUN);
    }
    else if (kero->velocity_x <= 0.f)
    {
        set_kero_state(kero, STATE_IDLE);
    }

    // Horizontal movement and sprite offset.
    float move = fp_mul(kero->velocity_x, (float)kero->delta_time);
    if (kero->heading)
    {
        kero->sprite_offset = 0;
        kero->pos_x += (kero->velocity_x > 0.f) ? move : -move;
    }
    else
    {
        kero->sprite_offset = 96;
        kero->pos_x += (kero->velocity_x > 0.f) ? -move : move;
    }

    if (kero->wears_mask)
    {
        kero->sprite_offset += 192;
    }

    clamp_kero_position(kero, map);

    // Animation state.
    if (kero->velocity_y < 0.f)
    {
        set_kero_state(kero, STATE_JUMP);
        kero->anim_fps = 15;
        kero->anim_length = 0;
        kero->anim_offset_x = 0;
        kero->anim_offset_y = 64;
    }
    else if (kero->velocity_y > 0.f)
    {
        set_kero_state(kero, STATE_FALL);
        kero->anim_fps = 15;
        kero->anim_length = 0;
        kero->anim_offset_x = 1;
        kero->anim_offset_y = 64;
    }
    else if (STATE_IDLE == kero->state)
    {
        kero->anim_fps = 15;
        kero->anim_length = 11;
        kero->anim_offset_x = 0;
        kero->anim_offset_y = 0;
        return;
    }
    else if (STATE_RUN == kero->state)
    {
        kero->anim_fps = 15;
        kero->anim_length = 12;
        kero->anim_offset_x = 0;
        kero->anim_offset_y = 32;
    }

    // Running state.
    if (STATE_RUN == kero->state || (kero->velocity_y != 0.f))
    {
        if (check_bit(*btn, BTN_LEFT) || check_bit(*btn, BTN_RIGHT))
        {
            kero->velocity_x += fp_mul(ACCELERATION, (float)kero->delta_time);
            if (kero->velocity_x > MAX_SPEED)
            {
                kero->velocity_x = MAX_SPEED;
            }
        }
        else
        {
            if (kero->velocity_x > 0.f)
            {
                kero->velocity_x -= fp_mul(DECELERATION, (float)kero->delta_time);
            }
            if (kero->velocity_x < 0.f)
            {
                kero->velocity_x = 0.f;
            }
        }
    }
}

bool render_kero(kero_t *kero, map_t *map)
{
    SDL_Rect src;
    src.x = (int)kero->pos_x - KERO_HALF;
    src.y = (int)kero->pos_y - KERO_HALF;
    src.w = KERO_SIZE;
    src.h = KERO_SIZE;

    if (src.x <= 0)
    {
        src.x = 0;
    }

    SDL_BlitSurface(map->render_canvas, &src, kero->temp_canvas, NULL);

    src.x = (kero->current_frame + kero->anim_offset_x) * KERO_SIZE;
    src.y = kero->anim_offset_y + kero->sprite_offset;

    SDL_BlitSurface(kero->sprite, &src, kero->temp_canvas, NULL);
    SDL_BlitSurface(kero->temp_canvas, NULL, kero->render_canvas, NULL);

    return true;
}
