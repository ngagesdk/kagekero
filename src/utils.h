/** @file utils.h
 *
 *  A minimalist, cross-platform puzzle-platformer, designed
 *  especially for the Nokia N-Gage.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
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
