/** @file map.c
 *
 *  A minimalist, cross-platform puzzle-platformer, designed
 *  especially for the Nokia N-Gage.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#include <SDL3/SDL.h>
#include <stdio.h>

#include "aabb.h"
#include "map.h"
#include "pfs.h"
#include "utils.h"

#if !defined __EMSCRIPTEN__
#include "zlib.h"
#endif

#if defined __SYMBIAN32__
#define CUTE_TILED_SNPRINTF(ARGS...) (void)(ARGS)
#define CUTE_TILED_STRTOLL           strtol
#define CUTE_TILED_STRTOULL          strtoul
#define STRPOOL_EMBEDDED_STRNICMP    strncasecmp
#endif

#define CUTE_TILED_IMPLEMENTATION
#include "cute_tiled.h"

#include "config.h"

#define H_GRAVITY 0x0000d0b30d77f26b

#define H_IS_DEADLY   0x0377cc445c348313
#define H_IS_SOLID    0x001ae728dd16b21b
#define H_IS_WALL     0x0000d0b3a99dccd0
#define H_OBJECTGROUP 0xc0b9d518970be349
#define H_OFFSET_TOP  0x727241bd0a7e257e
#define H_SPAWN       0x00000031105f18ee
#define H_STR         0x000000000b88ab7e
#define H_TILELAYER   0x0377d9f70e844fb0

static void destroy_tiled_map(map_t *map)
{
    map->hash_id_objectgroup = 0;
    map->hash_id_tilelayer = 0;

    if (map->handle)
    {
        cute_tiled_free_map(map->handle);
    }
    map->handle = NULL;
}

#if !defined __EMSCRIPTEN__
static bool decompress_gz_buffer(Uint8 *compressed_data, size_t compressed_size, Uint8 **out_decompressed_data, uLongf *out_decompressed_size)
{
#if defined(__SYMBIAN32__)
    // Smaller chunk size to reduce memory fragmentation and pre-allocate
    // based on expected compression ratio (typically 2-4x for tiled maps).
    const size_t CHUNK_SIZE = 8192;              // 8KB chunks instead of 16KB.
    size_t estimated_size = compressed_size * 3; // Assume 3x compression ratio.
#else
    const size_t CHUNK_SIZE = 16384;
    size_t estimated_size = CHUNK_SIZE;
#endif

    Uint8 *output = NULL;
    size_t output_capacity = 0;
    size_t output_size = 0;

    z_stream strm = { 0 };
    strm.next_in = compressed_data;
    strm.avail_in = (uInt)compressed_size;

    if (inflateInit2(&strm, 16 + MAX_WBITS) != Z_OK)
    {
        SDL_Log("inflateInit2 failed");
        return false;
    }

#if defined(__SYMBIAN32__)
    // Pre-allocate based on estimated size to reduce realloc calls
    output = (Uint8 *)SDL_malloc(estimated_size);
    if (output)
    {
        output_capacity = estimated_size;
    }
#endif

    int ret;
    do
    {
        if (output_size + CHUNK_SIZE > output_capacity)
        {
            output_capacity += CHUNK_SIZE;
            Uint8 *new_output = (Uint8 *)SDL_realloc(output, output_capacity);
            if (!new_output)
            {
                SDL_Log("Failed to allocate memory during decompression");
                SDL_free(output);
                inflateEnd(&strm);
                return false;
            }
            output = new_output;
        }

        strm.next_out = output + output_size;
        strm.avail_out = (uInt)CHUNK_SIZE;

        ret = inflate(&strm, Z_NO_FLUSH);
        if (ret != Z_OK && ret != Z_STREAM_END)
        {
            SDL_Log("inflate failed with code: %d", ret);
            SDL_free(output);
            inflateEnd(&strm);
            return false;
        }

        output_size += (CHUNK_SIZE - strm.avail_out);
    } while (ret != Z_STREAM_END);

    inflateEnd(&strm);

    *out_decompressed_data = output;
    *out_decompressed_size = (uLongf)output_size;

    return true;
}
#endif

static bool load_tiled_map(const char *file_name, map_t *map)
{
    Uint8 *buffer;

    if (map->handle)
    {
        destroy_tiled_map(map);
    }

    buffer = (Uint8 *)load_binary_file_from_path(file_name);
    if (!buffer)
    {
        SDL_Log("Failed to load resource: %s", file_name);
        return false;
    }

    int buffer_size = 0;
#if !defined __EMSCRIPTEN__
    Uint8 *decompressed_data = NULL;
    uLongf decompressed_size = 0;

    if (decompress_gz_buffer(buffer, size_of_file(file_name), &decompressed_data, &decompressed_size))
    {
        SDL_Log("Decompressed to %lu bytes", decompressed_size);
        SDL_free(buffer); // free original .gz buffer
        buffer = decompressed_data;
        buffer_size = decompressed_size;
    }
    else
    {
        SDL_Log("Decompression failed");
    }
#else
    buffer_size = size_of_file(file_name);
#endif

    map->handle = cute_tiled_load_map_from_memory((const void *)buffer, buffer_size, NULL);
    if (!map->handle)
    {
        SDL_free(buffer);
        SDL_Log("%s", cute_tiled_error_reason);
        return false;
    }
    SDL_free(buffer);

    Uint32 argb_color = map->handle->backgroundcolor;
    map->bg_r = (argb_color >> 16) & 0xFF;
    map->bg_g = (argb_color >> 8) & 0xFF;
    map->bg_b = argb_color & 0xFF;

    cute_tiled_layer_t *layer = map->handle->layers;
    while (layer)
    {
        if (H_TILELAYER == generate_hash((const unsigned char *)layer->type.ptr))
        {
            if (!map->hash_id_tilelayer)
            {
                map->hash_id_tilelayer = layer->type.hash_id;
                SDL_Log("Set hash ID for tile layer: %llu", map->hash_id_tilelayer);
            }
            map->layer_count += 1;
        }
        else if (H_OBJECTGROUP == generate_hash((const unsigned char *)layer->type.ptr))
        {
            if (!map->hash_id_objectgroup)
            {
                map->hash_id_objectgroup = layer->type.hash_id;
                SDL_Log("Set hash ID for object group: %llu", map->hash_id_objectgroup);
            }
        }
        layer = layer->next;
    }

    return true;
}

static void destroy_textures(map_t *map)
{
    if (map->render_canvas)
    {
        SDL_DestroySurface(map->render_canvas);
        map->render_canvas = NULL;
    }

    if (map->render_target)
    {
        SDL_DestroyTexture(map->render_target);
        map->render_target = NULL;
    }
}

static bool create_textures(SDL_Renderer *renderer, map_t *map)
{
    if (!renderer || !map || !map->handle)
    {
        SDL_Log("Invalid parameters for creating textures.");
        return false;
    }

    if (map->render_target || map->render_canvas)
    {
        destroy_textures(map);
    }

    map->height = map->handle->height * map->handle->tilesets->tileheight;
    map->width = map->handle->width * map->handle->tilesets->tilewidth;

    // Cache frequently accessed tileset dimensions to reduce pointer dereferences
    map->cached_tilewidth = map->handle->tilesets->tilewidth;
    map->cached_tileheight = map->handle->tilesets->tileheight;
    map->cached_map_width = map->handle->width;

#ifndef __DREAMCAST__
    SDL_PixelFormat pixel_format = SDL_PIXELFORMAT_XRGB4444;
#else
    SDL_PixelFormat pixel_format = SDL_PIXELFORMAT_ARGB1555;
#endif

    map->render_target = SDL_CreateTexture(renderer, pixel_format, SDL_TEXTUREACCESS_STREAMING, map->width, map->height);
    if (!map->render_target)
    {
        SDL_Log("Error creating texture: %s", SDL_GetError());
        return false;
    }

    if (!SDL_SetTextureScaleMode(map->render_target, SDL_SCALEMODE_NEAREST))
    {
        SDL_Log("Couldn't set texture scale mode: %s", SDL_GetError());
    }

    map->render_canvas = SDL_CreateSurface(map->width, map->height, pixel_format);

    if (!map->render_canvas)
    {
        SDL_Log("Error creating temporary surface: %s", SDL_GetError());
        return false;
    }

    return true;
}

static inline int get_local_id(int gid, cute_tiled_map_t *map)
{
    register int local_id = gid - map->tilesets->firstgid;
    return local_id >= 0 ? local_id : 0;
}

#if defined(__SYMBIAN32__)
// Tiles are always 16x16, use bit shifts (2^4 = 16).
static inline void get_tile_position(int gid, int *pos_x, int *pos_y, cute_tiled_map_t *map)
{
    cute_tiled_tileset_t *tileset = map->tilesets;
    register int local_id = get_local_id(gid, map);
    register int columns = tileset->columns;

    // Always shift by 4 for 16x16 tiles (eliminates branch overhead).
    *pos_x = (local_id % columns) << 4; // * 16
    *pos_y = (local_id / columns) << 4; // * 16
}
#else
static inline void get_tile_position(int gid, int *pos_x, int *pos_y, cute_tiled_map_t *map)
{
    cute_tiled_tileset_t *tileset = map->tilesets;
    register int local_id = get_local_id(gid, map);
    register int tilewidth = tileset->tilewidth;
    register int columns = tileset->columns;

    *pos_x = (local_id % columns) * tilewidth;
    *pos_y = (local_id / columns) * tileset->tileheight;
}
#endif

static inline void get_frame_position(int frame_index, int width, int height, int *pos_x, int *pos_y, int column_count)
{
    register int frame = frame_index % column_count;
    *pos_x = frame * width;
    *pos_y = (frame_index / column_count) * height;
}

static inline bool is_gid_valid(int gid, cute_tiled_map_t *map)
{
    return (gid != 0);
}

static bool set_object_animation(int gid, int *anim_length, int *id, cute_tiled_map_t *map)
{
    int local_id = get_local_id(gid, map);
    cute_tiled_tileset_t *tileset = map->tilesets;
    cute_tiled_tile_descriptor_t *tile = tileset->tiles;

    while (tile)
    {
        if (tile->tile_index == local_id)
        {
            if (tile->animation)
            {
                if (anim_length)
                {
                    *anim_length = tile->frame_count;
                }
                if (id)
                {
                    *id = tile->animation->tileid;
                }
                return true;
            }
        }
        tile = tile->next;
    }

    return false;
}

static inline int remove_gid_flip_bits(int gid)
{
    return cute_tiled_unset_flags(gid);
}

static bool tile_has_properties(int gid, cute_tiled_tile_descriptor_t **tile, cute_tiled_map_t *map)
{
    int local_id;

    local_id = gid - map->tilesets->firstgid;

    while ((*tile))
    {
        if ((*tile)->tile_index == local_id)
        {
            if (0 < (*tile)->property_count)
            {
                return true;
            }
        }
        (*tile) = (*tile)->next;
    }

    return false;
}

static bool is_layer_of_type(const layer_type type, cute_tiled_layer_t *layer, map_t *map)
{
    switch (type)
    {
        case TILE_LAYER:
            if (map->hash_id_tilelayer == layer->type.hash_id)
            {
                return true;
            }
            break;
        case OBJECT_GROUP:
            if (map->hash_id_objectgroup == layer->type.hash_id)
            {
                return true;
            }
            break;
    }

    return false;
}

cute_tiled_object_t *get_head_object(cute_tiled_layer_t *layer, map_t *map)
{
    if (is_layer_of_type(OBJECT_GROUP, layer, map))
    {
        return layer->objects;
    }

    return NULL;
}

static int get_next_object_id(int gid, int current_frame, cute_tiled_map_t *map)
{
    cute_tiled_tileset_t *tileset = map->tilesets;
    cute_tiled_tile_descriptor_t *tile = tileset->tiles;

    while (tile)
    {
        if (tile->tile_index == gid)
        {
            return tile->animation[current_frame].tileid;
        }
        tile = tile->next;
    }

    return 0;
}

static void load_property(const Uint64 name_hash, cute_tiled_property_t *properties, int property_count, map_t *map)
{
    // Early exit for empty properties or null pointer
    if (property_count == 0 || !properties)
    {
        return;
    }

    // Linear search
    for (register int index = 0; index < property_count; index += 1)
    {
        if (name_hash == generate_hash((const unsigned char *)properties[index].name.ptr))
        {
            // Skip null property names
            if (!properties[index].name.ptr)
            {
                return;
            }

            // Handle property based on type
            switch (properties[index].type)
            {
                case CUTE_TILED_PROPERTY_INT:
                    map->integer_property = properties[index].data.integer;
                    return;
                case CUTE_TILED_PROPERTY_BOOL:
                    map->boolean_property = (bool)properties[index].data.boolean;
                    return;
                case CUTE_TILED_PROPERTY_FLOAT:
                    map->decimal_property = (float)properties[index].data.floating;
                    return;
                case CUTE_TILED_PROPERTY_STRING:
                    map->string_property = properties[index].data.string.ptr;
                    return;
                default:
                    // COLOR, FILE, NONE - nothing to do
                    return;
            }
        }
    }
}

static bool get_boolean_property(const Uint64 name_hash, cute_tiled_property_t *properties, int property_count, map_t *map)
{
    // Early exit optimization for empty properties
    if (property_count == 0 || !properties)
    {
        return false;
    }

    map->boolean_property = false;
    load_property(name_hash, properties, property_count, map);
    return map->boolean_property;
}

static float get_decimal_property(const Uint64 name_hash, cute_tiled_property_t *properties, int property_count, map_t *map)
{
    // Early exit optimization for empty properties
    if (property_count == 0 || !properties)
    {
        return 0.0f;
    }

    map->decimal_property = 0.0;
    load_property(name_hash, properties, property_count, map);
    return map->decimal_property;
}

static int get_integer_property(const Uint64 name_hash, cute_tiled_property_t *properties, int property_count, map_t *map)
{
    // Early exit optimization for empty properties
    if (property_count == 0 || !properties)
    {
        return 0;
    }

    map->integer_property = 0;
    load_property(name_hash, properties, property_count, map);
    return map->integer_property;
}

static const char *get_string_property(const Uint64 name_hash, cute_tiled_property_t *properties, int property_count, map_t *map)
{
    // Early exit optimization for empty properties
    if (property_count == 0 || !properties)
    {
        return NULL;
    }

    map->string_property = NULL;
    load_property(name_hash, properties, property_count, map);
    return map->string_property;
}

static int get_tile_property_count(cute_tiled_tile_descriptor_t *tile)
{
    return tile->property_count;
}

static bool load_tiles(map_t *map)
{
    if (map->tile_desc)
    {
        SDL_free(map->tile_desc);
        map->tile_desc = NULL;
    }

    cute_tiled_layer_t *layer = map->handle->layers;

    map->tile_desc_count = map->handle->height * map->handle->width;

    if (map->tile_desc_count < 0)
    {
        return true;
    }

    map->tile_desc = (tile_desc_t *)SDL_calloc((size_t)map->tile_desc_count, sizeof(struct tile_desc));
    if (!map->tile_desc)
    {
        return false;
    }

    // Use cached dimensions.
    register int map_width = map->cached_map_width;
    register int map_height = map->handle->height;
    cute_tiled_tileset_t *tileset = map->handle->tilesets;

    while (layer)
    {
        if (is_layer_of_type(TILE_LAYER, layer, map))
        {
            int *layer_content = layer->data;
            register int tile_index = 0;

            for (int index_height = 0; index_height < map_height; index_height += 1)
            {
                for (int index_width = 0; index_width < map_width; index_width += 1, tile_index += 1)
                {
                    cute_tiled_tile_descriptor_t *tile = tileset->tiles;
                    int gid = remove_gid_flip_bits(layer_content[tile_index]);

                    if (tile_has_properties(gid, &tile, map->handle))
                    {
                        int prop_cnt = get_tile_property_count(tile);
                        cute_tiled_property_t *props = tile->properties;
                        tile_desc_t *current_tile = &map->tile_desc[tile_index];

                        if (get_boolean_property(H_IS_DEADLY, props, prop_cnt, map))
                        {
                            current_tile->is_deadly = true;
                        }
                        if (get_boolean_property(H_IS_SOLID, props, prop_cnt, map))
                        {
                            current_tile->is_solid = true;
                        }
                        if (get_boolean_property(H_IS_WALL, props, prop_cnt, map))
                        {
                            current_tile->is_wall = true;
                        }
                        current_tile->offset_top = get_integer_property(H_OFFSET_TOP, props, prop_cnt, map);
                    }
                }
            }
        }
        layer = layer->next;
    }

    return true;
}

static bool load_tileset(map_t *map)
{
    bool exit_code = true;
    char file_name[16] = { 0 };

    SDL_snprintf(file_name, 16, "%s", map->handle->tilesets->image.ptr);

    map->tileset_hash = generate_hash((const unsigned char *)file_name);

    if (map->tileset_hash != map->prev_tileset_hash)
    {
        map->prev_tileset_hash = map->tileset_hash;

        if (!load_surface_from_file((const char *)file_name, &map->tileset_surface))
        {
            SDL_Log("Error loading tileset image '%s'", file_name);
            exit_code = false;
        }
    }

    return exit_code;
}

static bool load_objects(map_t *map)
{
    if (map->obj)
    {
        SDL_free(map->obj);
        map->obj = NULL;
    }

    cute_tiled_layer_t *layer = map->handle->layers;

    while (layer)
    {
        if (layer->visible)
        {
            if (is_layer_of_type(OBJECT_GROUP, layer, map))
            {
                cute_tiled_object_t *object = get_head_object(layer, map);
                while (object)
                {
                    // Use generate_hash instead of cached hash_id.
                    Uint64 obj_name_hash = generate_hash((const unsigned char *)object->name.ptr);

                    if (H_COIN == obj_name_hash)
                    {
                        map->coins_left += 1;
                    }

                    if (H_SPAWN == obj_name_hash)
                    {
                        map->spawn_x = (int)object->x;
                        map->spawn_y = (int)object->y;
                    }

                    map->obj_count += 1;
                    object = object->next;
                }
            }
        }
        layer = layer->next;
    }
    map->coin_max = map->coins_left;

    if (map->obj_count > 0)
    {
        SDL_Log("Loading %u object(s)", map->obj_count);

        map->obj = (obj_t *)SDL_calloc((size_t)map->obj_count, sizeof(struct obj));
        if (!map->obj)
        {
            SDL_Log("Error allocating memory for objects");
            return false;
        }

        // Initialise objects.
        int index = 0;
        layer = map->handle->layers;
        while (layer)
        {
            if (layer->visible)
            {
                if (is_layer_of_type(OBJECT_GROUP, layer, map))
                {
                    cute_tiled_object_t *object = get_head_object(layer, map);
                    while (object)
                    {
                        // Use generate_hash instead of cached hash_id
                        Uint64 obj_name_hash = generate_hash((const unsigned char *)object->name.ptr);

                        if (H_BLOCK == obj_name_hash)
                        {
                            if (get_string_property(H_STR, object->properties, object->property_count, map))
                            {
                                map->obj[index].str = SDL_strdup(map->string_property);
                            }
                        }

                        map->obj[index].gid = remove_gid_flip_bits(object->gid);
                        map->obj[index].object_id = object->id;
                        map->obj[index].x = (int)object->x;
                        map->obj[index].y = (int)object->y;
                        index += 1;

                        object = object->next;
                    }
                }
            }
            layer = layer->next;
        }
    }
    else
    {
        return true;
    }

    return true;
}

static inline int lookup_lgbtq_tile_id(int id)
{
    // Optimize range checks: use unsigned subtraction trick
    register unsigned int offset1 = id - 930;
    register unsigned int offset2 = id - 980;
    return ((offset1 <= 19) || (offset2 <= 19)) ? id - 100 : id;
}

void destroy_map(map_t *map)
{
    if (!map)
    {
        return;
    }

    // Free up allocated memory in reverse order.

    // [6] Objects.
    if (map->obj)
    {
        for (int index = 0; index < map->obj_count; index += 1)
        {
            if (map->obj[index].str)
            {
                SDL_free((void *)map->obj[index].str);
                map->obj[index].str = NULL;
            }
        }

        SDL_free(map->obj);
        map->obj = NULL;
    }

    // [5] Tileset.
    if (map->tileset_surface)
    {
        SDL_DestroySurface(map->tileset_surface);
        map->tileset_surface = NULL;
    }

    // [4] Tiles.
    if (map->tile_desc)
    {
        SDL_free(map->tile_desc);
        map->tile_desc = NULL;
    }

    // [3] Textures & Surfaces.
    destroy_textures(map);

    // [2] Tiled map.
    destroy_tiled_map(map);

    // [1] Map.
    SDL_free(map);
}

bool load_map(const char *file_name, map_t **map, SDL_Renderer *renderer)
{
    bool exit_code = true;

    SDL_Log("Loading map: %s", file_name);

    // Load map file and allocate required memory.

    // [1] Map.
    if (!*map)
    {
        *map = (map_t *)SDL_calloc(1, sizeof(struct map));
        if (!*map)
        {
            SDL_Log("Error allocating memory for map");
            return false;
        }
    }
    else
    {
        (*map)->obj_count = 0;
        (*map)->coins_left = 0;
        (*map)->layer_count = 0;
        (*map)->spawn_x = 0;
        (*map)->spawn_y = 0;
        (*map)->static_tiles_rendered = false;
        (*map)->time_a = 0;
        (*map)->time_b = 0;
        (*map)->delta_time = 0;
        (*map)->time_since_last_frame = 0;
    }

    // [2] Tiled map.
    if (!load_tiled_map(file_name, *map))
    {
        exit_code = false;
        goto exit;
    }

    // [3] Textures & Surfaces.
    if (!create_textures(renderer, *map))
    {
        SDL_Log("Error creating textures and surfaces for map");
        exit_code = false;
        goto exit;
    }

    // [4] Tiles.
    if (!load_tiles(*map))
    {
        exit_code = false;
        goto exit;
    }

    // [5] Tileset.
    if (!load_tileset(*map))
    {
        exit_code = false;
        goto exit;
    }

    // [6] Objects.
    if (!load_objects(*map))
    {
        exit_code = false;
        goto exit;
    }

exit:
    if (!exit_code)
    {
        destroy_map(*map);
    }

    return exit_code;
}

bool render_map(map_t *map, SDL_Renderer *renderer, bool *has_updated)
{
    cute_tiled_layer_t *layer;
    cute_tiled_layer_t *prev_layer = NULL;
    *has_updated = false;

    if (!map || !renderer)
    {
        SDL_Log("Invalid parameters: map or renderer is NULL.");
        return false;
    }

    // Static tiles have already been rendered.
    if (map->static_tiles_rendered)
    {
        // Fast path: skip if no animated objects.
        if (!map->obj_count)
        {
            return true;
        }

        map->time_b = map->time_a;
        map->time_a = SDL_GetTicks();

        if (map->time_a > map->time_b)
        {
            map->delta_time = map->time_a - map->time_b;
        }
        else
        {
            map->delta_time = map->time_b - map->time_a;
        }

        // Update and render objects.
        map->time_since_last_frame += map->delta_time;

        if (map->time_since_last_frame >= (1000 / ANIM_FPS))
        {
            map->time_since_last_frame = 0;
            *has_updated = true;

            // Use cached dimensions to reduce pointer dereferences.
            register int tilewidth = map->cached_tilewidth;
            register int tileheight = map->cached_tileheight;
            register bool use_lgbtq = map->use_lgbtq_flag;
            register bool no_coins = !map->coins_left;
            register int obj_count = map->obj_count - 1;
            obj_t *obj_array = map->obj;
            SDL_Surface *tileset = map->tileset_surface;
            SDL_Surface *canvas = map->render_canvas;

            // Preallocate rects to avoid repeated stack allocation
            SDL_Rect dst = { .w = tilewidth, .h = tileheight };
            SDL_Rect src = { .w = tilewidth, .h = tileheight };
            SDL_Rect canvas_src = { .w = tilewidth, .h = tileheight };

            for (int index = 0; index < obj_count; index += 1)
            {
                obj_t *obj = &obj_array[index];

                // Fast path: skip invalid GIDs immediately.
                if (obj->gid <= 0)
                {
                    continue;
                }

                // Handle door state
                if (H_DOOR == obj->hash && no_coins)
                {
                    obj->start_frame = 1;
                    obj->current_frame = 1;
                }

                int local_id;

                if (use_lgbtq)
                {
                    local_id = lookup_lgbtq_tile_id(obj->id) + 1;
                }
                else
                {
                    local_id = obj->id + 1;
                }

                dst.x = obj->x;
                dst.y = obj->y;

                int tmp_x, tmp_y;
                get_tile_position(local_id, &tmp_x, &tmp_y, map->handle);
                src.x = tmp_x;
                src.y = tmp_y;

                // Blit canvas tile first (for transparency simulation).
                canvas_src.x = obj->canvas_src_x;
                canvas_src.y = obj->canvas_src_y;
                SDL_BlitSurface(tileset, &canvas_src, canvas, &dst);

                // Fast path: skip hidden objects.
                if (!obj->is_hidden)
                {
                    SDL_BlitSurface(tileset, &src, canvas, &dst);

                    // Update animation frame if animated.
                    if (obj->anim_length)
                    {
                        obj->current_frame += 1;
                        if (obj->current_frame >= obj->anim_length + obj->start_frame)
                        {
                            obj->current_frame = obj->start_frame;
                        }
                    }
                }

                obj->id = get_next_object_id(obj->gid, obj->current_frame, map->handle);
            }
        }

        return true;
    }

    // Static tiles have not been rendered yet. Do it once!
    int index = 0;
    layer = map->handle->layers;

    while (layer)
    {
        SDL_Rect dst = { 0 };
        SDL_Rect src = { 0 };

        if (is_layer_of_type(TILE_LAYER, layer, map))
        {
            if (layer->visible)
            {
                // Use cached dimensions to reduce pointer dereferences.
                register int map_width = map->cached_map_width;
                register int map_height = map->handle->height;
                register int tilewidth = map->cached_tilewidth;
                register int tileheight = map->cached_tileheight;
                int *layer_content = layer->data;
                register int tile_index = 0;

                src.w = dst.w = tilewidth;
                src.h = dst.h = tileheight;

                for (int index_height = 0; index_height < map_height; index_height += 1)
                {
                    register int dst_y = index_height * tileheight;
                    for (int index_width = 0; index_width < map_width; index_width += 1, tile_index += 1)
                    {
                        int gid = remove_gid_flip_bits(layer_content[tile_index]);

                        if (is_gid_valid(gid, map->handle))
                        {
                            dst.x = index_width * tilewidth;
                            dst.y = dst_y;

                            int tmp_x, tmp_y;
                            get_tile_position(gid, &tmp_x, &tmp_y, map->handle);
                            src.x = tmp_x;
                            src.y = tmp_y;

                            SDL_BlitSurface(map->tileset_surface, &src, map->render_canvas, &dst);
                        }
                    }
                }

                const char *layer_name = layer->name.ptr;
                SDL_Log("Render map layer: %s", layer_name);
            }
        }
        else if (is_layer_of_type(OBJECT_GROUP, layer, map))
        {
            cute_tiled_object_t *object = get_head_object(layer, map);
            while (object)
            {
                int gid = remove_gid_flip_bits(object->gid);

                if (is_gid_valid(gid, map->handle))
                {
                    int anim_length = 0;
                    int id = 0;

                    // Cache object position to avoid multiple pointer dereferences.
                    float obj_x = object->x;
                    float obj_y = object->y;

                    src.w = dst.w = map->handle->tilesets->tilewidth;
                    src.h = dst.h = map->handle->tilesets->tileheight;
                    dst.x = (int)obj_x;
                    dst.y = (int)obj_y - map->handle->tilesets->tileheight;

                    int tmp_x, tmp_y;
                    get_tile_position(gid, &tmp_x, &tmp_y, map->handle);
                    src.x = tmp_x;
                    src.y = tmp_y;

                    set_object_animation(gid, &anim_length, &id, map->handle);
                    map->obj[index].gid = get_local_id(gid, map->handle);
                    map->obj[index].id = id;
                    map->obj[index].x = dst.x;
                    map->obj[index].y = dst.y;
                    map->obj[index].current_frame = 0;
                    map->obj[index].anim_length = anim_length;
                    map->obj[index].object_id = object->id;
                    // Use generate_hash instead of cached hash_id.
                    map->obj[index].hash = generate_hash((const unsigned char *)object->name.ptr);

                    if (H_DOOR == map->obj[index].hash)
                    {
                        map->obj[index].anim_length = 0;
                    }

                    if (prev_layer)
                    {
                        int index_width = dst.x / map->handle->tilesets->tilewidth;
                        int index_height = dst.y / map->handle->tilesets->tileheight;
                        int *layer_content_below = prev_layer->data;
                        int gid_below = remove_gid_flip_bits(layer_content_below[(index_height * map->handle->width) + index_width]);
                        if (is_gid_valid(gid_below, map->handle))
                        {
                            int tmp_x_below, tmp_y_below;
                            get_tile_position(gid_below, &tmp_x_below, &tmp_y_below, map->handle);
                            map->obj[index].canvas_src_x = tmp_x_below;
                            map->obj[index].canvas_src_y = tmp_y_below;
                        }
                    }

                    index += 1;
                }

                object = object->next;
            }

            const char *layer_name = layer->name.ptr;
            SDL_Log("Render obj layer: %s", layer_name);
        }
        prev_layer = layer;
        layer = layer->next;
    }

    if (!SDL_UpdateTexture(map->render_target, NULL, map->render_canvas->pixels, map->render_canvas->pitch))
    {
        SDL_Log("Error updating static tile texture: %s", SDL_GetError());
        return false;
    }

    map->static_tiles_rendered = true;
    *has_updated = true;

    return true;
}

bool object_intersects(aabb_t bb, map_t *map, int *index_ptr)
{
    cute_tiled_layer_t *layer = map->handle->layers;
    *index_ptr = 0;

    while (layer)
    {
        if (layer->visible)
        {
            if (is_layer_of_type(OBJECT_GROUP, layer, map))
            {
                cute_tiled_object_t *object = get_head_object(layer, map);
                while (object)
                {
                    int gid = remove_gid_flip_bits(object->gid);

                    if (is_gid_valid(gid, map->handle))
                    {
                        aabb_t object_aabb;
#if defined(__SYMBIAN32__)
                        // All tiles are 16x16, use constant for half-size.
                        const int obj_half = 8;
                        object_aabb.left = object->x - obj_half;
                        object_aabb.right = object->x + obj_half;
                        object_aabb.top = object->y - obj_half;
                        object_aabb.bottom = object->y + obj_half;
#else
                        int obj_half_width = (int)(object->width / 2);
                        int obj_half_height = (int)(object->height / 2);
                        object_aabb.left = object->x - obj_half_width;
                        object_aabb.right = object->x + obj_half_width;
                        object_aabb.top = object->y - obj_half_height;
                        object_aabb.bottom = object->y + obj_half_height;
#endif

                        if (do_intersect(bb, object_aabb))
                        {
                            return true;
                        }
                        *index_ptr += 1;
                    }
                    object = object->next;
                }
            }
        }
        layer = layer->next;
    }

    *index_ptr = -1;
    return false;
}

int get_tile_index(int pos_x, int pos_y, map_t *map)
{
    // Use cached dimensions to reduce pointer dereferences
    register int map_width = map->cached_map_width;
    register int max_index = map->tile_desc_count - 1;

#if defined(__SYMBIAN32__)
    // N-Gage optimization: Tiles are always 16x16, use bit shift by 4
    register int index = (pos_x >> 4) + ((pos_y >> 4) * map_width);
#else
    register int tilewidth = map->cached_tilewidth;
    register int tileheight = map->cached_tileheight;
    register int index = (pos_x / tilewidth) + ((pos_y / tileheight) * map_width);
#endif

    return (index > max_index) ? max_index : index;
}
