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
#include "overlay.h"
#include "pfs.h"
#include "utils.h"

static const char *death_lines[DEATH_LINE_COUNT] = {
    "Ribbit. Guess I   croaked for real  that time.",
    "Note to self:     spikes hurt more  than they look.",
    "Good thing I'm    not on an N-Gage  - I'd need a new  battery by now.",
    "One small hop for frog, one giant   leap into fail-   ure.",
    "Put that one on   my highlight reel - the blooper     edition.",
    "If Madeline can   do it a thousand  times, so can I.  Ribbit.",
    "I'd say 'call for help,' but my     N-Gage has no     signal.",
    "Death count: too  high. Pride: stillintact.",
    "Respawn faster    than an N-Gage    Arena match       disconnects.",
    "Pro tip: Don't do what I just did.",
    "At least when I   dash into spikes, I don't have to   listen to a moti- vational speech first.",
    "Guess I just      Madelined myself  into the spikes   again. Classic.",
    "Climbing my way   to the afterlife  - one dumb jump   ata time.",
    "Next time I'll    bring a moti-     vational sound-   track like        Madeline. Might help.",
    "If Madeline can   face her demons, Ican face...       whatever just     impaled me.",
    "Maybe I should've stuck to straw-   berries instead   of pain.",
    "Bad jump. Worse   landing.          10/10 Celeste tri-bute though.",
    "Hey Madeline! Saveme a spot on the  death counter!",
    "I'd call for help,but my inner      demon's on        vacation.",
    "Frog fact: unlike mountains, spikes always win.",
    "Like a Nokia brick- unbreakable? Nottoday.",
    "Should've brought my Celeste        climbing gloves.",
    "Better luck next  leap, Frogger 2003edition.",
    "This is where I   leapt... and this is where I        flopped.",
    "This is where I   sticked the land- ing -just kidding.",
    "This was where I  ribbited. This waswhere I regretted it.",
    "This was where I  went full ninja.  And full pancake.",
    "This was where I  thought Frogger   physics still     applied."
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
    SDL_snprintf(first_map, 11, "%03d.tmj%s", FIRST_LEVEL, MAP_PREFIX);
    if (!load_map(first_map, &(*nc)->map, (*nc)->renderer))
    {
        return false;
    }

    if (!load_kero(&(*nc)->kero, (*nc)->map))
    {
        SDL_Log("Failed to load kero");
        return false;
    }

    if (!load_overlay(&(*nc)->ui))
    {
        SDL_Log("Failed to load overlay");
        return false;
    }

    if (!render_map((*nc)->map, (*nc)->renderer, &(*nc)->has_updated))
    {
        SDL_Log("Failed to render map");
        return false;
    }

#if !defined __SYMBIAN32__
    if (!load_texture_from_file(FRAME_IMAGE, &(*nc)->frame, (*nc)->renderer))
    {
        return false;
    }
#endif
    SDL_srand(0);

    return true;
}

bool update(core_t *nc)
{
    update_kero(nc->kero, nc->map, &nc->btn, nc->renderer, nc->is_paused, &nc->has_updated);

    nc->cam_x = (int)nc->kero->pos_x - (SCREEN_W / 2);
    nc->cam_y = (int)nc->kero->pos_y - (SCREEN_H / 2);

    render_map(nc->map, nc->renderer, &nc->has_updated);
    render_kero(nc->kero, nc->map);

    if ((nc->kero->prev_life_count != nc->kero->life_count) ||
        (nc->map->prev_coins != nc->map->coins_left) ||
        (nc->ui->menu_selection != MENU_NONE && nc->has_updated))
    {
        render_overlay(nc->map->coins_left, nc->map->coin_max, nc->kero->life_count, nc->ui);
    }

    if (nc->kero->state == STATE_DEAD)
    {
        nc->map->show_dialogue = true;
        render_text(death_lines[nc->kero->line_index], nc->kero->wears_mask, nc->ui);
    }
    else
    {
        nc->map->show_dialogue = false;
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
    SDL_Rect visible_area;

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
        visible_area.x = nc->cam_x;
        visible_area.y = nc->cam_y;
        visible_area.w = SCREEN_W;
        visible_area.h = SCREEN_H;

        SDL_Surface *temp;
        if (!SDL_LockTextureToSurface(nc->map->render_target, NULL, &temp))
        {
            SDL_Log("Error locking animated tile texture: %s", SDL_GetError());
            return false;
        }
        SDL_BlitSurface(nc->map->render_canvas, &visible_area, temp, &visible_area);

        SDL_Rect dst_rect;
        dst_rect.x = (int)nc->kero->pos_x - KERO_HALF;
        dst_rect.y = (int)nc->kero->pos_y - KERO_HALF;
        dst_rect.w = KERO_SIZE;
        dst_rect.h = KERO_SIZE;

        SDL_BlitSurface(nc->kero->render_canvas, NULL, temp, &dst_rect);

        dst_rect.x = 0 + nc->cam_x;
        dst_rect.y = 4 + nc->cam_y;
        dst_rect.w = 57;
        dst_rect.h = 16;
        SDL_BlitSurface(nc->ui->coin_count_canvas, NULL, temp, &dst_rect);

        dst_rect.x = 139 + nc->cam_x;
        dst_rect.y = 4 + nc->cam_y;
        dst_rect.w = 37;
        dst_rect.h = 16;
        SDL_BlitSurface(nc->ui->life_count_canvas, NULL, temp, &dst_rect);

        if (nc->is_paused)
        {
            dst_rect.x = 40 + nc->cam_x;
            dst_rect.y = 80 + nc->cam_y;
            dst_rect.w = 96;
            dst_rect.h = 48;

            SDL_BlitSurface(nc->ui->menu_canvas, NULL, temp, &dst_rect);
        }

        if (nc->map->show_dialogue)
        {
            dst_rect.x = 0 + nc->cam_x;
            dst_rect.y = 136 + nc->cam_y; // Why 136? Shouldn't this be 104?.
            dst_rect.w = 176;
            dst_rect.h = 72;

            SDL_BlitSurface(nc->ui->dialogue_canvas, NULL, temp, &dst_rect);
        }

        SDL_UnlockTexture(nc->map->render_target);

        src.x = (float)(0 + nc->cam_x);
        src.y = (float)(0 + nc->cam_y);
        src.w = SCREEN_W;
        src.h = SCREEN_H;

        int screen_offset_x;
        int screen_offset_y;

#if defined DEBUG
        screen_offset_x = SCREEN_OFFSET_X;
        screen_offset_y = SCREEN_OFFSET_Y;
#elif !defined __SYMBIAN32__
        screen_offset_x = nc->screen_offset_x;
        screen_offset_y = nc->screen_offset_y;
#else
        screen_offset_x = SCREEN_OFFSET_X;
        screen_offset_y = SCREEN_OFFSET_Y;
#endif

        dst.x = (float)screen_offset_x;
        dst.y = (float)screen_offset_y;
        dst.w = SCREEN_W;
        dst.h = SCREEN_H;

        if (!SDL_RenderTexture(nc->renderer, nc->map->render_target, &src, &dst))
        {
            SDL_Log("Error rendering texture: %s", SDL_GetError());
            return false;
        }
    }

    SDL_RenderPresent(nc->renderer);

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
        case SDL_EVENT_KEY_DOWN:
            {
                if (nc->event->key.repeat)
                { // No key repeat.
                    break;
                }

                button_t button = get_button_from_key(nc->event->key.key);
                set_bit(&nc->btn, button);

                if (nc->is_paused)
                {
                    add_to_ring_buffer(button);
                    const button_t cheat_sequence[5] = { BTN_5, BTN_4, BTN_2, BTN_8, BTN_7 };
                    if (find_sequence(cheat_sequence, 5))
                    {
                        SDL_Log("Cheat code activated: 5-4-2-8-7"); // L G B T Q
                        nc->kero->wears_mask = true;
                        nc->map->use_lgbtq_flag = true;
                        clear_ring_buffer();
                    }
                }
                else
                {
                    clear_ring_buffer();
                }

                if ((check_bit(nc->btn, BTN_SOFTRIGHT) || check_bit(nc->btn, BTN_SOFTLEFT)) && !nc->is_paused)
                {
                    nc->is_paused = true;
                    nc->ui->menu_selection = MENU_RESUME;
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
                                // Tbd.
                                break;
                            case MENU_QUIT:
                                return false;
                        }
                    }
                    else if (check_bit(nc->btn, BTN_UP))
                    {
                        nc->ui->menu_selection -= 1;
                        if (nc->ui->menu_selection < MENU_RESUME)
                        {
                            nc->ui->menu_selection = MENU_QUIT;
                        }
                    }
                    else if (check_bit(nc->btn, BTN_DOWN))
                    {
                        nc->ui->menu_selection += 1;
                        if (nc->ui->menu_selection > MENU_QUIT)
                        {
                            nc->ui->menu_selection = MENU_RESUME;
                        }
                    }
                }

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

    return true;
}

void destroy(core_t *nc)
{
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
