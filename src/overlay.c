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

    *pos_x = 64 + (index % 18) * 7;
    *pos_y = 146 + (index / 18) * 9;
}

void destroy_overlay(overlay_t *ui)
{
    if (ui)
    {
        if (ui->digits)
        {
            SDL_DestroyTexture(ui->digits);
            ui->digits = NULL;
        }

        if (ui->surface)
        {
            SDL_DestroyTexture(ui->surface);
            ui->surface = NULL;
        }

        if (ui->dialogue_canvas)
        {
            SDL_DestroyTexture(ui->dialogue_canvas);
            ui->dialogue_canvas = NULL;
        }

        if (ui->menu_canvas)
        {
            SDL_DestroyTexture(ui->menu_canvas);
            ui->menu_canvas = NULL;
        }

        if (ui->life_count_canvas)
        {
            SDL_DestroyTexture(ui->life_count_canvas);
            ui->life_count_canvas = NULL;
        }

        if (ui->coin_count_canvas)
        {
            SDL_DestroyTexture(ui->coin_count_canvas);
            ui->coin_count_canvas = NULL;
        }

        SDL_free(ui);
    }
}

bool load_overlay(map_t *map, overlay_t **ui, SDL_Renderer *renderer)
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

    if (!load_texture_from_file("overlay.png", &(*ui)->surface, renderer))
    {
        SDL_Log("Error loading overlay image");
        return false;
    }

    // Helper macro: create TARGET texture and blit a region of the overlay into it.
    // coin_count_canvas: 55x16
    (*ui)->coin_count_canvas = SDL_CreateTexture(renderer, pixel_format, SDL_TEXTUREACCESS_TARGET, 55, 16);
    if (!(*ui)->coin_count_canvas)
    {
        SDL_Log("Error creating coin counter texture: %s", SDL_GetError());
        return false;
    }
    SDL_SetTextureScaleMode((*ui)->coin_count_canvas, SDL_SCALEMODE_NEAREST);

    // life_count_canvas: 38x16
    (*ui)->life_count_canvas = SDL_CreateTexture(renderer, pixel_format, SDL_TEXTUREACCESS_TARGET, 38, 16);
    if (!(*ui)->life_count_canvas)
    {
        SDL_Log("Error creating life counter texture: %s", SDL_GetError());
        return false;
    }
    SDL_SetTextureScaleMode((*ui)->life_count_canvas, SDL_SCALEMODE_NEAREST);

    // menu_canvas: 96x48
    (*ui)->menu_canvas = SDL_CreateTexture(renderer, pixel_format, SDL_TEXTUREACCESS_TARGET, 96, 48);
    if (!(*ui)->menu_canvas)
    {
        SDL_Log("Error creating menu texture: %s", SDL_GetError());
        return false;
    }
    SDL_SetTextureScaleMode((*ui)->menu_canvas, SDL_SCALEMODE_NEAREST);

    SDL_SetRenderTarget(renderer, (*ui)->menu_canvas);
    SDL_FRect src_f = { .x = 0.f, .y = 16.f, .w = 96.f, .h = 48.f };
    SDL_FRect dst_f = { .x = 0.f, .y = 0.f, .w = 96.f, .h = 48.f };
    SDL_RenderTexture(renderer, (*ui)->surface, &src_f, &dst_f);
    SDL_SetRenderTarget(renderer, NULL);

    // dialogue_canvas: 176x72
    (*ui)->dialogue_canvas = SDL_CreateTexture(renderer, pixel_format, SDL_TEXTUREACCESS_TARGET, 176, 72);
    if (!(*ui)->dialogue_canvas)
    {
        SDL_Log("Error creating dialogue texture: %s", SDL_GetError());
        return false;
    }
    SDL_SetTextureScaleMode((*ui)->dialogue_canvas, SDL_SCALEMODE_NEAREST);

    SDL_SetRenderTarget(renderer, (*ui)->dialogue_canvas);
    src_f.x = 0.f; src_f.y = 74.f; src_f.w = 176.f; src_f.h = 72.f;
    dst_f.w = 176.f; dst_f.h = 72.f;
    SDL_RenderTexture(renderer, (*ui)->surface, &src_f, &dst_f);
    SDL_SetRenderTarget(renderer, NULL);

    // digits: 80x8 sub-texture
    (*ui)->digits = SDL_CreateTexture(renderer, pixel_format, SDL_TEXTUREACCESS_TARGET, 80, 8);
    if (!(*ui)->digits)
    {
        SDL_Log("Error creating digits texture: %s", SDL_GetError());
        return false;
    }
    SDL_SetTextureScaleMode((*ui)->digits, SDL_SCALEMODE_NEAREST);

    SDL_SetRenderTarget(renderer, (*ui)->digits);
    src_f.x = 58.f; src_f.y = 0.f; src_f.w = 80.f; src_f.h = 8.f;
    dst_f.x = 0.f; dst_f.y = 0.f; dst_f.w = 80.f; dst_f.h = 8.f;
    SDL_RenderTexture(renderer, (*ui)->surface, &src_f, &dst_f);
    SDL_SetRenderTarget(renderer, NULL);

    return true;
}

bool render_overlay(int coins_left, int coins_max, int life_count, map_t *map, overlay_t *ui, SDL_Renderer *renderer)
{
    ui->time_b = ui->time_a;
    ui->time_a = SDL_GetTicks();
    ui->delta_time = (ui->time_a > ui->time_b)
                         ? ui->time_a - ui->time_b
                         : ui->time_b - ui->time_a;

    if (coins_left > 9)  coins_left = 9;
    else if (coins_left < 0) coins_left = 0;
    if (coins_max > 9)   coins_max = 9;
    else if (coins_max < 0)  coins_max = 0;
    if (life_count > 99) life_count = 99;
    else if (life_count < 0) life_count = 0;

    SDL_FRect src_f;
    SDL_FRect dst_f;

    // --- coin_count_canvas ---
    SDL_SetRenderTarget(renderer, ui->coin_count_canvas);

    src_f.x = 0.f; src_f.y = 0.f; src_f.w = 54.f; src_f.h = 16.f;
    dst_f.x = 0.f; dst_f.y = 0.f; dst_f.w = 54.f; dst_f.h = 16.f;
    SDL_RenderTexture(renderer, ui->surface, &src_f, &dst_f);

    int coins = coins_max - coins_left;
    src_f.x = (float)(coins * 8); src_f.y = 0.f; src_f.w = 8.f; src_f.h = 8.f;
    dst_f.x = 16.f; dst_f.y = 4.f; dst_f.w = 8.f; dst_f.h = 8.f;
    SDL_RenderTexture(renderer, ui->digits, &src_f, &dst_f);

    src_f.x = (float)(coins_max * 8);
    dst_f.x = 42.f; dst_f.y = 4.f;
    SDL_RenderTexture(renderer, ui->digits, &src_f, &dst_f);

    SDL_SetRenderTarget(renderer, NULL);

    // --- life_count_canvas ---
    SDL_SetRenderTarget(renderer, ui->life_count_canvas);

    src_f.x = 139.f; src_f.y = 0.f; src_f.w = 37.f; src_f.h = 16.f;
    dst_f.x = 0.f;   dst_f.y = 0.f; dst_f.w = 37.f; dst_f.h = 16.f;
    SDL_RenderTexture(renderer, ui->surface, &src_f, &dst_f);

    if (life_count < 10)
    {
        src_f.x = (float)(life_count * 8); src_f.y = 0.f; src_f.w = 8.f; src_f.h = 8.f;
        dst_f.x = 27.f; dst_f.y = 4.f; dst_f.w = 8.f; dst_f.h = 8.f;
        SDL_RenderTexture(renderer, ui->digits, &src_f, &dst_f);
    }
    else
    {
        int life_first_digit  = (life_count / 10) % 10;
        int life_second_digit = life_count % 10;

        src_f.x = (float)(life_first_digit * 8); src_f.y = 0.f; src_f.w = 8.f; src_f.h = 8.f;
        dst_f.x = 19.f; dst_f.y = 4.f; dst_f.w = 8.f; dst_f.h = 8.f;
        SDL_RenderTexture(renderer, ui->digits, &src_f, &dst_f);

        src_f.x = (float)(life_second_digit * 8);
        dst_f.x = 27.f;
        SDL_RenderTexture(renderer, ui->digits, &src_f, &dst_f);
    }

    SDL_SetRenderTarget(renderer, NULL);

    // --- menu_canvas ---
    if (ui->menu_selection)
    {
        float sel_dst_y = 4.f;
        switch (ui->menu_selection)
        {
            default:
            case MENU_RESUME:
                ui->menu_canvas_offset = 0;
                sel_dst_y = 4.f;
                break;
            case MENU_SETTINGS:
                ui->menu_canvas_offset = 0;
                sel_dst_y = 19.f;
                break;
            case MENU_QUIT:
                ui->menu_canvas_offset = 0;
                sel_dst_y = 34.f;
                break;
            case MENU_MHZ:
                ui->menu_canvas_offset = 96;
                sel_dst_y = 4.f;
                break;
            case MENU_BACK:
                ui->menu_canvas_offset = 96;
                sel_dst_y = 34.f;
                break;
        }

        ui->time_since_last_frame += ui->delta_time;
        if (ui->time_since_last_frame >= (1000 / ANIM_FPS))
        {
            SDL_SetRenderTarget(renderer, ui->menu_canvas);

            // Restore left border strip.
            src_f.x = 81.f; src_f.y = 19.f; src_f.w = 13.f; src_f.h = 42.f;
            dst_f.x = 2.f;  dst_f.y = 2.f;  dst_f.w = 13.f; dst_f.h = 42.f;
            SDL_RenderTexture(renderer, ui->surface, &src_f, &dst_f);

            if (ui->menu_selection != ui->prev_selection)
            {
                src_f.x = (float)ui->menu_canvas_offset; src_f.y = 16.f; src_f.w = 96.f; src_f.h = 48.f;
                dst_f.x = 0.f; dst_f.y = 0.f; dst_f.w = 96.f; dst_f.h = 48.f;
                SDL_RenderTexture(renderer, ui->surface, &src_f, &dst_f);
            }

            if (is_overclock_enabled() && ui->menu_selection >= MENU_MHZ)
            {
                src_f.x = 58.f; src_f.y = 8.f; src_f.w = 24.f; src_f.h = 8.f;
                dst_f.x = 20.f; dst_f.y = 4.f; dst_f.w = 24.f; dst_f.h = 8.f;
                SDL_RenderTexture(renderer, ui->surface, &src_f, &dst_f);
            }

            ui->time_since_last_frame = 0;
            ui->current_frame += 1;
            if (ui->current_frame >= 12)
            {
                ui->current_frame = 0;
            }

            src_f.x = (float)(ui->current_frame * 14); src_f.y = 64.f; src_f.w = 14.f; src_f.h = 10.f;
            dst_f.x = 2.f; dst_f.y = sel_dst_y; dst_f.w = 14.f; dst_f.h = 10.f;
            SDL_RenderTexture(renderer, ui->surface, &src_f, &dst_f);

            SDL_SetRenderTarget(renderer, NULL);
        }
    }

    return true;
}

bool render_text(const char *text, bool alt_portrait, map_t *map, overlay_t *ui, SDL_Renderer *renderer)
{
    if (alt_portrait)
    {
        return render_text_ex(text, alt_portrait, 608, 146, map, ui, renderer);
    }
    else
    {
        return render_text_ex(text, alt_portrait, 615, 81, map, ui, renderer);
    }
}

bool render_text_ex(const char *text, bool alt_portrait, int portrait_x, int portrait_y, map_t *map, overlay_t *ui, SDL_Renderer *renderer)
{
    const int char_width = 7;
    const int char_height = 9;
    const int start_x_row_0_to_3 = 42;
    const int start_x_row_4_to_6 = 7;
    const int start_y = 6;
    const int line_height = 9;

    SDL_FRect src_f;
    SDL_FRect dst_f;

    SDL_SetRenderTarget(renderer, ui->dialogue_canvas);

    // Draw portrait.
    src_f.x = (float)portrait_x; src_f.y = (float)portrait_y; src_f.w = 31.f; src_f.h = 31.f;
    dst_f.x = 7.f; dst_f.y = 7.f; dst_f.w = 31.f; dst_f.h = 31.f;
    SDL_RenderTexture(renderer, ui->surface, &src_f, &dst_f);

    src_f.w = (float)char_width;
    src_f.h = (float)char_height;
    dst_f.x = (float)start_x_row_0_to_3;
    dst_f.y = (float)start_y;
    dst_f.w = (float)char_width;
    dst_f.h = (float)char_height;

    int index = 0;
    int char_index = 0;
    int char_pos_x, char_pos_y;

    while (index < 141)
    {
        if (text[char_index] != '\0')
        {
            get_character_position(text[char_index], &char_pos_x, &char_pos_y);
            char_index++;
        }
        else
        {
            char_pos_x = 64;
            char_pos_y = 146;
        }

        src_f.x = (float)char_pos_x;
        src_f.y = (float)char_pos_y;
        SDL_RenderTexture(renderer, ui->surface, &src_f, &dst_f);

        index++;

        int row = 0;
        if (index > 117)      row = 6;
        else if (index > 93)  row = 5;
        else if (index > 71)  row = 4;
        else if (index > 53)  row = 3;
        else if (index > 35)  row = 2;
        else if (index > 17)  row = 1;

        dst_f.y = (float)(start_y + row * line_height);

        if (index == 18 || index == 36 || index == 54 ||
            index == 72 || index == 95 || index == 118)
        {
            dst_f.x = (float)(row < 4 ? start_x_row_0_to_3 : start_x_row_4_to_6);
        }
        else
        {
            dst_f.x += (float)char_width;
        }
    }

    SDL_SetRenderTarget(renderer, NULL);

    return true;
}
