/** @file kagekero.c
 *
 *  A minimalist, cross-platform puzzle-platformer, designed
 *  especially for the Nokia N-Gage.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#include <SDL3/SDL.h>

#include "app.h"
#include "config.h"
#include "kagekero.h"
#include "kero.h"
#include "map.h"
#include "pfs.h"
#include "utils.h"

bool init_kagekero(kagekero_t **nc)
{
    *nc = (kagekero_t *)SDL_calloc(1, sizeof(kagekero_t));
    if (!*nc)
    {
        SDL_Log("Failed to allocate memory for engine core");
        return false;
    }

    if (!init_app((SDL_Renderer **)&(*nc)->renderer, (*nc)->window))
    {
        return SDL_APP_FAILURE;
    }

    init_file_reader();

    if (!load_map("001.tmj", &(*nc)->map, (*nc)->renderer))
    {
        return false;
    }

    if (!load_kero(&(*nc)->kero, (*nc)->map))
    {
        SDL_Log("Failed to load kero");
        return false;
    }

    if (!render_map((*nc)->map, (*nc)->renderer))
    {
        SDL_Log("Failed to render map");
        return false;
    }

#ifndef __SYMBIAN32__
    if (!load_texture_from_file("frame.png", &(*nc)->frame, (*nc)->renderer))
    {
        return false;
    }
#endif

    return true;
}

bool update_kagekero(kagekero_t *nc)
{
    update_kero(nc->kero, nc->map, &nc->btn);

    nc->cam_x = (int)nc->kero->pos_x - (SCREEN_W / 2);
    nc->cam_y = (int)nc->kero->pos_y - (SCREEN_H / 2);

    render_map(nc->map, nc->renderer);
    render_kero(nc->kero, nc->map);

#ifndef __SYMBIAN32__
    SDL_FRect src;
    src.w = FRAME_WIDTH;
    src.h = FRAME_HEIGHT;
    src.x = src.y = 0.f;

    SDL_FRect dst;
    dst.w = FRAME_WIDTH;
    dst.h = FRAME_HEIGHT;
    dst.x = FRAME_OFFSET_X;
    dst.y = FRAME_OFFSET_Y;

    SDL_RenderTexture(nc->renderer, nc->frame, &src, &dst);
#endif

    return true;
}

bool draw_kagekero_scene(kagekero_t *nc)
{
    if (!SDL_SetRenderDrawColor(nc->renderer, nc->map->bg_r, nc->map->bg_g, nc->map->bg_b, 255))
    {
        SDL_Log("Error setting render draw color: %s", SDL_GetError());
        return false;
    }

    SDL_FRect dst;
#ifndef __SYMBIAN32__
    dst.x = SCREEN_OFFSET_X;
    dst.y = SCREEN_OFFSET_Y;
    dst.w = SCREEN_W;
    dst.h = SCREEN_H;

    if (!SDL_RenderFillRect(nc->renderer, &dst))
    {
        SDL_Log("Error drawing background rect: %s", SDL_GetError());
        return false;
    }
#else
    SDL_RenderClear(nc->renderer);
#endif

    if (!SDL_UpdateTexture(nc->map->render_target, NULL, nc->map->render_canvas->pixels, nc->map->render_canvas->pitch))
    {
        SDL_Log("Error updating animated tile texture: %s", SDL_GetError());
        return false;
    }

    SDL_Rect dst_rect;
    dst_rect.x = (int)nc->kero->pos_x - KERO_HALF;
    dst_rect.y = (int)nc->kero->pos_y - KERO_HALF;
    dst_rect.w = KERO_SIZE;
    dst_rect.h = KERO_SIZE;

    if (!SDL_UpdateTexture(nc->map->render_target, &dst_rect, nc->kero->render_canvas->pixels, nc->kero->render_canvas->pitch))
    {
        SDL_Log("Error updating animated tile texture: %s", SDL_GetError());
        return false;
    }

    if (nc->cam_x <= 0)
    {
        nc->cam_x = 0;
    }
    else if (nc->cam_x >= nc->map->width - SCREEN_W)
    {
        nc->cam_x = nc->map->width - SCREEN_W;
    }
    if (nc->cam_y <= 0)
    {
        nc->cam_y = 0;
    }
    else if (nc->cam_y >= nc->map->height - SCREEN_H)
    {
        nc->cam_y = nc->map->height - SCREEN_H;
    }

    SDL_FRect src;
    src.x = (float)(0 + nc->cam_x);
    src.y = (float)(0 + nc->cam_y);
    src.w = SCREEN_W;
    src.h = SCREEN_H;

    dst.x = SCREEN_OFFSET_X;
    dst.y = SCREEN_OFFSET_Y;
    dst.w = SCREEN_W;
    dst.h = SCREEN_H;

    if (!SDL_RenderTexture(nc->renderer, nc->map->render_target, &src, &dst))
    {
        SDL_Log("Error rendering texture: %s", SDL_GetError());
        return false;
    }
    SDL_RenderPresent(nc->renderer);

    return true;
}

bool handle_kagekero_events(kagekero_t *nc)
{
    switch (nc->event->type)
    {
        case SDL_EVENT_QUIT:
            {
                return false;
            }
        case SDL_EVENT_GAMEPAD_ADDED:
            {
                const SDL_JoystickID which = nc->event->gdevice.which;
                SDL_Gamepad *gamepad = SDL_OpenGamepad(which);
                if (!gamepad)
                {
                    SDL_LogDebug(SDL_LOG_CATEGORY_INPUT, "Joystick #%" SDL_PRIu32 " could not be opened: %s", which, SDL_GetError());
                }
                else
                {
                    SDL_LogDebug(SDL_LOG_CATEGORY_INPUT, "Joystick #%" SDL_PRIu32 " connected: %s", which, SDL_GetGamepadName(gamepad));
                }
                return true;
            }
        case SDL_EVENT_GAMEPAD_REMOVED:
            {
                const SDL_JoystickID which = nc->event->gdevice.which;
                SDL_Gamepad *gamepad = SDL_GetGamepadFromID(which);
                if (gamepad)
                {
                    SDL_CloseGamepad(gamepad); /* the joystick was unplugged. */
                }
                return true;
            }
        case SDL_EVENT_KEY_DOWN:
            {
                if (nc->event->key.repeat)
                { // No key repeat.
                    break;
                }

                button_t button = get_button_from_key(nc->event->key.key);
                set_bit(&nc->btn, button);
                break;
            }
        case SDL_EVENT_KEY_UP:
            {
                button_t button = get_button_from_key(nc->event->key.key);
                clear_bit(&nc->btn, button);
                break;
            }
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
            {
                const SDL_JoystickID which = nc->event->gbutton.which;
                button_t button = get_button_from_gamepad(nc->event->gbutton.button);
                set_bit(&nc->btn, button);

                break;
            }
        case SDL_EVENT_GAMEPAD_BUTTON_UP:
            {
                const SDL_JoystickID which = nc->event->gbutton.which;
                button_t button = get_button_from_gamepad(nc->event->gbutton.button);
                clear_bit(&nc->btn, button);

                break;
            }
    }

    if (check_bit(nc->btn, BTN_SOFTLEFT))
    {
        return false;
    }

    return true;
}

void destroy_kagekero(kagekero_t *nc)
{
    if (nc)
    {
        if (nc->kero)
        {
            destroy_kero(nc->kero);
            nc->kero = NULL;
        }

        if (nc->map)
        {
            destroy_map(nc->map);
            nc->map = NULL;
        }

#ifndef __SYMBIAN32__
        if (nc->frame)
        {
            SDL_DestroyTexture(nc->frame);
            nc->frame = NULL;
        }
#endif

        if (nc->renderer)
        {
            SDL_DestroyRenderer(nc->renderer);
            nc->renderer = NULL;
        }

        if (nc->window)
        {
            SDL_DestroyWindow(nc->window);
            nc->window = NULL;
        }

        SDL_free(nc);
    }

    destroy_app();
}
