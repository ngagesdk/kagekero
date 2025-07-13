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

#include "aabb.h"
#include "config.h"
#include "fixedp.h"
#include "kero.h"
#include "map.h"
#include "overlay.h"
#include "utils.h"

static const char *death_lines[DEATH_LINE_COUNT] = {
    "Ribbit. Guess I   croaked for real  this time.",
    "This is where I   leapt... and this is where I flopp- ed.",
    "One small hop for frog, one giant   leap into fail-   ure.",
    "This was where I  ribbited. This waswhere I regretted it.",
    "Put that one on   my highlight reel - the blooper ed- ition.",
    "If Madeline can   do it a thousand  times, so can I.  Ribbit.",
    "Death count: too  high. Pride: stillintact.",
    "Pro tip: Don't do what I just did.",
    "This was where I  thought Frogger   physics still app-lied.",
    "At least when I   dash into spikes, I don't have to   listen to a moti- vational speech first.",
    "Guess I just      Madelined myself  into the spikes   again. Classic.",
    "Climbing my way   to the afterlife  - one dumb jump   ata time.",
    "Next time I'll    bring a moti-     vational sound-   track like        Madeline. Might help.",
    "If Madeline can   face her demons, Ican face... what- ever just impal-  ed me.",
    "Maybe I should've stuck to straw-   berries instead   of pain.",
    "This was where I  went full ninja.  And full pancake.",
    "Bad jump. Worse   landing.          10/10 Celeste tri-bute though.",
    "Hey Madeline! Saveme a spot on the  death counter!",
    "I'd call for help,but my inner      demon's on vac-   ation.",
    "Like a Nokia brick- unbreakable? Nottoday.",
    "Should've brought my Celeste climb- ing gloves.",
};

static void update_kero_timing(kero_t *kero)
{
    kero->time_b = kero->time_a;
    kero->time_a = SDL_GetTicks();
    kero->delta_time = (kero->time_a > kero->time_b)
                           ? kero->time_a - kero->time_b
                           : kero->time_b - kero->time_a;
}

static void update_kero_animation(kero_t *kero, bool *has_updated)
{
    *has_updated = false;

    kero->time_since_last_frame += kero->delta_time;
    if (kero->time_since_last_frame >= (1000 / kero->anim_fps))
    {
        *has_updated = true;
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

static void handle_jump(kero_t *kero, map_t *map, unsigned int *btn)
{
    if (check_bit(*btn, BTN_7) && !check_bit(*btn, BTN_5) && !kero->jump_lock)
    {
        int index = get_tile_index((int)kero->pos_x, (int)kero->pos_y - KERO_SIZE, map);
        index -= map->handle->width;
        if (index >= 0)
        {
            if (kero->prev_state != STATE_JUMP && kero->state != STATE_JUMP)
            {
                kero->velocity_y = -JUMP_VELOCITY;
                set_kero_state(kero, STATE_JUMP);
            }
            kero->jump_lock = true;
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

static void handle_interaction(kero_t *kero, map_t *map, unsigned int *btn, SDL_Renderer *renderer)
{
    if (check_bit(*btn, BTN_UP))
    {
        aabb_t kero_bb;
        kero_bb.top = kero->pos_y - KERO_HALF;
        kero_bb.bottom = kero->pos_y + KERO_HALF;
        kero_bb.left = kero->pos_x - KERO_HALF;
        kero_bb.right = kero->pos_x + KERO_HALF;

        int index = -1;

        if (object_intersects(kero_bb, map, &index))
        {
            if (H_DOOR == map->obj[index].hash)
            {
                if (1 == map->obj[index].start_frame) // Door is open.
                {
                    char next_map[11] = { 0 };
                    kero->level += 1;

                    SDL_snprintf(next_map, 11, "%03d.%s", kero->level, MAP_SUFFIX);
                    if (!load_map(next_map, &map, renderer))
                    {
                        SDL_Log("Failed to load next map: %s", next_map);
                        return;
                    }
                    else
                    {
                        kero->pos_x = (float)map->spawn_x;
                        kero->pos_y = (float)map->spawn_y;
                        kero->velocity_x = 0.f;
                        kero->velocity_y = 0.f;
                    }
                }
            }
        }
    }
}

static void handle_intersect(kero_t *kero, map_t *map, overlay_t *ui)
{
    aabb_t kero_bb;
    kero_bb.top = kero->pos_y - KERO_HALF;
    kero_bb.bottom = kero->pos_y + KERO_HALF;
    kero_bb.left = kero->pos_x - KERO_HALF;
    kero_bb.right = kero->pos_x + KERO_HALF;

    int index = -1;

    if (object_intersects(kero_bb, map, &index))
    {
        if (H_COIN == map->obj[index].hash)
        {
            if (!map->obj[index].is_hidden)
            {
                map->prev_coins = map->coins_left;
                map->coins_left -= 1;
                if (map->coins_left < 0)
                {
                    map->coins_left = 0;
                }
            }
            map->obj[index].is_hidden = true;
        }
        else if (H_BLOCK == map->obj[index].hash)
        {
            if (map->obj[index + 1].str && !map->show_dialogue)
            {
                map->show_dialogue = true;
                map->keep_dialogue = false;
                render_text_ex(map->obj[index + 1].str, true, 640, 146, map, ui);
            }
        }
    }
}

static void handle_dash(kero_t *kero, unsigned int *btn)
{
    if (check_bit(*btn, BTN_5) && check_bit(*btn, BTN_7))
    {
        return;
    }

    if (!kero->jump_lock && !kero->velocity_y)
    {
        // Only allow dash while jumping or falling.
        return;
    }

    if (check_bit(*btn, BTN_5) && STATE_DEAD != kero->prev_state)
    {
        set_kero_state(kero, STATE_DASH);

        if (kero->prev_state != STATE_DASH)
        {
            kero->current_frame = 0;
            kero->time_since_last_frame = 0;
            kero->velocity_x = ACCELERATION_DASH;
        }

        kero->anim_fps = 15;
        kero->anim_length = 6;
        kero->anim_offset_x = 2;
        kero->anim_offset_y = 2;
    }
}

static void handle_death(kero_t *kero)
{
    set_kero_state(kero, STATE_DEAD);

    kero->anim_fps = 15;
    kero->anim_length = 3;
    kero->anim_offset_x = 8;
    kero->anim_offset_y = 2;
    kero->repeat_anim = false;
    kero->respawn_lock = true;

    kero->life_count -= 1;

    if (kero->life_count < 0)
    {
        // Tbd.
        kero->life_count = 0;
    }
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

#ifndef __DREAMCAST__
    SDL_PixelFormat pixel_format = SDL_PIXELFORMAT_XRGB4444;
#else
    SDL_PixelFormat pixel_format = SDL_PIXELFORMAT_ARGB1555;
#endif

    (*kero)->render_canvas = SDL_CreateSurface(KERO_SIZE, KERO_SIZE, pixel_format);
    if (!(*kero)->render_canvas)
    {
        SDL_Log("Error creating render canvas surface: %s", SDL_GetError());
        return false;
    }

    (*kero)->temp_canvas = SDL_CreateSurface(KERO_SIZE, KERO_SIZE, pixel_format);
    if (!(*kero)->temp_canvas)
    {
        SDL_Log("Error creating temporary surface: %s", SDL_GetError());
        return false;
    }

    (*kero)->pos_x = (float)map->spawn_x;
    (*kero)->pos_y = (float)map->spawn_y;
    (*kero)->anim_fps = 1;
    (*kero)->repeat_anim = true;
    (*kero)->heading = 1;
    (*kero)->level = FIRST_LEVEL;
    (*kero)->life_count = 99;
    (*kero)->line_index = -1;

    set_kero_state(*kero, STATE_IDLE);

    return true;
}

void update_kero(kero_t *kero, map_t *map, overlay_t *ui, unsigned int *btn, SDL_Renderer *renderer, bool is_paused, bool *has_updated)
{
    update_kero_timing(kero);

    if (is_paused)
    {
        kero->jump_lock = true;
        return;
    }

    update_kero_animation(kero, has_updated);

    if (STATE_DEAD == kero->state)
    {
        kero->jump_lock = true;

        if (!kero->respawn_lock)
        {
            if (check_bit(*btn, BTN_7) || check_bit(*btn, BTN_SELECT))
            {
                reset_kero_on_out_of_bounds(kero, map);
                kero->repeat_anim = true;
                kero->state = STATE_IDLE;
                kero->prev_life_count = kero->life_count;
            }
        }
        else if (!check_bit(*btn, BTN_7) && !check_bit(*btn, BTN_SELECT))
        {
            kero->respawn_lock = false;
        }

        return;
    }

    handle_intersect(kero, map, ui);
    handle_dash(kero, btn);

    int index = get_tile_index((int)kero->pos_x, (int)kero->pos_y, map);

    // Check ground status.
    bool on_deadly_ground = map->tile_desc[index].is_deadly;
    index += map->handle->width;
    bool on_solid_ground = map->tile_desc[index].is_solid && kero->state != STATE_JUMP;
    bool at_bottom = kero->pos_y > map->height - KERO_HALF;

    // Vertical movement.
    if (on_deadly_ground)
    {
        handle_death(kero);
        return;
    }
    else if (at_bottom)
    {
        apply_gravity(kero);
    }
    else if (on_solid_ground)
    {
        if (STATE_FALL == kero->prev_state || STATE_JUMP == kero->prev_state)
        {
            kero->velocity_x = 0.f; // Stop horizontal movement when landing.
        }
        else if (STATE_DASH == kero->prev_state)
        {
            // Reset dash state when landing.
            set_kero_state(kero, STATE_IDLE);
            kero->velocity_x = 0.f;
        }
        kero->velocity_y = 0.f;

        handle_interaction(kero, map, btn, renderer);

        if (!check_bit(*btn, BTN_7))
        {
            kero->jump_lock = false;
        }
        handle_jump(kero, map, btn);
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
        kero->line_index++;
        render_text(death_lines[kero->line_index], kero->wears_mask, map, ui);
        map->show_dialogue = true;

        handle_death(kero);
        return;
    }

    // Horizontal input and state.
    if (STATE_DASH != kero->state)
    {
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
    }
    else if (kero->velocity_x <= 0.f)
    {
        set_kero_state(kero, STATE_IDLE);
    }

    // Horizontal movement and sprite offset.
    float move = fp_mul(kero->velocity_x, (float)kero->delta_time);
    if (kero->heading)
    {
        kero->sprite_offset_y = 0;
        kero->pos_x += (kero->velocity_x > 0.f) ? move : -move;
    }
    else
    {
        kero->sprite_offset_y = 3;
        kero->pos_x += (kero->velocity_x > 0.f) ? -move : move;
    }

    if (kero->wears_mask)
    {
        kero->sprite_offset_x = 12;
    }
    else
    {
        kero->sprite_offset_x = 0;
    }

    clamp_kero_position(kero, map);

    // Animation state.
    if (kero->velocity_y < 0.f)
    {
        if (STATE_DASH != kero->state)
        {
            set_kero_state(kero, STATE_JUMP);
            kero->anim_fps = 15;
            kero->anim_length = 0;
            kero->anim_offset_x = 0;
            kero->anim_offset_y = 2;
        }
    }
    else if (kero->velocity_y > 0.f)
    {
        if (STATE_DASH != kero->state)
        {
            set_kero_state(kero, STATE_FALL);
            kero->anim_fps = 15;
            kero->anim_length = 0;
            kero->anim_offset_x = 1;
            kero->anim_offset_y = 2;
        }
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
        kero->anim_offset_y = 1;
    }

    // Running state.
    if (STATE_RUN == kero->state || (kero->velocity_y != 0.f))
    {
        if ((check_bit(*btn, BTN_LEFT) || check_bit(*btn, BTN_RIGHT)) && STATE_DASH != kero->state)
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

    if (!SDL_BlitSurface(map->render_canvas, &src, kero->temp_canvas, NULL))
    {
        SDL_Log("Error blitting kero canvas: %s", SDL_GetError());
        return false;
    }

    src.x = (kero->current_frame + kero->anim_offset_x + kero->sprite_offset_x) * KERO_SIZE;
    src.y = 656 + (kero->anim_offset_y + kero->sprite_offset_y) * KERO_SIZE;

    if (!SDL_BlitSurface(map->tileset_surface, &src, kero->temp_canvas, NULL))
    {
        SDL_Log("Error blitting kero sprite: %s", SDL_GetError());
        return false;
    }

    if (!SDL_BlitSurface(kero->temp_canvas, NULL, kero->render_canvas, NULL))
    {
        SDL_Log("Error blitting kero temp canvas: %s", SDL_GetError());
        return false;
    }

    return true;
}
