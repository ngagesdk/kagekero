/** @file game.c
 *
 *  A minimalist, cross-platform puzzle-platformer, designed
 *  especially for the Nokia N-Gage.
 *
 *  Copyright (c) 2026, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#include <SDL3/SDL.h>

#include "cheats.h"
#include "config.h"
#include "core.h"
#include "game.h"
#include "kero.h"
#include "map.h"
#include "overclock.h"
#include "overlay.h"
#include "utils.h"

static const char *pride_lines[PRIDE_LINE_COUNT] = {
    "This frog's pro-  nouns? Rib/bit.   Deal with it.",
    "Ribbit! Looks likeKero's hopping outand proud!",
    "Who knew cheats   could be this     queer? Kero did.  Kero always knew.",
    "One small hop for a frog, one giant leap for frogkind",
    "You thought Kero  was just a frog?  Surprise - they'rea queer icon."
};

bool load_game(core_t *nc)
{
    char first_map[11] = { 0 };
    SDL_snprintf(first_map, 11, "%03d.%s", FIRST_LEVEL, MAP_SUFFIX);
    if (!load_map(first_map, &nc->map, nc->renderer))
    {
        return false;
    }

    if (!load_kero(&nc->kero, nc->map, nc->renderer))
    {
        SDL_Log("Failed to load kero");
        return false;
    }

    if (!load_overlay(nc->map, &nc->ui, nc->renderer))
    {
        SDL_Log("Failed to load overlay");
        return false;
    }

    if (!render_map(nc->map, nc->renderer, &nc->has_updated))
    {
        SDL_Log("Failed to render map");
        return false;
    }

    return true;
}

bool update_game(core_t *nc)
{
    update_kero(nc->kero, nc->map, nc->ui, &nc->btn, nc->renderer, nc->is_paused, &nc->has_updated);

    nc->cam_x = (int)nc->kero->pos_x - (SCREEN_W / 2);
    nc->cam_y = (int)nc->kero->pos_y - (SCREEN_H / 2);

    render_map(nc->map, nc->renderer, &nc->has_updated);

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

bool handle_game_button_down(core_t *nc, button_t button)
{
    if (nc->is_paused)
    {
        add_to_ring_buffer(button);
#ifdef __SYMBIAN32__
        int sequence_length = 5;
        static const button_t cheat_sequence[5] = { BTN_5, BTN_4, BTN_2, BTN_8, BTN_7 };
#else
        // int sequence_length = 10;
        int sequence_length = 3;
        static const button_t cheat_sequence[10] = { BTN_LEFT, BTN_LEFT, BTN_LEFT };
        // static const button_t cheat_sequence[10] = { BTN_UP, BTN_UP, BTN_DOWN, BTN_DOWN, BTN_LEFT, BTN_RIGHT, BTN_LEFT, BTN_RIGHT, BTN_5, BTN_7 };
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
            return true;
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
        clear_ring_buffer();
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

void unload_game(core_t *nc)
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
}
