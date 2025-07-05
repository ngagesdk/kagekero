/** @file app.c
 *
 *  A minimalist, cross-platform puzzle-platformer, designed
 *  especially for the Nokia N-Gage.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#include "SDL3/SDL.h"

#include "app.h"
#include "config.h"
#include "pfs.h"

static SDL_AudioDeviceID audio_device;

bool init_app(SDL_Renderer **renderer, SDL_Window *window)
{
#ifndef __SYMBIAN32__
    SDL_SetHint("SDL_RENDER_VSYNC", "1");
#endif
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_INFO);
    SDL_SetAppMetadata("kagekero", "1.0", "de.ngagesdk.kagekero");

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
    {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return false;
    }

    if (!SDL_InitSubSystem(SDL_INIT_GAMEPAD))
    {
        SDL_Log("Couldn't initialize gamepad subsystem: %s", SDL_GetError());
    }

#if !defined __SYMBIAN32__
    SDL_DisplayID display_id = SDL_GetPrimaryDisplay();
    if (!display_id)
    {
        SDL_Log("Couldn't get primary display: %s", SDL_GetError());
        return false;
    }
#endif

#if defined __3DS__ || defined __DREAMCAST__
    int window_w = WINDOW_W;
    int window_h = WINDOW_H;

#elif !defined __SYMBIAN32__ && !defined DEBUG
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

    int window_w = display_bounds.w;
    int window_h = display_bounds.h;
    if (!SDL_HideCursor())
    {
        SDL_Log("Couldn't hide cursor: %s", SDL_GetError());
    }

#else
    int window_w = WINDOW_W * SCALE;
    int window_h = WINDOW_H * SCALE;
    int max_scale = SCALE;
#endif

    window = SDL_CreateWindow("\xE5\xBD\xB1\xE3\x82\xB1\xE3\x83\xAD", window_w, window_h, WINDOW_FLAGS);
    if (!window)
    {
        SDL_Log("Couldn't create window: %s", SDL_GetError());
        return false;
    }

#if defined __3DS__ || defined __DREAMCAST__
#elif !defined __SYMBIAN32__
    SDL_WINDOWPOS_CENTERED_DISPLAY(display_id);
#endif

    *renderer = SDL_CreateRenderer(window, 0);
    if (!*renderer)
    {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return false;
    }

#if !defined __3DS__ && !defined __DREAMCAST__
    if (!SDL_SetRenderScale(*renderer, (float)max_scale, (float)max_scale))
    {
        SDL_Log("Could not apply drawing scale factor: %s", SDL_GetError());
        return false;
    }
#endif

    if (!SDL_DisableScreenSaver())
    {
        SDL_Log("Couldn't disable screen saver: %s", SDL_GetError());
    }

    SDL_AudioSpec spec;
    spec.channels = 1;
    spec.format = SDL_AUDIO_S16;
    spec.freq = 8000;

    audio_device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec);
    if (audio_device == 0)
    {
        SDL_Log("SDL_OpenAudioDevice: %s", SDL_GetError());
        return false;
    }

    return true;
}

void destroy_app(void)
{
    SDL_CloseAudioDevice(audio_device);
}
