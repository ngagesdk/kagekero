/** @file overlay.h
 *
 *  A minimalist, cross-platform puzzle-platformer, designed
 *  especially for the Nokia N-Gage.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#ifndef OVERLAY_H
#define OVERLAY_H

#include <SDL3/SDL.h>

typedef struct overlay
{
    SDL_Texture* render_canvas;

} overlay_t;

void destroy_overlay(overlay_t *ui);
bool load_overlay(overlay_t **ui);

#endif // OVERLAY_H
