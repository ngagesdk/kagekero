/** @file app.h
 *
 *  A minimalist, cross-platform puzzle-platformer, designed
 *  especially for the Nokia N-Gage.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#ifndef APP_H
#define APP_H

#include <SDL3/SDL.h>

bool init_app(SDL_Renderer **renderer, SDL_Window *window);
void destroy_app(void);

#endif // APP_H
