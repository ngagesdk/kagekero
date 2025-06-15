/** @file utils.c
 *
 *  A cross-platform engine with native Nokia N-Gage compatibility.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#include <SDL3/SDL.h>

#include "pfs.h"
#include "utils.h"

#define STBI_ONLY_PNG
#define STBI_NO_THREAD_LOCALS
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

bool load_surface_from_file(const char *file_name, SDL_Surface **surface)
{
    Uint8 *buffer;
    SDL_IOStream *asset;
    size_t file_size;

    if (!file_name)
    {
        return true;
    }
    SDL_Log("Loading texture from file: %s", file_name);

    buffer = (Uint8 *)load_binary_file_from_path(file_name);
    if (!buffer)
    {
        SDL_Log("Failed to load asset: %s", file_name);
        return false;
    }

    file_size = size_of_file(file_name);
    asset = SDL_IOFromConstMem((Uint8 *)buffer, file_size);
    if (!asset)
    {
        SDL_free(buffer);
        SDL_Log("Failed to convert asset %s: %s", file_name, SDL_GetError());
        return false;
    }

    int width, height, bpp;
    stbi_uc *pixels = stbi_load_from_memory(buffer, file_size, &width, &height, &bpp, 4);
    if (!pixels)
    {
        SDL_Log("Couldn't load image data: %s", stbi_failure_reason());
        SDL_free(buffer);
        return false;
    }
    SDL_free(buffer);

    *surface = SDL_CreateSurfaceFrom(width, height, SDL_PIXELFORMAT_RGBA32, (void *)pixels, width * 4);
    if (!*surface)
    {
        SDL_free(buffer);
        SDL_Log("Failed to load image: %s", SDL_GetError());
        return false;
    }

    const SDL_PixelFormatDetails *format_details = SDL_GetPixelFormatDetails((*surface)->format);
    if (!SDL_SetSurfaceColorKey(*surface, true, SDL_MapRGB(format_details, NULL, 0xff, 0x00, 0xff)))
    {
        SDL_Log("Couldn't set surface color key: %s", SDL_GetError());
        SDL_DestroySurface(*surface);
        return false;
    }

    return true;
}

bool load_texture_from_file(const char *file_name, SDL_Texture **texture, SDL_Renderer *renderer)
{
    Uint8 *buffer;
    SDL_IOStream *asset;
    SDL_Surface *surface;
    size_t file_size;

    if (!file_name)
    {
        return true;
    }
    SDL_Log("Loading texture from file: %s", file_name);

    buffer = (Uint8 *)load_binary_file_from_path(file_name);
    if (!buffer)
    {
        SDL_Log("Failed to load asset: %s", file_name);
        return false;
    }

    file_size = size_of_file(file_name);
    asset = SDL_IOFromConstMem((Uint8 *)buffer, file_size);
    if (!asset)
    {
        SDL_free(buffer);
        SDL_Log("Failed to convert asset %s: %s", file_name, SDL_GetError());
        return false;
    }

    int width, height, bpp;
    stbi_uc *pixels = stbi_load_from_memory(buffer, file_size, &width, &height, &bpp, 4);
    if (!pixels)
    {
        SDL_Log("Couldn't load image data: %s", stbi_failure_reason());
        SDL_free(buffer);
        return false;
    }
    SDL_free(buffer);

    surface = SDL_CreateSurfaceFrom(width, height, SDL_PIXELFORMAT_RGBA32, (void *)pixels, width * 4);
    if (!surface)
    {
        SDL_free(buffer);
        SDL_Log("Failed to load image: %s", SDL_GetError());
        return false;
    }

    const SDL_PixelFormatDetails *format_details = SDL_GetPixelFormatDetails(surface->format);
    if (!SDL_SetSurfaceColorKey(surface, true, SDL_MapRGB(format_details, NULL, 0xff, 0x00, 0xff)))
    {
        SDL_Log("Couldn't set surface color key: %s", SDL_GetError());
        SDL_DestroySurface(surface);
        return false;
    }

    *texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!*texture)
    {
        SDL_Log("Could not create texture from surface: %s", SDL_GetError());
        SDL_DestroySurface(surface);
        return false;
    }

    if (!SDL_SetTextureScaleMode(*texture, SDL_SCALEMODE_NEAREST))
    {
        SDL_Log("Couldn't set texture scale mode: %s", SDL_GetError());
    }

    return true;
}

/* djb2 by Dan Bernstein
 * http://www.cse.yorku.ca/~oz/hash.html
 */
Uint64 generate_hash(const unsigned char *name)
{
    Uint64 hash = 5381;
    Uint32 c;

    while ((c = *name++))
    {
        hash = ((hash << 5) + hash) + c;
    }

    return hash;
}

unsigned int set_bit(unsigned int *number, unsigned int n)
{
    *number = *number | (1u << n);
    return *number;
}

unsigned int clear_bit(unsigned int *number, unsigned int n)
{
    *number = *number & ~(1u << n);
    return *number;
}

unsigned int toggle_bit(unsigned int *number, unsigned int n)
{
    *number = *number ^ (1u << n);
    return *number;
}

bool check_bit(unsigned int number, unsigned int n)
{
    return (bool)((number >> n) & 1u);
}

button_t get_button_from_key(SDL_Keycode key)
{
    switch (key)
    {
        case SDLK_BACKSPACE:
            return BTN_BACKSPACE;
        case SDLK_1:
            return BTN_1;
        case SDLK_2:
            return BTN_2;
        case SDLK_3:
            return BTN_3;
        case SDLK_4:
            return BTN_4;
        case SDLK_5:
        case SDLK_LSHIFT:
            return BTN_5;
        case SDLK_6:
            return BTN_6;
        case SDLK_7:
        case SDLK_SPACE:
            return BTN_7;
        case SDLK_8:
            return BTN_8;
        case SDLK_9:
            return BTN_9;
        case SDLK_0:
            return BTN_0;
        case SDLK_ASTERISK:
            return BTN_ASTERISK;
        case SDLK_HASH:
            return BTN_HASH;
        case SDLK_SOFTLEFT:
        case SDLK_ESCAPE:
            return BTN_SOFTLEFT;
        case SDLK_SOFTRIGHT:
            return BTN_SOFTRIGHT;
        case SDLK_SELECT:
            return BTN_SELECT;
        case SDLK_UP:
        case SDLK_W:
            return BTN_UP;
        case SDLK_DOWN:
        case SDLK_S:
            return BTN_DOWN;
        case SDLK_LEFT:
        case SDLK_A:
            return BTN_LEFT;
        case SDLK_RIGHT:
        case SDLK_D:
            return BTN_RIGHT;
    }

    return 0u;
}

button_t get_button_from_gamepad(Uint8 pad_btn)
{
    switch (pad_btn)
    {
        case SDL_GAMEPAD_BUTTON_SOUTH:
            return BTN_7;
        case SDL_GAMEPAD_BUTTON_EAST:
            return BTN_5;
        case SDL_GAMEPAD_BUTTON_DPAD_UP:
            return BTN_UP;
        case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
            return BTN_DOWN;
        case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
            return BTN_LEFT;
        case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
            return BTN_RIGHT;
    }

    return 0u;
}
