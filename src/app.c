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

    window = SDL_CreateWindow("ncore", WINDOW_W * SCALE, WINDOW_H * SCALE, WINDOW_FLAGS);
    if (!window)
    {
        SDL_Log("Couldn't create window: %s", SDL_GetError());
        return false;
    }

    *renderer = SDL_CreateRenderer(window, 0);
    if (!*renderer)
    {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return false;
    }

    if (!SDL_SetRenderScale(*renderer, SCALE, SCALE))
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
