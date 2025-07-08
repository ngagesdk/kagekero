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

#include "aabb.h"
#include "map.h"
#include "pfs.h"
#include "utils.h"

#if defined __SYMBIAN32__
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

#define H_GRAVITY     0x0000d0b30d77f26b
#define H_IS_DEADLY   0x0377cc445c348313
#define H_IS_SOLID    0x001ae728dd16b21b
#define H_IS_WALL     0x0000d0b3a99dccd0
#define H_OBJECTGROUP 0xc0b9d518970be349
#define H_OFFSET_TOP  0x727241bd0a7e257e
#define H_SPAWN       0x00000031105f18ee
#define H_TILELAYER   0x0377d9f70e844fb0

static const char* death_lines[] = {
    "Ribbit. Guess I croaked for real that time.",
    "Note to self: spikes hurt more than they look.",
    "Good thing I'm not on an N-Gage — I'd need a new battery by now.",
    "One small hop for frog, one giant leap into failure.",
    "Put that one on my highlight reel — the blooper edition.",
    "If Madeline can do it a thousand times, so can I. Ribbit.",
    "I'd say 'call for help,' but my N-Gage has no signal.",
    "Death count: too high. Pride: still intact.",
    "Respawn faster than an N-Gage Arena match disconnects.",
    "Pro tip: Don’t do what I just did.",
    "At least when I dash into spikes, I don’t have to listen to a motivational speech first.",
    "Guess I just Madelined myself into the spikes again. Classic.",
    "Climbing my way to the afterlife — one dumb jump at a time.",
    "Next time I'll bring a motivational soundtrack like Madeline. Might help.",
    "If Madeline can face her demons, I can face... whatever just impaled me.",
    "Maybe I should’ve stuck to strawberries instead of pain.",
    "Bad jump. Worse landing. 10/10 Celeste tribute though.",
    "Hey Madeline! Save me a spot on the death counter!",
    "I'd call for help, but my inner demon's on vacation.",
    "Frog fact: unlike mountains, spikes always win.",
	"Like a Nokia brick - unbreakable? Not today.",
	"Should've brought my Celeste climbing gloves.",
	"Better luck next leap, Frogger 2003 edition.",
	"This is where I leapt... and this is where I floped.",
	"This is where I sticked the landing - just kidding.",
	"This was where I ribbited. This was where I regretted it.",
	"This was where I went full ninja. And full pancake.",
	"This was where I thought Frogger physics still applied."
};

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

#if defined __SYMBIAN32__
static bool decompress_gz_buffer(Uint8 *compressed_data, size_t compressed_size, Uint8 **out_decompressed_data, uLongf *out_decompressed_size)
{
    const size_t CHUNK_SIZE = 16384;
    Uint8 *output = NULL;
    size_t output_capacity = 0;
    size_t output_size = 0;

    z_stream strm = { 0 };
    strm.next_in = compressed_data;
    strm.avail_in = compressed_size;

    if (inflateInit2(&strm, 16 + MAX_WBITS) != Z_OK)
    {
        SDL_Log("inflateInit2 failed");
        return false;
    }

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
        strm.avail_out = CHUNK_SIZE;

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
    *out_decompressed_size = output_size;

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
#if defined __SYMBIAN32__
    Uint8 *decompressed_data = NULL;
    uLongf decompressed_size = 0;

    if (decompress_gz_buffer(buffer, size_of_file(file_name), &decompressed_data, &decompressed_size))
    {
        // Success � `decompressed_data` contains result, size in `decompressed_size`
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

static int get_local_id(int gid, cute_tiled_map_t *map)
{
    int local_id = gid - map->tilesets->firstgid;
    return local_id >= 0 ? local_id : 0;
}

static void get_tile_position(int gid, int *pos_x, int *pos_y, cute_tiled_map_t *map)
{
    cute_tiled_tileset_t *tileset = map->tilesets;
    int local_id = get_local_id(gid, map);

    *pos_x = (local_id % tileset->columns) * map->tilesets->tilewidth;
    *pos_y = (local_id / tileset->columns) * map->tilesets->tileheight;
}

static void get_frame_position(int frame_index, int width, int height, int *pos_x, int *pos_y, int column_count)
{
    *pos_x = (frame_index % column_count) * width;
    *pos_y = (frame_index / column_count) * height;
}

static bool is_gid_valid(int gid, cute_tiled_map_t *map)
{
    if (gid)
    {
        return true;
    }

    return false;
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

static int remove_gid_flip_bits(int gid)
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
    int index;
    bool prop_found = false;

    for (index = 0; index < property_count; index += 1)
    {
        if (name_hash == generate_hash((const unsigned char *)properties[index].name.ptr))
        {
            prop_found = true;
            break;
        }
    }

    if (!prop_found)
    {
        return;
    }

    // Entities are allowed to have no properties.
    if (0 == property_count)
    {
        return;
    }

    if (properties[index].name.ptr)
    {
        switch (properties[index].type)
        {
            case CUTE_TILED_PROPERTY_COLOR:
            case CUTE_TILED_PROPERTY_FILE:
            case CUTE_TILED_PROPERTY_NONE:
                break;
            case CUTE_TILED_PROPERTY_INT:
                map->integer_property = properties[index].data.integer;
                break;
            case CUTE_TILED_PROPERTY_BOOL:
                map->boolean_property = (bool)properties[index].data.boolean;
                break;
            case CUTE_TILED_PROPERTY_FLOAT:
                map->decimal_property = (float)properties[index].data.floating;
                break;
            case CUTE_TILED_PROPERTY_STRING:
                map->string_property = properties[index].data.string.ptr;
                break;
        }
    }
}

static bool get_boolean_property(const Uint64 name_hash, cute_tiled_property_t *properties, int property_count, map_t *map)
{
    map->boolean_property = false;
    load_property(name_hash, properties, property_count, map);
    return map->boolean_property;
}

static float get_decimal_property(const Uint64 name_hash, cute_tiled_property_t *properties, int property_count, map_t *map)
{
    map->decimal_property = 0.0;
    load_property(name_hash, properties, property_count, map);
    return map->decimal_property;
}

static int get_integer_property(const Uint64 name_hash, cute_tiled_property_t *properties, int property_count, map_t *map)
{
    map->integer_property = 0;
    load_property(name_hash, properties, property_count, map);
    return map->integer_property;
}

static const char *get_string_property(const Uint64 name_hash, cute_tiled_property_t *properties, int property_count, map_t *map)
{
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

    while (layer)
    {
        if (is_layer_of_type(TILE_LAYER, layer, map))
        {
            for (int index_height = 0; index_height < map->handle->height; index_height += 1)
            {
                for (int index_width = 0; index_width < map->handle->width; index_width += 1)
                {
                    cute_tiled_tileset_t *tileset = map->handle->tilesets;
                    cute_tiled_tile_descriptor_t *tile = tileset->tiles;
                    int *layer_content = layer->data;
                    int tile_index = (index_height * map->handle->width) + index_width;
                    int gid = remove_gid_flip_bits(layer_content[tile_index]);

                    if (tile_has_properties(gid, &tile, map->handle))
                    {
                        int prop_cnt = get_tile_property_count(tile);

                        if (get_boolean_property(H_IS_DEADLY, tile->properties, prop_cnt, map))
                        {
                            map->tile_desc[tile_index].is_deadly = true;
                        }
                        if (get_boolean_property(H_IS_SOLID, tile->properties, prop_cnt, map))
                        {
                            map->tile_desc[tile_index].is_solid = true;
                        }
                        if (get_boolean_property(H_IS_WALL, tile->properties, prop_cnt, map))
                        {
                            map->tile_desc[tile_index].is_wall = true;
                        }
                        map->tile_desc[tile_index].offset_top = get_integer_property(H_OFFSET_TOP, tile->properties, prop_cnt, map);
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
                    if (H_COIN == generate_hash((const unsigned char *)object->name.ptr))
                    {
                        map->coins_left += 1;
                    }

                    if (H_SPAWN == generate_hash((const unsigned char *)object->name.ptr))
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

static int lookup_lgbtq_tile_id(int id)
{
    if ((id >= 930 && id <= 949) || (id >= 980 && id <= 999))
    {
        return id - 100;
    }
    else
    {
        return id;
    }
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

            for (int index = 0; index < map->obj_count - 1; index += 1)
            {
                if (map->obj[index].gid <= 0)
                {
                    continue; // Skip invalid GIDs.
                }

                if (H_DOOR == map->obj[index].hash && !map->coins_left)
                {
                    map->obj[index].start_frame = 1;
                    map->obj[index].current_frame = 1;
                }

                SDL_Rect dst = { 0 };
                SDL_Rect src = { 0 };
                int gid = map->obj[index].gid;
                int next_tile_id = 0;
                int local_id;

                if (map->use_lgbtq_flag)
                {
                    local_id = lookup_lgbtq_tile_id(map->obj[index].id) + 1;
                }
                else
                {
                    local_id = map->obj[index].id + 1;
                }

                src.w = dst.w = map->handle->tilesets->tilewidth;
                src.h = dst.h = map->handle->tilesets->tileheight;
                src.x = 0;
                src.y = 0;
                dst.x = map->obj[index].x;
                dst.y = map->obj[index].y;

                int tmp_x, tmp_y;
                get_tile_position(local_id, &tmp_x, &tmp_y, map->handle);
                src.x = tmp_x;
                src.y = tmp_y;

                // Simulate transparency by blitting the uppermost static tile first.
                // Note: the canvas tile has to be on the layer below the object layer.
                SDL_Rect canvas_src = { 0 };
                canvas_src.w = src.w;
                canvas_src.h = src.h;
                canvas_src.x = map->obj[index].canvas_src_x;
                canvas_src.y = map->obj[index].canvas_src_y;
                SDL_BlitSurface(map->tileset_surface, &canvas_src, map->render_canvas, &dst);

                if (!map->obj[index].is_hidden)
                {
                    SDL_BlitSurface(map->tileset_surface, &src, map->render_canvas, &dst);

                    if (map->obj[index].anim_length)
                    {
                        map->obj[index].current_frame += 1;
                        if (map->obj[index].current_frame >= map->obj[index].anim_length + map->obj[index].start_frame)
                        {
                            map->obj[index].current_frame = map->obj[index].start_frame;
                        }
                    }
                }

                next_tile_id = get_next_object_id(gid, map->obj[index].current_frame, map->handle);

                map->obj[index].id = next_tile_id;
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
                for (int index_height = 0; index_height < map->handle->height; index_height += 1)
                {
                    for (int index_width = 0; index_width < map->handle->width; index_width += 1)
                    {
                        int *layer_content = layer->data;
                        int gid = remove_gid_flip_bits(layer_content[(index_height * map->handle->width) + index_width]);

                        if (is_gid_valid(gid, map->handle))
                        {
                            src.w = dst.w = map->handle->tilesets->tilewidth;
                            src.h = dst.h = map->handle->tilesets->tileheight;
                            dst.x = index_width * map->handle->tilesets->tilewidth;
                            dst.y = index_height * map->handle->tilesets->tileheight;

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

                    src.w = dst.w = map->handle->tilesets->tilewidth;
                    src.h = dst.h = map->handle->tilesets->tileheight;
                    dst.x = (int)object->x;
                    dst.y = (int)object->y - map->handle->tilesets->tileheight;

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
                        int obj_half_width = (int)(object->width / 2);
                        int obj_half_height = (int)(object->height / 2);
                        object_aabb.left = object->x - obj_half_width;
                        object_aabb.right = object->x + obj_half_width;
                        object_aabb.top = object->y - obj_half_height;
                        object_aabb.bottom = object->y + obj_half_height;

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
    int index;

    index = pos_x / map->handle->tilesets->tilewidth;
    index += (pos_y / map->handle->tilesets->tileheight) * map->handle->width;

    if (index > (map->tile_desc_count - 1))
    {
        index = map->tile_desc_count - 1;
    }

    return index;
}
