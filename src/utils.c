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

    if (!file_name) {
        return true;
    }
    SDL_Log("Loading texture from file: %s", file_name);

    buffer = (Uint8 *)load_binary_file_from_path(file_name);
    if (!buffer) {
        SDL_Log("Failed to load asset: %s", file_name);
        return false;
    }

    file_size = size_of_file(file_name);
    asset = SDL_IOFromConstMem((Uint8 *)buffer, file_size);
    if (!asset) {
        SDL_free(buffer);
        SDL_Log("Failed to convert asset %s: %s", file_name, SDL_GetError());
        return false;
    }

    int width, height, bpp;
    stbi_uc *pixels = stbi_load_from_memory(buffer, file_size, &width, &height, &bpp, 4);
    if (!pixels) {
        SDL_Log("Couldn't load image data: %s", stbi_failure_reason());
        SDL_free(buffer);
        return false;
    }
    SDL_free(buffer);

    *surface = SDL_CreateSurfaceFrom(width, height, SDL_PIXELFORMAT_RGBA32, (void *)pixels, width * 4);
    if (!*surface) {
        SDL_free(buffer);
        SDL_Log("Failed to load image: %s", SDL_GetError());
        return false;
    }

    const SDL_PixelFormatDetails *format_details = SDL_GetPixelFormatDetails((*surface)->format);
    if (!SDL_SetSurfaceColorKey(*surface, true, SDL_MapRGB(format_details, NULL, 0xff, 0x00, 0xff))) {
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

    if (!file_name) {
        return true;
    }
    SDL_Log("Loading texture from file: %s", file_name);

    buffer = (Uint8 *)load_binary_file_from_path(file_name);
    if (!buffer) {
        SDL_Log("Failed to load asset: %s", file_name);
        return false;
    }

    file_size = size_of_file(file_name);
    asset = SDL_IOFromConstMem((Uint8 *)buffer, file_size);
    if (!asset) {
        SDL_free(buffer);
        SDL_Log("Failed to convert asset %s: %s", file_name, SDL_GetError());
        return false;
    }

    int width, height, bpp;
    stbi_uc *pixels = stbi_load_from_memory(buffer, file_size, &width, &height, &bpp, 4);
    if (!pixels) {
        SDL_Log("Couldn't load image data: %s", stbi_failure_reason());
        SDL_free(buffer);
        return false;
    }
    SDL_free(buffer);

    surface = SDL_CreateSurfaceFrom(width, height, SDL_PIXELFORMAT_RGBA32, (void *)pixels, width * 4);
    if (!surface) {
        SDL_free(buffer);
        SDL_Log("Failed to load image: %s", SDL_GetError());
        return false;
    }

    const SDL_PixelFormatDetails *format_details = SDL_GetPixelFormatDetails(surface->format);
    if (!SDL_SetSurfaceColorKey(surface, true, SDL_MapRGB(format_details, NULL, 0xff, 0x00, 0xff))) {
        SDL_Log("Couldn't set surface color key: %s", SDL_GetError());
        SDL_DestroySurface(surface);
        return false;
    }

    *texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!*texture) {
        SDL_Log("Could not create texture from surface: %s", SDL_GetError());
        SDL_DestroySurface(surface);
        return false;
    }

    if (!SDL_SetTextureScaleMode(*texture, SDL_SCALEMODE_NEAREST)) {
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

    while ((c = *name++)) {
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
