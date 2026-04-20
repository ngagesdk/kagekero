/** @file core.c
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
#include "cheats.h"
#include "config.h"
#include "core.h"
#include "kero.h"
#include "map.h"
#include "overclock.h"
#include "overlay.h"
#include "pfs.h"
#include "utils.h"

static const char *pride_lines[PRIDE_LINE_COUNT] = {
    "This frog's pro-  nouns? Rib/bit.   Deal with it.",
    "Ribbit! Looks likeKero's hopping outand proud!",
    "Who knew cheats   could be this     queer? Kero did.  Kero always knew.",
    "One small hop for a frog, one giant leap for frogkind",
    "You thought Kero  was just a frog?  Surprise - they'rea queer icon."
};

bool init(core_t **nc)
{
    *nc = (core_t *)SDL_calloc(1, sizeof(core_t));
    if (!*nc)
    {
        SDL_Log("Failed to allocate memory for engine core");
        return false;
    }

    if (!init_app((SDL_Renderer **)&(*nc)->renderer, (*nc)->window))
    {
        return SDL_APP_FAILURE;
    }
    (*nc)->state = STATE_INTRO;

#if !defined __SYMBIAN32__ && !defined DEBUG
    SDL_DisplayID display_id = SDL_GetPrimaryDisplay();
    if (!display_id)
    {
        SDL_Log("Couldn't get primary display: %s", SDL_GetError());
        return false;
    }

    SDL_Rect display_bounds;
    if (!SDL_GetDisplayBounds(display_id, &display_bounds))
    {
        SDL_Log("Couldn't get display bounds: %s", SDL_GetError());
        return false;
    }

    int max_scale = SDL_min(display_bounds.w / WINDOW_W, display_bounds.h / WINDOW_H);
    if (max_scale < 1)
    {
        max_scale = 1;
    }

    (*nc)->frame_offset_x = ((display_bounds.w - FRAME_WIDTH * max_scale) / 2) / max_scale;
    (*nc)->frame_offset_y = ((display_bounds.h - FRAME_HEIGHT * max_scale) / 2) / max_scale;
    (*nc)->screen_offset_x = ((display_bounds.w - SCREEN_W * max_scale) / 2) / max_scale;
    (*nc)->screen_offset_y = ((display_bounds.h - SCREEN_H * max_scale) / 2) / max_scale;
#endif

    init_file_reader();

    char first_map[11] = { 0 };
    SDL_snprintf(first_map, 11, "%03d.%s", FIRST_LEVEL, MAP_SUFFIX);
    if (!load_map(first_map, &(*nc)->map, (*nc)->renderer))
    {
        return false;
    }

    if (!load_kero(&(*nc)->kero, (*nc)->map, (*nc)->renderer))
    {
        SDL_Log("Failed to load kero");
        return false;
    }

    if (!load_overlay((*nc)->map, &(*nc)->ui, (*nc)->renderer))
    {
        SDL_Log("Failed to load overlay");
        return false;
    }

    if (!render_map((*nc)->map, (*nc)->renderer, &(*nc)->has_updated, 0, 0))
    {
        SDL_Log("Failed to render map");
        return false;
    }

#if !defined __SYMBIAN32__
    if (!load_texture_from_file(FRAME_IMAGE, &(*nc)->frame, (*nc)->renderer))
    {
        return false;
    }

    (*nc)->backbuffer = SDL_CreateTexture((*nc)->renderer, SDL_PIXELFORMAT_XRGB4444, SDL_TEXTUREACCESS_TARGET, SCREEN_W, SCREEN_H);
    if (!(*nc)->backbuffer)
    {
        SDL_Log("Failed to create backbuffer: %s", SDL_GetError());
        return false;
    }
    SDL_SetTextureScaleMode((*nc)->backbuffer, SDL_SCALEMODE_NEAREST);
#endif

    return true;
}

bool update(core_t *nc)
{
    update_kero(nc->kero, nc->map, nc->ui, &nc->btn, nc->renderer, nc->is_paused, &nc->has_updated);

    nc->cam_x = (int)nc->kero->pos_x - (SCREEN_W / 2);
    nc->cam_y = (int)nc->kero->pos_y - (SCREEN_H / 2);

    render_map(nc->map, nc->renderer, &nc->has_updated, nc->cam_x, nc->cam_y);

    if ((nc->kero->prev_life_count != nc->kero->life_count) ||
        (nc->map->prev_coins != nc->map->coins_left) ||
        (nc->is_paused && nc->ui->menu_selection != nc->ui->prev_selection))
    {
        render_overlay(nc->map->coins_left, nc->map->coin_max, nc->kero->life_count, nc->map, nc->ui, nc->renderer);
    }

#if defined __3DS__
    SDL_RenderTexture(nc->renderer, nc->frame, NULL, NULL);
#elif defined __DREAMCAST__
    SDL_FRect dst;
    dst.w = FRAME_WIDTH;
    dst.h = FRAME_HEIGHT;
    dst.x = FRAME_OFFSET_X;
    dst.y = FRAME_OFFSET_Y;

    SDL_RenderTexture(nc->renderer, nc->frame, NULL, &dst);
#elif !defined __SYMBIAN32__
    SDL_FRect src;
    src.w = FRAME_WIDTH;
    src.h = FRAME_HEIGHT;
    src.x = 0.f;
    src.y = 0.f;

    SDL_FRect dst;
    dst.w = FRAME_WIDTH;
    dst.h = FRAME_HEIGHT;
    dst.x = (float)nc->frame_offset_x;
    dst.y = (float)nc->frame_offset_y;

    SDL_RenderTexture(nc->renderer, nc->frame, &src, &dst);
#endif

    return true;
}

bool draw_scene(core_t *nc)
{
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
    SDL_FRect dst;

#if defined __SYMBIAN32__
    if (!nc->has_updated)
#endif
    {
        // Composite everything into the backbuffer.
#if defined __SYMBIAN32__
        SDL_SetRenderTarget(nc->renderer, NULL);
#else
        SDL_SetRenderTarget(nc->renderer, nc->backbuffer);
#endif

        // Draw visible slice of the map.
        src.x = (float)nc->cam_x;
        src.y = (float)nc->cam_y;
        src.w = SCREEN_W;
        src.h = SCREEN_H;
        dst.x = 0.f;
        dst.y = 0.f;
        dst.w = SCREEN_W;
        dst.h = SCREEN_H;
        SDL_RenderTexture(nc->renderer, nc->map->render_target, &src, &dst);

        // Draw kero in screen space.
        render_kero(nc->kero, nc->renderer, nc->cam_x, nc->cam_y);

        // HUD: coin counter.
        dst.x = 0.f;
        dst.y = 4.f;
        dst.w = 55.f;
        dst.h = 16.f;
        SDL_RenderTexture(nc->renderer, nc->ui->coin_count_canvas, NULL, &dst);

        // HUD: life counter.
        dst.x = 139.f;
        dst.y = 4.f;
        dst.w = 37.f;
        dst.h = 16.f;
        SDL_RenderTexture(nc->renderer, nc->ui->life_count_canvas, NULL, &dst);

        if (nc->is_paused)
        {
            dst.x = 40.f;
            dst.y = 80.f;
            dst.w = 96.f;
            dst.h = 48.f;
            SDL_RenderTexture(nc->renderer, nc->ui->menu_canvas, NULL, &dst);
        }

        if (nc->map->show_dialogue)
        {
            dst.x = 0.f;
            dst.y = 136.f;
            dst.w = 176.f;
            dst.h = 72.f; // Why 136? Shouldn't this be 104?
            SDL_RenderTexture(nc->renderer, nc->ui->dialogue_canvas, NULL, &dst);
        }

#if !defined __SYMBIAN32__
        // Present backbuffer to screen.
        SDL_SetRenderTarget(nc->renderer, NULL);

        int screen_offset_x;
        int screen_offset_y;
#if defined DEBUG
        screen_offset_x = SCREEN_OFFSET_X;
        screen_offset_y = SCREEN_OFFSET_Y;
#else
        screen_offset_x = nc->screen_offset_x;
        screen_offset_y = nc->screen_offset_y;
#endif
        dst.x = (float)screen_offset_x;
        dst.y = (float)screen_offset_y;
        dst.w = SCREEN_W;
        dst.h = SCREEN_H;

        if (!SDL_RenderTexture(nc->renderer, nc->backbuffer, NULL, &dst))
        {
            SDL_Log("Error rendering backbuffer: %s", SDL_GetError());
            return false;
        }
#endif
    }

    SDL_RenderPresent(nc->renderer);

    return true;
}

static bool handle_button_down(core_t *nc, button_t button)
{
    if (nc->is_paused)
    {
        add_to_ring_buffer(button);
#ifdef __SYMBIAN32__
        int sequence_length = 5;
        const button_t cheat_sequence[5] = { BTN_5, BTN_4, BTN_2, BTN_8, BTN_7 };
#else
        int sequence_length = 10;
        const button_t cheat_sequence[10] = { BTN_UP, BTN_UP, BTN_DOWN, BTN_DOWN, BTN_LEFT, BTN_RIGHT, BTN_LEFT, BTN_RIGHT, BTN_5, BTN_7 };
#endif
        if (find_sequence(cheat_sequence, sequence_length))
        {
            nc->kero->wears_mask = true;
            nc->map->use_lgbtq_flag = true;
            nc->map->show_dialogue = true;
            nc->map->keep_dialogue = true;
            nc->is_paused = false;

            static int pride_line_index = 0;
            render_text(pride_lines[pride_line_index], nc->kero->wears_mask, nc->map, nc->ui, nc->renderer);
            pride_line_index++;
            if (pride_line_index > PRIDE_LINE_COUNT - 1)
            {
                pride_line_index = 0;
            }

            clear_ring_buffer();
        }
    }
    else
    {
        clear_ring_buffer();
    }

    if ((check_bit(nc->btn, BTN_SOFTRIGHT) || check_bit(nc->btn, BTN_SOFTLEFT)) && !nc->is_paused && !nc->map->show_dialogue)
    {
        nc->is_paused = true;
        nc->ui->prev_selection = nc->ui->menu_selection;
        nc->ui->menu_selection = MENU_RESUME;
    }
    else if (nc->map->show_dialogue)
    {
        if (check_bit(nc->btn, BTN_5) || check_bit(nc->btn, BTN_7) || check_bit(nc->btn, BTN_SELECT))
        {
            if (!nc->map->keep_dialogue)
            {
                nc->map->show_dialogue = false;
            }
            else
            {
                nc->map->keep_dialogue = false;
            }
        }
    }
    else if (nc->is_paused)
    {
        if (check_bit(nc->btn, BTN_7) || check_bit(nc->btn, BTN_SELECT))
        {
            switch (nc->ui->menu_selection)
            {
                default:
                case MENU_NONE:
                    break;
                case MENU_RESUME:
                    nc->is_paused = false;
                    break;
                case MENU_SETTINGS:
                    nc->ui->prev_selection = nc->ui->menu_selection;
                    nc->ui->menu_selection = MENU_MHZ;
                    nc->ui->is_settings_menu = true;
                    break;
                case MENU_QUIT:
                    return false;
                case MENU_MHZ:
                    {
                        if (is_overclock_enabled())
                        {
                            disable_overclock();
                        }
                        else
                        {
                            enable_overclock();
                        }
                        break;
                    }
                case MENU_BACK:
                    nc->ui->prev_selection = nc->ui->menu_selection;
                    nc->ui->menu_selection = MENU_SETTINGS;
                    nc->ui->is_settings_menu = false;
                    break;
            }
        }
        else if (check_bit(nc->btn, BTN_UP))
        {
            nc->ui->prev_selection = nc->ui->menu_selection;
            nc->ui->menu_selection -= 1;

            if (!nc->ui->is_settings_menu)
            {
                if (nc->ui->menu_selection < MENU_RESUME)
                {
                    nc->ui->menu_selection = MENU_QUIT;
                }
            }
            else
            {
                if (nc->ui->menu_selection < MENU_MHZ)
                {
                    nc->ui->menu_selection = MENU_BACK;
                }
            }
        }
        else if (check_bit(nc->btn, BTN_DOWN))
        {
            nc->ui->prev_selection = nc->ui->menu_selection;
            nc->ui->menu_selection += 1;
            if (!nc->ui->is_settings_menu)
            {
                if (nc->ui->menu_selection > MENU_QUIT)
                {
                    nc->ui->menu_selection = MENU_RESUME;
                }
            }
            else
            {
                if (nc->ui->menu_selection > MENU_BACK)
                {
                    nc->ui->menu_selection = MENU_MHZ;
                }
            }
        }
    }

    return true;
}

bool handle_events(core_t *nc)
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
        case SDL_EVENT_GAMEPAD_AXIS_MOTION:
            {
                const SDL_JoystickID which = nc->event->gdevice.which;
                SDL_Gamepad *gamepad = SDL_OpenGamepad(which);

                Sint16 x_axis = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTX);
                Sint16 y_axis = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTY);

                const Sint16 threshold = 8000; // Threshold for axis motion to be considered significant.

                if (x_axis <= -threshold)
                {
                    set_bit(&nc->btn, BTN_LEFT);
                    clear_bit(&nc->btn, BTN_RIGHT);
                }
                else if (x_axis >= threshold)
                {
                    clear_bit(&nc->btn, BTN_LEFT);
                    set_bit(&nc->btn, BTN_RIGHT);
                }
                else
                {
                    clear_bit(&nc->btn, BTN_LEFT);
                    clear_bit(&nc->btn, BTN_RIGHT);
                }
            }
        case SDL_EVENT_KEY_DOWN:
            {
                if (nc->event->key.repeat)
                { // No key repeat.
                    break;
                }

                button_t button = get_button_from_key(nc->event->key.key);
                set_bit(&nc->btn, button);
                return handle_button_down(nc, button);
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
                return handle_button_down(nc, button);
            }
        case SDL_EVENT_GAMEPAD_BUTTON_UP:
            {
                const SDL_JoystickID which = nc->event->gbutton.which;
                button_t button = get_button_from_gamepad(nc->event->gbutton.button);
                clear_bit(&nc->btn, button);

                break;
            }
    }

    return true;
}

void destroy(core_t *nc)
{
    disable_overclock();

    if (nc)
    {
        if (nc->ui)
        {
            destroy_overlay(nc->ui);
            nc->ui = NULL;
        }

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
        if (nc->backbuffer)
        {
            SDL_DestroyTexture(nc->backbuffer);
            nc->backbuffer = NULL;
        }

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
