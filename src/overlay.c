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

#include "config.h"
#include "overlay.h"
#include "utils.h"

static void get_character_position(const unsigned char character, int *pos_x, int *pos_y)
{
    int index = 0;

    // If the character is not valid, select space.
    if ((character < 0x20) || (character > 0x7e))
    {
        index = 0;
    }
    else
    {
        index = character - 0x20;
    }

    *pos_x = (index % 18) * 7;
    *pos_y = (index / 18) * 9;
}

void destroy_overlay(overlay_t *ui)
{
    if (ui)
    {
        if (ui->font)
        {
            SDL_DestroySurface(ui->font);
            ui->font = NULL;
        }

        if (ui->digits)
        {
            SDL_DestroySurface(ui->digits);
            ui->digits = NULL;
        }

        if (ui->dialogue_canvas)
        {
            SDL_DestroySurface(ui->dialogue_canvas);
            ui->dialogue_canvas = NULL;
        }

        if (ui->menu_canvas)
        {
            SDL_DestroySurface(ui->menu_canvas);
            ui->menu_canvas = NULL;
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

    (*ui)->menu_canvas = SDL_CreateSurface(96, 48, pixel_format);
    if (!(*ui)->menu_canvas)
    {
        SDL_Log("Error creating menu surface: %s", SDL_GetError());
        return false;
    }

    SDL_Rect src;
    src.x = 0;
    src.y = 16;
    src.w = 96;
    src.h = 48;

    if (!SDL_BlitSurface((*ui)->image, &src, (*ui)->menu_canvas, NULL))
    {
        SDL_Log("Error blitting to menu canvas: %s", SDL_GetError());
        return false;
    }

    (*ui)->dialogue_canvas = SDL_CreateSurface(176, 72, pixel_format);
    if (!(*ui)->dialogue_canvas)
    {
        SDL_Log("Error creating dialogue surface: %s", SDL_GetError());
        return false;
    }

    src.x = 0;
    src.y = 74;
    src.w = 176;
    src.h = 72;

    if (!SDL_BlitSurface((*ui)->image, &src, (*ui)->dialogue_canvas, NULL))
    {
        SDL_Log("Error blitting to dialogue canvas: %s", SDL_GetError());
        return false;
    }

    (*ui)->digits = SDL_CreateSurface(80, 8, pixel_format);
    if (!(*ui)->digits)
    {
        SDL_Log("Error creating digits surface: %s", SDL_GetError());
        return false;
    }

    src.x = 58;
    src.y = 0;
    src.w = 80;
    src.h = 8;

    if (!SDL_BlitSurface((*ui)->image, &src, (*ui)->digits, NULL))
    {
        SDL_Log("Error blitting to coin counter canvas: %s", SDL_GetError());
        return false;
    }

    if (!load_surface_from_file("font.png", &(*ui)->font))
    {
        SDL_Log("Failed to load font image: %s", SDL_GetError());
        return false;
    }

    return true;
}

bool render_overlay(int coins_left, int coins_max, int life_count, overlay_t *ui)
{
    ui->time_b = ui->time_a;
    ui->time_a = SDL_GetTicks();
    ui->delta_time = (ui->time_a > ui->time_b)
                         ? ui->time_a - ui->time_b
                         : ui->time_b - ui->time_a;

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

    if (!SDL_BlitSurface(ui->digits, &src, ui->coin_count_canvas, &dst))
    {
        SDL_Log("Error blitting coin count to canvas: %s", SDL_GetError());
        return false;
    }

    src.x = coins_max * 8;
    dst.x = 42;
    dst.y = 4;

    if (!SDL_BlitSurface(ui->digits, &src, ui->coin_count_canvas, &dst))
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

        if (!SDL_BlitSurface(ui->digits, &src, ui->life_count_canvas, &dst))
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

        if (!SDL_BlitSurface(ui->digits, &src, ui->life_count_canvas, &dst))
        {
            SDL_Log("Error blitting first life digit to canvas: %s", SDL_GetError());
            return false;
        }

        src.x = life_second_digit * 8;
        dst.x = 27;

        if (!SDL_BlitSurface(ui->digits, &src, ui->life_count_canvas, &dst))
        {
            SDL_Log("Error blitting second life digit to canvas: %s", SDL_GetError());
            return false;
        }
    }

    if (ui->menu_selection)
    {
        dst.w = 14;
        dst.h = 10;
        dst.x = 2;

        switch (ui->menu_selection)
        {
            default:
            case MENU_NONE:
            case MENU_RESUME:
                dst.y = 4;
                break;
            case MENU_SETTINGS:
                dst.y = 19;
                break;
            case MENU_QUIT:
                dst.y = 34;
                break;
        }

        ui->time_since_last_frame += ui->delta_time;
        if (ui->time_since_last_frame >= (1000 / ANIM_FPS))
        {
            SDL_Rect tmp_dst;
            SDL_Rect tmp_src;

            // This can be optimised if required.
            tmp_src.x = 81;
            tmp_src.y = 19;
            tmp_src.w = 13;
            tmp_src.h = 42;

            tmp_dst.x = 2;
            tmp_dst.y = 2;
            tmp_dst.w = 13;
            tmp_dst.h = 42;

            if (!SDL_BlitSurface(ui->image, &tmp_src, ui->menu_canvas, &tmp_dst))
            {
                SDL_Log("Error blitting menu background to canvas: %s", SDL_GetError());
                return false;
            }

            ui->time_since_last_frame = 0;
            ui->current_frame += 1;

            if (ui->current_frame >= 12)
            {
                ui->current_frame = 0;
            }

            src.x = 0 + (ui->current_frame * 14);
            src.y = 64;
            src.w = 14;
            src.h = 10;

            if (!SDL_BlitSurface(ui->image, &src, ui->menu_canvas, &dst))
            {
                SDL_Log("Error blitting menu selection to canvas: %s", SDL_GetError());
                return false;
            }
        }
    }

    return true;
}

bool render_text(const char* text, int pos_x, int pos_y, overlay_t* ui)
{
    SDL_Rect src;
    src.x = 0;
    src.y = 0;
    src.w = 7;
    src.h = 9;

    SDL_Rect dst;
    dst.x = pos_x;
    dst.y = pos_y;
    dst.w = 7;
    dst.h = 9;

    int char_index = 0;
    while ('\0' != text[char_index])
    {
        get_character_position(text[char_index], &src.x, &src.y);
        char_index += 1;

        if (!SDL_BlitSurface(ui->font, &src, ui->dialogue_canvas, &dst))
        {
            SDL_Log("Error blitting character to canvas: %s", SDL_GetError());
        }
        dst.x += 7;
    }

    return true;
}
