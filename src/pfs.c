/** @file pfs.c
 *
 *  Packed file system implementation.
 *  Adapted version, originally from The Mistral Report:
 *  https://montyontherun.itch.io/the-mistral-report
 *
 *  Copyright (c) 2019, Daniel Monteiro. All rights reserved.
 *  SPDX-License-Identifier: BSD-2-Clause
 *
 **/

#include <SDL3/SDL.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DATA_PATH_MAX_LEN 256

static char data_path[DATA_PATH_MAX_LEN];

void init_file_reader(void)
{
    SDL_snprintf(data_path, DATA_PATH_MAX_LEN, "%sdata.pfs", SDL_GetBasePath());
}

size_t size_of_file(const char *path)
{
    FILE *data_pack = fopen(data_path, "rb");
    char buffer[80 + 1];
    int32_t size = 0;
    int32_t offset = 0;
    int16_t entries = 0;

    fread(&entries, 2, 1, data_pack);

    for (int c = 0; c < entries; ++c)
    {
        uint8_t string_size = 0;

        fread(&offset, 4, 1, data_pack);
        fread(&string_size, 1, 1, data_pack);

        if (string_size > 80)
        {
            string_size = 80;
        }

        fread(&buffer, string_size + 1, 1, data_pack);

        if (!SDL_strncmp(buffer, path, string_size))
        {
            goto found;
        }
    }

found:
    if (offset == 0)
    {
        printf("failed to load %s\n", path);
        exit(-1);
    }

    fseek(data_pack, offset, SEEK_SET);
    fread(&size, 4, 1, data_pack);
    fclose(data_pack);

    return size;
}

uint8_t *load_binary_file_from_path(const char *path)
{
    FILE *data_pack = fopen(data_path, "rb");
    int32_t offset = 0;
    int16_t entries = 0;
    char buffer[80 + 1] = { 0 };
    int32_t size = 0;
    uint8_t *to_return;

    fread(&entries, 2, 1, data_pack);

    for (int c = 0; c < entries; ++c)
    {
        int8_t string_size = 0;

        fread(&offset, 4, 1, data_pack);
        fread(&string_size, 1, 1, data_pack);

        if (string_size > 80)
        {
            string_size = 80;
        }

        fread(&buffer, string_size + 1, 1, data_pack);

        if (!SDL_strncmp(buffer, path, string_size))
        {
            goto found;
        }
    }

    return NULL;

found:
    if (offset == 0)
    {
        printf("failed to load %s\n", path);
        exit(-1);
    }

    fseek(data_pack, offset, SEEK_SET);

    fread(&size, 4, 1, data_pack);
    to_return = (uint8_t *)SDL_calloc(1, size);

    fread(to_return, sizeof(uint8_t), size, data_pack);
    fclose(data_pack);

    return to_return;
}

FILE *open_binary_file_from_path(const char *path)
{
    FILE *data_pack = fopen(data_path, "rb");
    int32_t offset = 0;
    int16_t entries = 0;
    char buffer[85];
    int32_t size = 0;

    fread(&entries, 2, 1, data_pack);

    for (int c = 0; c < entries; ++c)
    {
        int8_t string_size = 0;

        fread(&offset, 4, 1, data_pack);
        fread(&string_size, 1, 1, data_pack);
        fread(&buffer, string_size + 1, 1, data_pack);

        if (!strcmp(buffer, path))
        {
            goto found;
        }
    }

    return NULL;

found:
    if (offset == 0)
    {
        printf("failed to load %s\n", path);
        exit(-1);
    }

    fseek(data_pack, offset, SEEK_SET);
    fread(&size, 4, 1, data_pack);

    return data_pack;
}
