/** @file config.h
 *
 *  A minimalist, cross-platform puzzle-platformer, designed
 *  especially for the Nokia N-Gage.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#ifndef CONFIG_H
#define CONFIG_H

#include "SDL3/SDL.h"

#define FIRST_LEVEL       1
#define ANIM_FPS          15
#define ACCELERATION_DASH 0.6f
#define ACCELERATION      0.0025f
#define DECELERATION      0.0025f
#define MAX_SPEED         0.1f
#define GRAVITY           0.00125f
#define MAX_FALLING_SPEED 0.2f
#define JUMP_VELOCITY     0.3f

#define SCREEN_W 176
#define SCREEN_H 208

#if defined DEBUG
// Default configuration.
#elif defined __SYMBIAN32__
#define MAP_PREFIX      ".gz"
#define WINDOW_W        176
#define WINDOW_H        208
#define SCREEN_OFFSET_X 0
#define SCREEN_OFFSET_Y 0
#elif defined __3DS__
#define WINDOW_W        400
#define WINDOW_H        240
#define SCREEN_OFFSET_X 112
#define SCREEN_OFFSET_Y 16
#define FRAME_IMAGE     "frame_400x240.png"
#elif defined __DREAMCAST__
#define WINDOW_FLAGS    SDL_WINDOW_FULLSCREEN
#define FRAME_OFFSET_X  64
#define FRAME_OFFSET_Y  -12
#define SCREEN_OFFSET_X 232
#define SCREEN_OFFSET_Y 136
#define FRAME_IMAGE     "frame_512x512.png"
#else
#define WINDOW_FLAGS    SDL_WINDOW_FULLSCREEN
#define SCREEN_OFFSET_X 232
#define SCREEN_OFFSET_Y 136
#endif

#if defined __3DS__
#define FRAME_WIDTH  400
#define FRAME_HEIGHT 240
#elif defined __DREAMCAST__
#define FRAME_WIDTH  512
#define FRAME_HEIGHT 512
#else
#define FRAME_WIDTH  640
#define FRAME_HEIGHT 480
#endif

#ifndef FRAME_IMAGE
#define FRAME_IMAGE "frame.png"
#endif

#ifndef FRAME_OFFSET_X
#define FRAME_OFFSET_X 0
#endif

#ifndef FRAME_OFFSET_Y
#define FRAME_OFFSET_Y 0
#endif

#ifndef MAP_PREFIX
#define MAP_PREFIX ""
#endif

#ifndef SCALE
#define SCALE 1
#endif

#ifndef SCREEN_OFFSET_X
#define SCREEN_OFFSET_X 232
#endif

#ifndef SCREEN_OFFSET_Y
#define SCREEN_OFFSET_Y 136
#endif

#ifndef WINDOW_W
#define WINDOW_W 640
#endif

#ifndef WINDOW_H
#define WINDOW_H 480
#endif

#ifndef WINDOW_FLAGS
#define WINDOW_FLAGS 0
#endif

#endif // CONFIG_H
