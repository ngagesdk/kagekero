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

    (*ui)->coin_count_canvas = SDL_CreateSurface(55, 16, pixel_format);
    if (!(*ui)->coin_count_canvas)
    {
        SDL_Log("Error creating coin counter surface: %s", SDL_GetError());
        return false;
    }

    (*ui)->life_count_canvas = SDL_CreateSurface(38, 16, pixel_format);
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
    src.x = 58;
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

bool render_overlay(int coins_left, int coins_max, int life_count, overlay_t *ui)
{
    if (coins_left > 9)
    {
        coins_left = 9;
    }
    else if (coins_left < 0)
    {
        coins_left = 0;
    }

    if (coins_max > 9)
    {
        coins_max = 9;
    }
    else if (coins_max < 0)
    {
        coins_max = 0;
    }

    if (life_count > 99)
    {
        life_count = 99;
    }
    else if (life_count < 0)
    {
        life_count = 0;
    }

    SDL_Rect src;
    src.x = 0;
    src.y = 0;
    src.w = 54;
    src.h = 16;

    if (!SDL_BlitSurface(ui->image, &src, ui->coin_count_canvas, NULL))
    {
        SDL_Log("Error blitting to coin counter canvas: %s", SDL_GetError());
        return false;
    }

    int coins = coins_max - coins_left;

    src.x = coins * 8;
    src.w = 8;
    src.h = 8;

    SDL_Rect dst;
    dst.x = 16;
    dst.y = 4;
    dst.w = 8;
    dst.h = 8;

    if (!SDL_BlitSurface(ui->font, &src, ui->coin_count_canvas, &dst))
    {
        SDL_Log("Error blitting coin count to canvas: %s", SDL_GetError());
        return false;
    }

    src.x = coins_max * 8;
    dst.x = 42;
    dst.y = 4;

    if (!SDL_BlitSurface(ui->font, &src, ui->coin_count_canvas, &dst))
    {
        SDL_Log("Error blitting coin count to canvas: %s", SDL_GetError());
        return false;
    }

    src.x = 139;
    src.w = 37;
    src.h = 16;

    if (!SDL_BlitSurface(ui->image, &src, ui->life_count_canvas, NULL))
    {
        SDL_Log("Error blitting to life counter canvas: %s", SDL_GetError());
        return false;
    }

    if (life_count < 10)
    {
        src.x = life_count * 8;
        src.w = 8;
        src.h = 8;

        dst.x = 27;
        dst.w = 8;
        dst.h = 8;

        if (!SDL_BlitSurface(ui->font, &src, ui->life_count_canvas, &dst))
        {
            SDL_Log("Error blitting life count to canvas: %s", SDL_GetError());
            return false;
        }
    }
    else
    {
        int life_first_digit = (life_count / 10) % 10;
        int life_second_digit = life_count % 10;

        src.x = life_first_digit * 8;
        src.w = 8;
        src.h = 8;

        dst.x = 19;
        dst.y = 4;
        dst.w = 8;
        dst.h = 8;

        if (!SDL_BlitSurface(ui->font, &src, ui->life_count_canvas, &dst))
        {
            SDL_Log("Error blitting first life digit to canvas: %s", SDL_GetError());
            return false;
        }

        src.x = life_second_digit * 8;
        dst.x = 27;

        if (!SDL_BlitSurface(ui->font, &src, ui->life_count_canvas, &dst))
        {
            SDL_Log("Error blitting second life digit to canvas: %s", SDL_GetError());
            return false;
        }
    }

    return true;
}
