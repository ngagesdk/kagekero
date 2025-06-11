/** @file ncore.c
 *
 *  A cross-platform engine with native Nokia N-Gage compatibility.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#include <SDL3/SDL.h>

#include "app.h"
#include "config.h"
#include "hero.h"
#include "map.h"
#include "ncore.h"
#include "pfs.h"
#include "utils.h"

bool init_ncore(ncore_t **nc)
{
    *nc = (ncore_t *)SDL_calloc(1, sizeof(ncore_t));
    if (!*nc) {
        SDL_Log("Failed to allocate memory for engine core");
        return false;
    }

    if (!init_app((SDL_Renderer **)&(*nc)->renderer, (*nc)->window)) {
        return SDL_APP_FAILURE;
    }

    init_file_reader();

    if (!load_map("001.tmj", &(*nc)->map, (*nc)->renderer)) {
        return false;
    }

    if (!load_hero(&(*nc)->hero, 80.f, 136.f)) {
        SDL_Log("Failed to load hero");
        return false;
    }

    if (!render_map((*nc)->map, (*nc)->renderer)) {
        SDL_Log("Failed to render map");
        return false;
    }

#ifndef __SYMBIAN32__
    if (!load_texture_from_file("frame.png", &(*nc)->frame, (*nc)->renderer)) {
        return false;
    }
#endif

    return true;
}

bool update_ncore(ncore_t *nc)
{
    update_hero(nc->hero, nc->map, &nc->btn);

    nc->cam_x = (int)nc->hero->pos_x - (SCREEN_W / 2);
    nc->cam_y = (int)nc->hero->pos_y - (SCREEN_H / 2);

    render_map(nc->map, nc->renderer);
    render_hero(nc->hero, nc->map);

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

bool draw_ncore_scene(ncore_t *nc)
{
    if (!SDL_SetRenderDrawColor(nc->renderer, nc->map->bg_r, nc->map->bg_g, nc->map->bg_b, 255)) {
        SDL_Log("Error setting render draw color: %s", SDL_GetError());
        return false;
    }

    SDL_FRect dst;
#ifndef __SYMBIAN32__
    dst.x = SCREEN_OFFSET_X;
    dst.y = SCREEN_OFFSET_Y;
    dst.w = SCREEN_W;
    dst.h = SCREEN_H;

    if (!SDL_RenderFillRect(nc->renderer, &dst)) {
        SDL_Log("Error drawing background rect: %s", SDL_GetError());
        return false;
    }
#else
    SDL_RenderClear(nc->renderer);
#endif

    if (!SDL_UpdateTexture(nc->map->render_target, NULL, nc->map->render_canvas->pixels, nc->map->render_canvas->pitch)) {
        SDL_Log("Error updating animated tile texture: %s", SDL_GetError());
        return false;
    }

    SDL_Rect dst_rect;
    dst_rect.x = (int)nc->hero->pos_x - 8;
    dst_rect.y = (int)nc->hero->pos_y - 8;
    dst_rect.w = 32;
    dst_rect.h = 32;

    if (!SDL_UpdateTexture(nc->map->render_target, &dst_rect, nc->hero->render_canvas->pixels, nc->hero->render_canvas->pitch)) {
        SDL_Log("Error updating animated tile texture: %s", SDL_GetError());
        return false;
    }

    if (nc->cam_x <= 0) {
        nc->cam_x = 0;
    } else if (nc->cam_x >= nc->map->width - SCREEN_W) {
        nc->cam_x = nc->map->width - SCREEN_W;
    }
    if (nc->cam_y <= 0) {
        nc->cam_y = 0;
    } else if (nc->cam_y >= nc->map->height - SCREEN_H) {
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

    if (!SDL_RenderTexture(nc->renderer, nc->map->render_target, &src, &dst)) {
        SDL_Log("Error rendering texture: %s", SDL_GetError());
        return false;
    }
    SDL_RenderPresent(nc->renderer);

    return true;
}

bool handle_ncore_events(ncore_t *nc)
{
    switch (nc->event->type) {
    case SDL_EVENT_QUIT:
    {
        return false;
    }
    case SDL_EVENT_GAMEPAD_ADDED:
    {
        const SDL_JoystickID which = nc->event->gdevice.which;
        SDL_Gamepad *gamepad = SDL_OpenGamepad(which);
        if (!gamepad) {
            // SDL_Log("Joystick #%" SDL_PRIu32 " could not be opened: %s", which, SDL_GetError());
        } else {
            // SDL_Log("Joystick #%" SDL_PRIu32 " connected: %s", which, SDL_GetGamepadName(gamepad));
        }
        return true;
    }
    case SDL_EVENT_GAMEPAD_REMOVED:
    {
        const SDL_JoystickID which = nc->event->gdevice.which;
        SDL_Gamepad *gamepad = SDL_GetGamepadFromID(which);
        if (gamepad) {
            SDL_CloseGamepad(gamepad); /* the joystick was unplugged. */
        }
        return true;
    }
    case SDL_EVENT_KEY_DOWN:
    {
        if (nc->event->key.repeat) { // No key repeat.
            break;
        }

        if (SDLK_BACKSPACE == nc->event->key.key) {
            set_bit(&nc->btn, BTN_BACKSPACE);
        }

        if (SDLK_1 == nc->event->key.key) {
            set_bit(&nc->btn, BTN_1);
        }

        if (SDLK_2 == nc->event->key.key) {
            set_bit(&nc->btn, BTN_2);
        }

        if (SDLK_3 == nc->event->key.key) {
            set_bit(&nc->btn, BTN_3);
        }

        if (SDLK_4 == nc->event->key.key) {
            set_bit(&nc->btn, BTN_4);
        }

        if (SDLK_5 == nc->event->key.key) {
            set_bit(&nc->btn, BTN_5);
        }

        if (SDLK_6 == nc->event->key.key) {
            set_bit(&nc->btn, BTN_6);
        }

        if (SDLK_7 == nc->event->key.key) {
            set_bit(&nc->btn, BTN_7);
        }

        if (SDLK_8 == nc->event->key.key) {
            set_bit(&nc->btn, BTN_8);
        }

        if (SDLK_9 == nc->event->key.key) {
            set_bit(&nc->btn, BTN_9);
        }

        if (SDLK_0 == nc->event->key.key) {
            set_bit(&nc->btn, BTN_0);
        }

        if (SDLK_ASTERISK == nc->event->key.key) {
            set_bit(&nc->btn, BTN_ASTERISK);
        }

        if (SDLK_HASH == nc->event->key.key) {
            set_bit(&nc->btn, BTN_HASH);
        }

        if (SDLK_SOFTLEFT == nc->event->key.key || SDLK_ESCAPE == nc->event->key.key) {
            set_bit(&nc->btn, BTN_SOFTLEFT);
        }

        if (SDLK_SOFTRIGHT == nc->event->key.key) {
            set_bit(&nc->btn, BTN_SOFTRIGHT);
        }

        if (SDLK_SELECT == nc->event->key.key) {
            set_bit(&nc->btn, BTN_SELECT);
        }

        if (SDLK_UP == nc->event->key.key) {
            set_bit(&nc->btn, BTN_UP);
        }

        if (SDLK_DOWN == nc->event->key.key) {
            set_bit(&nc->btn, BTN_DOWN);
        }

        if (SDLK_LEFT == nc->event->key.key) {
            set_bit(&nc->btn, BTN_LEFT);
        }

        if (SDLK_RIGHT == nc->event->key.key) {
            set_bit(&nc->btn, BTN_RIGHT);
        }
        break;
    }
    case SDL_EVENT_KEY_UP:
    {
        if (nc->event->key.repeat) { // No key repeat.
            break;
        }

        if (SDLK_BACKSPACE == nc->event->key.key) {
            clear_bit(&nc->btn, BTN_BACKSPACE);
        }

        if (SDLK_1 == nc->event->key.key) {
            clear_bit(&nc->btn, BTN_1);
        }

        if (SDLK_2 == nc->event->key.key) {
            clear_bit(&nc->btn, BTN_2);
        }

        if (SDLK_3 == nc->event->key.key) {
            clear_bit(&nc->btn, BTN_3);
        }

        if (SDLK_4 == nc->event->key.key) {
            clear_bit(&nc->btn, BTN_4);
        }

        if (SDLK_5 == nc->event->key.key) {
            clear_bit(&nc->btn, BTN_5);
        }

        if (SDLK_6 == nc->event->key.key) {
            clear_bit(&nc->btn, BTN_6);
        }

        if (SDLK_7 == nc->event->key.key) {
            clear_bit(&nc->btn, BTN_7);
        }

        if (SDLK_8 == nc->event->key.key) {
            clear_bit(&nc->btn, BTN_8);
        }

        if (SDLK_9 == nc->event->key.key) {
            clear_bit(&nc->btn, BTN_9);
        }

        if (SDLK_0 == nc->event->key.key) {
            clear_bit(&nc->btn, BTN_0);
        }

        if (SDLK_ASTERISK == nc->event->key.key) {
            clear_bit(&nc->btn, BTN_ASTERISK);
        }

        if (SDLK_HASH == nc->event->key.key) {
            clear_bit(&nc->btn, BTN_HASH);
        }

        if (SDLK_SOFTLEFT == nc->event->key.key) {
            clear_bit(&nc->btn, BTN_SOFTLEFT);
        }

        if (SDLK_SOFTRIGHT == nc->event->key.key) {
            clear_bit(&nc->btn, BTN_SOFTRIGHT);
        }

        if (SDLK_SELECT == nc->event->key.key) {
            clear_bit(&nc->btn, BTN_SELECT);
        }

        if (SDLK_UP == nc->event->key.key) {
            clear_bit(&nc->btn, BTN_UP);
        }

        if (SDLK_DOWN == nc->event->key.key) {
            clear_bit(&nc->btn, BTN_DOWN);
        }

        if (SDLK_LEFT == nc->event->key.key) {
            clear_bit(&nc->btn, BTN_LEFT);
        }

        if (SDLK_RIGHT == nc->event->key.key) {
            clear_bit(&nc->btn, BTN_RIGHT);
        }
        break;
    }
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
    {
        const SDL_JoystickID which = nc->event->gbutton.which;
        // SDL_Log("Gamepad #%" SDL_PRIu32 " button %s -> %s", which, SDL_GetGamepadStringForButton(nc->event->gbutton.button), nc->event->gbutton.down ? "PRESSED" : "RELEASED");
        break;
    }
    }

    if (check_bit(nc->btn, BTN_SOFTLEFT)) {
        return false;
    }

    return true;
}

void destroy_ncore(ncore_t *nc)
{
    if (nc) {
        if (nc->hero) {
            destroy_hero(nc->hero);
            nc->hero = NULL;
        }

        if (nc->map) {
            destroy_map(nc->map);
            nc->map = NULL;
        }

#ifndef __SYMBIAN32__
        if (nc->frame) {
            SDL_DestroyTexture(nc->frame);
            nc->frame = NULL;
        }
#endif

        if (nc->renderer) {
            SDL_DestroyRenderer(nc->renderer);
            nc->renderer = NULL;
        }

        if (nc->window) {
            SDL_DestroyWindow(nc->window);
            nc->window = NULL;
        }

        SDL_free(nc);
    }

    destroy_app();
}
