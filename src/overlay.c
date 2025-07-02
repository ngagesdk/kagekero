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
#include "utils.h"

void destroy_overlay(overlay_t *ui)
{
    if (ui)
    {
        if (ui->font)
        {
            SDL_DestroySurface(ui->font);
            ui->font = NULL;
        }

        if (ui->life_count_canvas)
        {
            SDL_DestroySurface(ui->life_count_canvas);
            ui->life_count_canvas = NULL;
        }

        if (ui->coin_count_canvas)
        {
            SDL_DestroySurface(ui->coin_count_canvas);
            ui->coin_count_canvas = NULL;
        }

        if (ui->image)
        {
            SDL_DestroySurface(ui->image);
            ui->image = NULL;
        }

        SDL_free(ui);
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

    if (!load_surface_from_file("overlay.png", &(*ui)->image))
    {
        SDL_Log("Failed to load overlay image: %s", SDL_GetError());
        return false;
    }

#ifndef __DREAMCAST__
    SDL_PixelFormat pixel_format = SDL_PIXELFORMAT_XRGB4444;
#else
    SDL_PixelFormat pixel_format = SDL_PIXELFORMAT_ARGB1555;
#endif

    (*ui)->coin_count_canvas = SDL_CreateSurface(57, 16, pixel_format);
    if (!(*ui)->coin_count_canvas)
    {
        SDL_Log("Error creating coin counter surface: %s", SDL_GetError());
        return false;
    }

    (*ui)->life_count_canvas = SDL_CreateSurface(37, 16, pixel_format);
    if (!(*ui)->life_count_canvas)
    {
        SDL_Log("Error creating life counter surface: %s", SDL_GetError());
        return false;
    }

    (*ui)->font = SDL_CreateSurface(80, 8, pixel_format);
    if (!(*ui)->font)
    {
        SDL_Log("Error creating font surface: %s", SDL_GetError());
        return false;
    }

    SDL_Rect src;
    src.x = 57;
    src.y = 0;
    src.w = 80;
    src.h = 8;

    if (!SDL_BlitSurface((*ui)->image, &src, (*ui)->font, NULL))
    {
        SDL_Log("Error blitting to coin counter canvas: %s", SDL_GetError());
        return false;
    }

    return true;
}

bool render_overlay(int coint_count, int life_count, overlay_t *ui)
{
    SDL_Rect src;
    src.x = 0;
    src.y = 0;
    src.w = 56;
    src.h = 16;

    if (!SDL_BlitSurface(ui->image, &src, ui->coin_count_canvas, NULL))
    {
        SDL_Log("Error blitting to coin counter canvas: %s", SDL_GetError());
        return false;
    }

    src.x = 139;
    src.y = 0;
    src.w = 37;
    src.h = 16;

    if (!SDL_BlitSurface(ui->image, &src, ui->life_count_canvas, NULL))
    {
        SDL_Log("Error blitting to life counter canvas: %s", SDL_GetError());
        return false;
    }

    return true;
}
