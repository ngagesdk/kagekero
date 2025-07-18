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
#include "map.h"
#include "overclock.h"
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

    *pos_x = 672 + (index % 18) * 7;
    *pos_y = 146 + (index / 18) * 9;
}

void destroy_overlay(overlay_t *ui)
{
    if (ui)
    {
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

        SDL_free(ui);
    }
}

bool load_overlay(map_t *map, overlay_t **ui)
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
    src.x = 608;
    src.y = 16;
    src.w = 96;
    src.h = 48;

    if (!SDL_BlitSurface(map->tileset_surface, &src, (*ui)->menu_canvas, NULL))
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

    src.x = 608;
    src.y = 74;
    src.w = 176;
    src.h = 72;

    if (!SDL_BlitSurface(map->tileset_surface, &src, (*ui)->dialogue_canvas, NULL))
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

    src.x = 666;
    src.y = 0;
    src.w = 80;
    src.h = 8;

    if (!SDL_BlitSurface(map->tileset_surface, &src, (*ui)->digits, NULL))
    {
        SDL_Log("Error blitting to coin counter canvas: %s", SDL_GetError());
        return false;
    }

    return true;
}

bool render_overlay(int coins_left, int coins_max, int life_count, map_t *map, overlay_t *ui)
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
    src.x = 608;
    src.y = 0;
    src.w = 54;
    src.h = 16;

    if (!SDL_BlitSurface(map->tileset_surface, &src, ui->coin_count_canvas, NULL))
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

    src.x = 747;
    src.w = 37;
    src.h = 16;

    if (!SDL_BlitSurface(map->tileset_surface, &src, ui->life_count_canvas, NULL))
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
            case MENU_RESUME:
                ui->menu_canvas_offset = 0;
                dst.y = 4;
                break;
            case MENU_SETTINGS:
                ui->menu_canvas_offset = 0;
                dst.y = 19;
                break;
            case MENU_QUIT:
                ui->menu_canvas_offset = 0;
                dst.y = 34;
                break;
            case MENU_MHZ:
                ui->menu_canvas_offset = 96;
                break;
            case MENU_BACK:
                ui->menu_canvas_offset = 96;
                dst.y = 34;
                break;
        }

        ui->time_since_last_frame += ui->delta_time;
        if (ui->time_since_last_frame >= (1000 / ANIM_FPS))
        {
            SDL_Rect tmp_dst;
            SDL_Rect tmp_src;

            // This can be optimised if required.
            tmp_src.x = 689;
            tmp_src.y = 19;
            tmp_src.w = 13;
            tmp_src.h = 42;

            tmp_dst.x = 2;
            tmp_dst.y = 2;
            tmp_dst.w = 13;
            tmp_dst.h = 42;

            if (!SDL_BlitSurface(map->tileset_surface, &tmp_src, ui->menu_canvas, &tmp_dst))
            {
                SDL_Log("Error blitting menu background to canvas: %s", SDL_GetError());
                return false;
            }

            if (ui->menu_selection != ui->prev_selection)
            {
                SDL_Rect src;
                src.x = 608 + ui->menu_canvas_offset;
                src.y = 16;
                src.w = 96;
                src.h = 48;

                if (!SDL_BlitSurface(map->tileset_surface, &src, ui->menu_canvas, NULL))
                {
                    SDL_Log("Error blitting to menu canvas: %s", SDL_GetError());
                    return false;
                }
            }

            if (is_overclock_enabled() && ui->menu_selection >= MENU_MHZ)
            {
                tmp_src.x = 666;
                tmp_src.y = 8;
                tmp_src.w = 24;
                tmp_src.h = 8;

                tmp_dst.x = 20;
                tmp_dst.y = 4;
                tmp_dst.w = 24;
                tmp_dst.h = 8;

                if (!SDL_BlitSurface(map->tileset_surface, &tmp_src, ui->menu_canvas, &tmp_dst))
                {
                    SDL_Log("Error blitting to menu canvas: %s", SDL_GetError());
                    return false;
                }
            }

            ui->time_since_last_frame = 0;
            ui->current_frame += 1;

            if (ui->current_frame >= 12)
            {
                ui->current_frame = 0;
            }

            src.x = 608 + (ui->current_frame * 14);
            src.y = 64;
            src.w = 14;
            src.h = 10;

            if (!SDL_BlitSurface(map->tileset_surface, &src, ui->menu_canvas, &dst))
            {
                SDL_Log("Error blitting menu selection to canvas: %s", SDL_GetError());
                return false;
            }
        }
    }

    return true;
}

bool render_text(const char *text, bool alt_portrait, map_t *map, overlay_t *ui)
{
    if (alt_portrait)
    {
        return (render_text_ex(text, alt_portrait, 608, 146, map, ui));
    }
    else
    {
        return (render_text_ex(text, alt_portrait, 615, 81, map, ui));
    }
}

bool render_text_ex(const char *text, bool alt_portrait, int portrait_x, int portrait_y, map_t *map, overlay_t *ui)
{
    const int char_width = 7;
    const int char_height = 9;
    const int start_x_row_0_to_3 = 42;
    const int start_x_row_4_to_6 = 7;
    const int start_y = 6;
    const int line_height = 9;

    SDL_Rect src;
    SDL_Rect dst;

    src.x = portrait_x;
    src.y = portrait_y;
    src.w = 31;
    src.h = 31;

    dst.x = 7;
    dst.y = 7;
    dst.w = 31;
    dst.h = 31;

    if (!SDL_BlitSurface(map->tileset_surface, &src, ui->dialogue_canvas, &dst))
    {
        SDL_Log("Error blitting to dialogue canvas: %s", SDL_GetError());
        return false;
    }

    src.x = 0;
    src.y = 0;
    src.w = char_width;
    src.h = char_height;

    dst.x = start_x_row_0_to_3;
    dst.y = start_y;
    dst.w = char_width;
    dst.h = char_height;

    int index = 0;
    int char_index = 0;

    while (index < 141)
    {
        if (text[char_index] != '\0')
        {
            get_character_position(text[char_index], &src.x, &src.y);
            char_index++;
        }
        else
        {
            src.x = 672;
            src.y = 146;
        }

        if (!SDL_BlitSurface(map->tileset_surface, &src, ui->dialogue_canvas, &dst))
        {
            SDL_Log("Error blitting character to canvas: %s", SDL_GetError());
        }

        index++;

        int row = 0;
        if (index > 117)
        {
            row = 6;
        }
        else if (index > 93)
        {
            row = 5;
        }
        else if (index > 71)
        {
            row = 4;
        }
        else if (index > 53)
        {
            row = 3;
        }
        else if (index > 35)
        {
            row = 2;
        }
        else if (index > 17)
        {
            row = 1;
        }

        dst.y = start_y + row * line_height;

        if (index == 18 || index == 36 || index == 54 ||
            index == 72 || index == 95 || index == 118)
        {
            if (row < 4)
            {
                dst.x = start_x_row_0_to_3;
            }
            else
            {
                dst.x = start_x_row_4_to_6;
            }
        }
        else
        {
            dst.x += char_width;
        }
    }

    return true;
}
