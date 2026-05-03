/** @file utils.h
 *
 *  A minimalist, cross-platform puzzle-platformer, designed
 *  especially for the Nokia N-Gage.
 *
 *  Copyright (c) 2026, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#ifndef UTILS_H
#define UTILS_H

#include <SDL3/SDL.h>

typedef enum button
{
    BTN_NONE = 0,
    BTN_BACKSPACE,
    BTN_1,
    BTN_2,
    BTN_3,
    BTN_4,
    BTN_5,
    BTN_6,
    BTN_7,
    BTN_8,
    BTN_9,
    BTN_0,
    BTN_ASTERISK,
    BTN_HASH,
    BTN_SOFTLEFT,
    BTN_SOFTRIGHT,
    BTN_SELECT,
    BTN_UP,
    BTN_DOWN,
    BTN_LEFT,
    BTN_RIGHT

} button_t;

// Fast bit manipulation macros (optimized for ARM)
#if defined(__SYMBIAN32__)
// On ARM, use inline bit operations for better performance.
#define CHECK_BIT_FAST(num, n) ((num) & (1U << (n)))
#define SET_BIT_FAST(num, n) ((num) |= (1U << (n)))
#define CLEAR_BIT_FAST(num, n) ((num) &= ~(1U << (n)))
#else
#define CHECK_BIT_FAST check_bit
#define SET_BIT_FAST set_bit
#define CLEAR_BIT_FAST clear_bit
#endif

// Fast tile calculations using bit shifts since tile sizes are powers of 2.
#define TILE_WIDTH_SHIFT 3   // 2^3 = 8 pixels (change to 4 for 16-pixel tiles)
#define TILE_HEIGHT_SHIFT 3  // 2^3 = 8 pixels

// Fast tile index calculation: (y / tile_height) * map_width + (x / tile_width)
// Using bit shifts: (y >> TILE_HEIGHT_SHIFT) * map_width + (x >> TILE_WIDTH_SHIFT)
#define GET_TILE_INDEX_FAST(x, y, map_width) \
    (((y) >> TILE_HEIGHT_SHIFT) * (map_width) + ((x) >> TILE_WIDTH_SHIFT))

// Fast position to tile coordinate.
#define POS_TO_TILE_X(x) ((x) >> TILE_WIDTH_SHIFT)
#define POS_TO_TILE_Y(y) ((y) >> TILE_HEIGHT_SHIFT)

// Fast tile to position coordinate.
#define TILE_TO_POS_X(tile_x) ((tile_x) << TILE_WIDTH_SHIFT)
#define TILE_TO_POS_Y(tile_y) ((tile_y) << TILE_HEIGHT_SHIFT)

// Snap position to tile grid.
#define SNAP_TO_TILE_X(x) ((x) & ~((1 << TILE_WIDTH_SHIFT) - 1))
#define SNAP_TO_TILE_Y(y) ((y) & ~((1 << TILE_HEIGHT_SHIFT) - 1))

bool load_surface_from_file(const char *file_name, SDL_Surface **texture);
bool load_texture_from_file(const char *file_name, SDL_Texture **texture, SDL_Renderer *renderer);

Uint64 generate_hash(const unsigned char *name);

unsigned int set_bit(unsigned int *number, unsigned int n);
unsigned int clear_bit(unsigned int *number, unsigned int n);
unsigned int toggle_bit(unsigned int *number, unsigned int n);
bool check_bit(unsigned int number, unsigned int n);

button_t get_button_from_key(SDL_Keycode key);
button_t get_button_from_gamepad(Uint8 pad_btn);

#endif // UTILS_H
