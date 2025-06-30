/** @file overlay.c
 *
 *  A minimalist, cross-platform puzzle-platformer, designed
 *  especially for the Nokia N-Gage.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#include <SDL3/SDL.h>

#include "overlay.h"

void destroy_overlay(overlay_t *ui)
{
    if (ui)
    {
        if (ui->render_canvas)
        {
            SDL_DestroySurface(ui->render_canvas);
            ui->render_canvas = NULL;
        }
    }
}

bool load_overlay(overlay_t **ui)
{
    *ui = (overlay_t *)SDL_calloc(1, sizeof(overlay_t));
    if (!*ui)
    {
        SDL_Log("Failed to allocate memory for overlay");
        return false;
    }

#ifndef __DREAMCAST__
    SDL_PixelFormat pixel_format = SDL_PIXELFORMAT_XRGB4444;
#else
    SDL_PixelFormat pixel_format = SDL_PIXELFORMAT_ARGB1555;
#endif

    (*ui)->render_canvas = SDL_CreateSurface(176, 12, pixel_format);
    if (!(*ui)->render_canvas)
    {
        SDL_Log("Error creating temporary surface: %s", SDL_GetError());
        return false;
    }

    return true;
}

bool render_overlay(overlay_t* ui)
{
    return true;
}
