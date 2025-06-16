/** @file app.c
 *
 *  A cross-platform engine with native Nokia N-Gage compatibility.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#include "app.h"
#include "SDL3/SDL.h"
#include "config.h"
#include "pfs.h"

static SDL_AudioDeviceID audio_device;

bool init_app(SDL_Renderer **renderer, SDL_Window *window)
{
#ifdef WIN32
    SDL_SetHint("SDL_RENDER_VSYNC", "1");
#endif
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_INFO);
    SDL_SetAppMetadata("ncore", "1.0", "com.ncore.ngagesdk");

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
    {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return false;
    }

    if (!SDL_InitSubSystem(SDL_INIT_GAMEPAD))
    {
        SDL_Log("Couldn't initialize gamepad subsystem: %s", SDL_GetError());
    }

#ifdef WIN32
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

    int window_w = WINDOW_W * max_scale;
    int window_h = WINDOW_H * max_scale;

    int window_x = display_bounds.x + (display_bounds.w - window_w) / 2;
    int window_y = display_bounds.y + (display_bounds.h - window_h) / 2;
#else
    int window_w = WINDOW_W * SCALE;
    int window_h = WINDOW_H * SCALE;
    int max_scale = SCALE;
#endif

    window = SDL_CreateWindow("ncore", window_w, window_h, WINDOW_FLAGS);
    if (!window)
    {
        SDL_Log("Couldn't create window: %s", SDL_GetError());
        return false;
    }

#if WIN32
    SDL_WINDOWPOS_CENTERED_DISPLAY(display_id);
    // SDL_SetWindowFullscreen(window, true);
#endif

    *renderer = SDL_CreateRenderer(window, 0);
    if (!*renderer)
    {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return false;
    }

    if (!SDL_SetRenderScale(*renderer, max_scale, max_scale))
    {
        SDL_Log("Could not apply drawing scale factor: %s", SDL_GetError());
        return false;
    }

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
