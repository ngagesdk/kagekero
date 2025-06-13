/** @file config.h
 *
 *  A cross-platform engine with native Nokia N-Gage compatibility.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#ifndef CONFIG_H
#define CONFIG_H

#include "SDL3/SDL.h"

#define ANIM_FPS       15
#define METER_IN_PIXEL 16
#define ACCELERATION   0.0025f
#define DECELERATION   0.0008f
#define MAX_SPEED      0.1f
#define GRAVITY        0.00125f

#define START_MAP "001.tmj"

#ifdef __SYMBIAN32__
#define SCALE           1
#define WINDOW_W        176
#define WINDOW_H        208
#define WINDOW_FLAGS    0
#define SCREEN_OFFSET_X 0
#define SCREEN_OFFSET_Y 0
#elif defined __3DS__
#define SCALE           1
#define WINDOW_W        400
#define WINDOW_H        240
#define WINDOW_FLAGS    0
#define FRAME_OFFSET_X  -120
#define FRAME_OFFSET_Y  -120
#define SCREEN_OFFSET_X 112
#define SCREEN_OFFSET_Y 16
#elif defined __DREAMCAST__
#define SCALE           1
#define WINDOW_W        640
#define WINDOW_H        480
#define WINDOW_FLAGS    SDL_WINDOW_FULLSCREEN
#define FRAME_OFFSET_X  0
#define FRAME_OFFSET_Y  0
#define SCREEN_OFFSET_X 232
#define SCREEN_OFFSET_Y 136
#else
#define SCALE           1
#define WINDOW_W        640
#define WINDOW_H        480
#define WINDOW_FLAGS    0
#define FRAME_OFFSET_X  0
#define FRAME_OFFSET_Y  0
#define SCREEN_OFFSET_X 232
#define SCREEN_OFFSET_Y 136
#endif

#define SCREEN_W     176
#define SCREEN_H     208
#define FRAME_WIDTH  640
#define FRAME_HEIGHT 480

#endif // CONFIG_H
