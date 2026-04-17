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
#include "overlay.h"

#define KERO_SIZE 32
#define KERO_HALF 16

typedef enum kero_state
{
    STATE_IDLE = 0,
    STATE_RUN,
    STATE_JUMP,
    STATE_FALL,
    STATE_DASH,
    STATE_DEAD

} kero_state_t;

// Memory alignment for better cache performance on ARM.
// Structure is ordered by access frequency and size for optimal packing.
typedef struct kero
{
    // Hot variables (accessed every frame); keep together for cache locality.
    float pos_x;      // 4 bytes; most frequently accessed.
    float pos_y;      // 4 bytes
    float velocity_x; // 4 bytes
    float velocity_y; // 4 bytes

    Uint64 delta_time; // 8 bytes; used in calculations.

    // Moderately accessed variables
    kero_state_t state;      // 4 bytes
    kero_state_t prev_state; // 4 bytes

    int heading;       // 4 bytes; accessed often.
    int current_frame; // 4 bytes

    // Animation properties; accessed together.
    int anim_fps;        // 4 bytes
    int anim_length;     // 4 bytes
    int anim_offset_x;   // 4 bytes
    int anim_offset_y;   // 4 bytes
    int sprite_offset_x; // 4 bytes
    int sprite_offset_y; // 4 bytes

    // Less frequently accessed.
    Uint64 time_a;                // 8 bytes
    Uint64 time_b;                // 8 bytes
    Uint64 time_since_last_frame; // 8 bytes

    float warp_x; // 4 bytes
    float warp_y; // 4 bytes

    int level;           // 4 bytes
    int prev_life_count; // 4 bytes
    int life_count;      // 4 bytes
    int line_index;      // 4 bytes

    // Booleans at end (1 byte each, but padded).
    bool repeat_anim;  // 1 byte
    bool jump_lock;    // 1 byte
    bool wears_mask;   // 1 byte
    bool respawn_lock; // 1 byte

    // Pointers at end (accessed less frequently for setup/teardown).
    SDL_Surface *render_canvas; // 4/8 bytes
    SDL_Surface *temp_canvas;   // 4/8 bytes

} kero_t
#ifdef __SYMBIAN32__
    __attribute__((aligned(32))) // Cache line alignment on ARM for better performance.
#endif
    ;

void destroy_kero(kero_t *kero);
bool load_kero(kero_t **kero, map_t *map);
void update_kero(kero_t *kero, map_t *map, overlay_t *ui, unsigned int *btn, SDL_Renderer *renderer, bool is_paused, bool *has_updated);
bool render_kero(kero_t *kero, map_t *map);

#endif // KERO_H
