/** @file main.c
 *
 *  A cross-platform engine with native Nokia N-Gage compatibility.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#define SDL_MAIN_USE_CALLBACKS 1

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "ncore.h"

ncore_t *core = NULL;

// This function runs once at startup.
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    if (!init_ncore(&core))
    {
        SDL_Log("Failed to initialize ncore.");
        return SDL_APP_FAILURE;
    }

    return SDL_APP_CONTINUE;
}

// This function runs when a new event (Keypresses, etc) occurs.
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    if (!core || !event)
    {
        return SDL_APP_CONTINUE;
    }
    else
    {
        core->event = event;
    }

    if (!handle_ncore_events(core))
    {
        return SDL_APP_SUCCESS;
    }

    return SDL_APP_CONTINUE;
}

// This function runs once per frame, and is the heart of the program.
SDL_AppResult SDL_AppIterate(void *appstate)
{
    if (!update_ncore(core))
    {
        return SDL_APP_SUCCESS;
    }
    if (!draw_ncore_scene(core))
    {
        SDL_Log("Failed to draw ncore scene");
        return SDL_APP_SUCCESS;
    }
    return SDL_APP_CONTINUE;
}

// This function runs once at shutdown.
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    destroy_ncore(core);
    //  SDL will clean up the window/renderer for us.
}
