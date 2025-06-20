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

#include "map.h"
#include "pfs.h"
#include "utils.h"

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
#define H_IS_COIN     0x0000d0b3a9931069
#define H_IS_DEADLY   0x0377cc445c348313
#define H_IS_DOOR     0x0000d0b3a9939d94
#define H_IS_SOLID    0x001ae728dd16b21b
#define H_IS_WALL     0x0000d0b3a99dccd0
#define H_OBJECTGROUP 0xc0b9d518970be349
#define H_OFFSET_TOP  0x727241bd0a7e257e
#define H_SPAWN       0x00000031105f18ee
#define H_TILELAYER   0x0377d9f70e844fb0

static cute_tiled_layer_t *get_head_layer(map_t *map)
{
    return map->handle->layers;
}

static bool load_tiled_map(const char *file_name, map_t *map)
{
    Uint8 *buffer;

    buffer = (Uint8 *)load_binary_file_from_path(file_name);
    if (!buffer)
    {
        SDL_Log("Failed to load resource: %s", file_name);
        return false;
    }

    map->handle = cute_tiled_load_map_from_memory((const void *)buffer, size_of_file(file_name), NULL);
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

    cute_tiled_layer_t *layer = get_head_layer(map);
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

static void destroy_tiled_map(map_t *map)
{
    map->hash_id_objectgroup = 0;
    map->hash_id_tilelayer = 0;

    if (map->handle)
    {
        cute_tiled_free_map(map->handle);
    }
}

static const int get_object_uid(cute_tiled_object_t *object)
{
    return object->id;
}

static const char *get_object_name(cute_tiled_object_t *object)
{
    return object->name.ptr;
}

static int get_object_property_count(cute_tiled_object_t *object)
{
    return object->property_count;
}

static const char *get_object_type_name(cute_tiled_object_t *object)
{
    return object->type.ptr;
}

static int get_tile_width(cute_tiled_map_t *map)
{
    return map->tilesets->tilewidth;
}

static int get_tile_height(cute_tiled_map_t *map)
{
    return map->tilesets->tileheight;
}

static bool create_textures(SDL_Renderer *renderer, map_t *map)
{
    if (!renderer || !map || !map->handle)
    {
        SDL_Log("Invalid parameters for creating textures.");
        return false;
    }

    map->height = map->handle->height * get_tile_height(map->handle);
    map->width = map->handle->width * get_tile_width(map->handle);

    map->render_target = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_XRGB4444,
        SDL_TEXTUREACCESS_STREAMING,
        map->width,
        map->height);

    if (!map->render_target)
    {
        SDL_Log("Error creating texture: %s", SDL_GetError());
        return false;
    }

    if (!SDL_SetTextureScaleMode(map->render_target, SDL_SCALEMODE_NEAREST))
    {
        SDL_Log("Couldn't set texture scale mode: %s", SDL_GetError());
    }

    map->render_canvas = SDL_CreateSurface(
        map->width,
        map->height,
        SDL_PIXELFORMAT_XRGB4444);

    if (!map->render_canvas)
    {
        SDL_Log("Error creating temporary surface: %s", SDL_GetError());
        return false;
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

static int get_first_gid(cute_tiled_map_t *map)
{
    return map->tilesets->firstgid;
}

static const char *get_layer_name(cute_tiled_layer_t *layer)
{
    return layer->name.ptr;
}

static int get_layer_property_count(cute_tiled_layer_t *layer)
{
    return layer->property_count;
}

static int get_local_id(int gid, cute_tiled_map_t *map)
{
    int local_id = gid - get_first_gid(map);
    return local_id >= 0 ? local_id : 0;
}

static void get_tile_position(int gid, int *pos_x, int *pos_y, cute_tiled_map_t *map)
{
    cute_tiled_tileset_t *tileset = map->tilesets;
    int local_id = get_local_id(gid, map);

    *pos_x = (local_id % tileset->columns) * get_tile_width(map);
    *pos_y = (local_id / tileset->columns) * get_tile_height(map);
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

static bool is_tile_animated(int gid, int *anim_length, int *id, cute_tiled_map_t *map)
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

    local_id = gid - get_first_gid(map);

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

static cute_tiled_tileset_t *get_head_tileset(cute_tiled_map_t *map)
{
    return map->tilesets;
}

static int get_next_animated_tile_id(int gid, int current_frame, cute_tiled_map_t *map)
{
    cute_tiled_tileset_t *tileset = get_head_tileset(map);
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

static int *get_layer_content(cute_tiled_layer_t *layer)
{
    return layer->data;
}

static int get_map_property_count(cute_tiled_map_t *map)
{
    return map->property_count;
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

static bool get_boolean_map_property(const Uint64 name_hash, map_t *map)
{
    int prop_cnt;

    if (!map)
    {
        return false;
    }

    prop_cnt = get_map_property_count(map->handle);
    map->boolean_property = false;
    load_property(name_hash, map->handle->properties, prop_cnt, map);
    return map->boolean_property;
}

static float get_decimal_map_property(const Uint64 name_hash, map_t *map)
{
    int prop_cnt;

    if (!map)
    {
        return 0.0;
    }

    prop_cnt = get_map_property_count(map->handle);
    map->decimal_property = 0.0;
    load_property(name_hash, map->handle->properties, prop_cnt, map);
    return map->decimal_property;
}

static int get_integer_map_property(const Uint64 name_hash, map_t *map)
{
    int prop_cnt;

    if (!map)
    {
        return 0;
    }

    prop_cnt = get_map_property_count(map->handle);
    map->integer_property = 0;
    load_property(name_hash, map->handle->properties, prop_cnt, map);
    return map->integer_property;
}

static const char *get_string_map_property(const Uint64 name_hash, map_t *map)
{
    int prop_cnt;

    if (!map)
    {
        return NULL;
    }

    prop_cnt = get_map_property_count(map->handle);
    map->string_property = NULL;
    load_property(name_hash, map->handle->properties, prop_cnt, map);
    return map->string_property;
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
    cute_tiled_layer_t *layer = get_head_layer(map);

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
                    cute_tiled_tileset_t *tileset = get_head_tileset(map->handle);
                    cute_tiled_tile_descriptor_t *tile = tileset->tiles;
                    int *layer_content = get_layer_content(layer);
                    int tile_index = (index_height * map->handle->width) + index_width;
                    int gid = remove_gid_flip_bits(layer_content[tile_index]);

                    if (tile_has_properties(gid, &tile, map->handle))
                    {
                        int prop_cnt = get_tile_property_count(tile);

                        if (get_boolean_property(H_IS_COIN, tile->properties, prop_cnt, map))
                        {
                            map->tile_desc[tile_index].is_coin = true;
                        }
                        if (get_boolean_property(H_IS_DEADLY, tile->properties, prop_cnt, map))
                        {
                            map->tile_desc[tile_index].is_deadly = true;
                        }
                        if (get_boolean_property(H_IS_DOOR, tile->properties, prop_cnt, map))
                        {
                            map->tile_desc[tile_index].is_door = true;
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

static bool load_entities(map_t *map)
{
    cute_tiled_layer_t *layer = get_head_layer(map);
    cute_tiled_object_t *object = NULL;

    if (map->entity_count)
    {
        SDL_Log("Map has no entities to load.");
        return true;
    }

    while (layer)
    {
        if (is_layer_of_type(OBJECT_GROUP, layer, map))
        {
            object = get_head_object(layer, map);
            while (object)
            {
                map->entity_count += 1;
                object = object->next;
            }
        }
        layer = layer->next;
    }

    if (map->entity_count)
    {
        map->entity = (entity_t *)SDL_calloc((size_t)map->entity_count, sizeof(struct entity));
        if (!map->entity)
        {
            SDL_Log("Error allocating memory for map entities");
            return false;
        }
    }

    SDL_Log("Loading %u entitie(s)", map->entity_count);

    layer = get_head_layer(map);
    while (layer)
    {
        if (is_layer_of_type(OBJECT_GROUP, layer, map))
        {
            int index = 0;
            object = get_head_object(layer, map);
            while (object)
            {
                entity_t *entity = &map->entity[index];

                entity->handle = object;
                entity->pos_x = (int)object->x;
                entity->pos_y = (int)object->y;
                entity->uid = (int)get_object_uid(object);
                entity->id = (int)index + 1;
                entity->is_gone = false;

                if (H_SPAWN == generate_hash(get_object_name(object)))
                {
                    map->spawn_x = entity->pos_x;
                    map->spawn_y = entity->pos_y;
                }

                index += 1;
                object = object->next;
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

    if (!load_surface_from_file((const char *)file_name, &map->tileset_surface))
    {
        SDL_Log("Error loading tileset image '%s'", file_name);
        exit_code = false;
    }

    return exit_code;
}

static bool load_animated_tiles(map_t *map)
{
    cute_tiled_layer_t *layer = get_head_layer(map);
    int animated_tile_count = 0;

    while (layer)
    {
        if (layer->visible)
        {
            if (is_layer_of_type(TILE_LAYER, layer, map))
            {
                for (int index_height = 0; index_height < map->handle->height; index_height += 1)
                {
                    for (int index_width = 0; index_width < map->handle->width; index_width += 1)
                    {
                        int *layer_content = get_layer_content(layer);
                        int gid = remove_gid_flip_bits(layer_content[(index_height * map->handle->width) + index_width]);

                        if (is_tile_animated(gid, NULL, NULL, map->handle))
                        {
                            animated_tile_count += 1;
                        }
                    }
                }
            }
            else if (is_layer_of_type(OBJECT_GROUP, layer, map))
            {
                cute_tiled_object_t *object = get_head_object(layer, map);
                while (object)
                {
                    animated_tile_count += 1;
                    object = object->next;
                }
            }
        }
        layer = layer->next;
    }

    if (animated_tile_count > 0)
    {
        SDL_Log("Loading %u animated tile(s)", animated_tile_count);

        map->animated_tile = (anim_tile_t *)SDL_calloc((size_t)animated_tile_count, sizeof(struct anim_tile));
        if (!map->animated_tile)
        {
            SDL_Log("Error allocating memory for animated tile");
            return false;
        }
    }
    else
    {
        return true;
    }

    return true;
}

void destroy_map(map_t *map)
{
    if (!map)
    {
        return;
    }

    // Free up allocated memory in reverse order.

    // [7] Animated tiles.
    SDL_free(map->animated_tile);

    // [6] Tileset.
    if (map->tileset_surface)
    {
        SDL_DestroySurface(map->tileset_surface);
        map->tileset_surface = NULL;
    }

    // [5] Entities.
    SDL_free(map->entity);
    map->entity = NULL;

    // [4] Tiles.
    SDL_free(map->tile_desc);
    map->tile_desc = NULL;

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

    if (*map)
    {
        destroy_map(*map);
    }

    SDL_Log("Loading map: %s", file_name);

    // Load map file and allocate required memory.

    // [1] Map.
    *map = (map_t *)SDL_calloc(1, sizeof(struct map));
    if (!*map)
    {
        SDL_Log("Error allocating memory for map");
        return false;
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

    // [5] Entities.
    if (!load_entities(*map))
    {
        exit_code = false;
        goto exit;
    }

    // [6] Tileset.
    if (!load_tileset(*map))
    {
        exit_code = false;
        goto exit;
    }

    // [7] Animated tiles.
    if (!load_animated_tiles(*map))
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

bool render_map(map_t *map, SDL_Renderer *renderer)
{
    cute_tiled_layer_t *layer;
    cute_tiled_layer_t *prev_layer = NULL;

    if (!map || !renderer)
    {
        SDL_Log("Invalid parameters: map or renderer is NULL.");
        return false;
    }

    // Static tiles have already been rendered.
    if (map->static_tiles_rendered)
    {
        if (map->animated_tile_index <= 0)
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

        // Update and render animated tiles & entities.
        map->time_since_last_frame += map->delta_time;

        if (map->time_since_last_frame >= (1000 / ANIM_FPS))
        {
            map->time_since_last_frame = 0;

            for (int index = 0; map->animated_tile_index > index; index += 1)
            {
                SDL_Rect dst = { 0 };
                SDL_Rect src = { 0 };
                int gid = map->animated_tile[index].gid;
                int next_tile_id = 0;
                int local_id;

                local_id = map->animated_tile[index].id + 1;
                src.w = dst.w = get_tile_width(map->handle);
                src.h = dst.h = get_tile_height(map->handle);
                src.x = 0;
                src.y = 0;
                dst.x = map->animated_tile[index].dst_x;
                dst.y = map->animated_tile[index].dst_y;

                int tmp_x, tmp_y;
                get_tile_position(local_id, &tmp_x, &tmp_y, map->handle);
                src.x = tmp_x;
                src.y = tmp_y;

                // Simulate transparency by blitting the uppermost static tile first.
                // Note: the canvas tile has to be on the layer below the object layer.
                SDL_Rect canvas_src = { 0 };
                canvas_src.w = src.w;
                canvas_src.h = src.h;
                canvas_src.x = map->animated_tile[index].canvas_src_x;
                canvas_src.y = map->animated_tile[index].canvas_src_y;
                SDL_BlitSurface(map->tileset_surface, &canvas_src, map->render_canvas, &dst);

                for (int n = 0; n <= map->entity_count; n += 1)
                {
                    if (map->entity[n].uid == map->animated_tile[index].object_id)
                    {
                        if (!map->entity[n].is_gone)
                        {
                            SDL_BlitSurface(map->tileset_surface, &src, map->render_canvas, &dst);

                            map->animated_tile[index].current_frame += 1;
                            if (map->animated_tile[index].current_frame >= map->animated_tile[index].anim_length)
                            {
                                map->animated_tile[index].current_frame = 0;
                            }
                        }
                    }
                }

                next_tile_id = get_next_animated_tile_id(gid, map->animated_tile[index].current_frame, map->handle);

                map->animated_tile[index].id = next_tile_id;
            }
        }

        return true;
    }

    // Static tiles have not been rendered yet. Do it once!
    layer = get_head_layer(map);

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
                        int *layer_content = get_layer_content(layer);
                        int gid = remove_gid_flip_bits(layer_content[(index_height * map->handle->width) + index_width]);

                        if (is_gid_valid(gid, map->handle))
                        {
                            int anim_length = 0;
                            int id = 0;

                            src.w = dst.w = get_tile_width(map->handle);
                            src.h = dst.h = get_tile_height(map->handle);
                            dst.x = index_width * get_tile_width(map->handle);
                            dst.y = index_height * get_tile_height(map->handle);

                            int tmp_x, tmp_y;
                            get_tile_position(gid, &tmp_x, &tmp_y, map->handle);
                            src.x = tmp_x;
                            src.y = tmp_y;

                            SDL_BlitSurface(map->tileset_surface, &src, map->render_canvas, &dst);

                            if (is_tile_animated(gid, &anim_length, &id, map->handle))
                            {
                                map->animated_tile[map->animated_tile_index].gid = get_local_id(gid, map->handle);
                                map->animated_tile[map->animated_tile_index].id = id;
                                map->animated_tile[map->animated_tile_index].dst_x = dst.x;
                                map->animated_tile[map->animated_tile_index].dst_y = dst.y;
                                map->animated_tile[map->animated_tile_index].current_frame = 0;
                                map->animated_tile[map->animated_tile_index].anim_length = anim_length;

                                if (prev_layer)
                                {
                                    int *layer_content_below = get_layer_content(prev_layer);
                                    int gid_below = remove_gid_flip_bits(layer_content_below[(index_height * map->handle->width) + index_width]);
                                    if (is_gid_valid(gid_below, map->handle))
                                    {
                                        int tmp_x_below, tmp_y_below;
                                        get_tile_position(gid_below, &tmp_x_below, &tmp_y_below, map->handle);
                                        map->animated_tile[map->animated_tile_index].canvas_src_x = tmp_x_below;
                                        map->animated_tile[map->animated_tile_index].canvas_src_y = tmp_y_below;
                                    }
                                }

                                map->animated_tile_index += 1;
                            }
                        }
                    }
                }

                const char *layer_name = get_layer_name(layer);
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

                    src.w = dst.w = get_tile_width(map->handle);
                    src.h = dst.h = get_tile_height(map->handle);
                    dst.x = (int)object->x;
                    dst.y = (int)object->y - get_tile_height(map->handle);

                    int tmp_x, tmp_y;
                    get_tile_position(gid, &tmp_x, &tmp_y, map->handle);
                    src.x = tmp_x;
                    src.y = tmp_y;

                    if (is_tile_animated(gid, &anim_length, &id, map->handle))
                    {
                        map->animated_tile[map->animated_tile_index].gid = get_local_id(gid, map->handle);
                        map->animated_tile[map->animated_tile_index].id = id;
                        map->animated_tile[map->animated_tile_index].dst_x = dst.x;
                        map->animated_tile[map->animated_tile_index].dst_y = dst.y;
                        map->animated_tile[map->animated_tile_index].current_frame = 0;
                        map->animated_tile[map->animated_tile_index].anim_length = anim_length;
                        map->animated_tile[map->animated_tile_index].object_id = get_object_uid(object);

                        if (prev_layer)
                        {
                            int index_height = dst.y / get_tile_height(map->handle);
                            int index_width = dst.x / get_tile_width(map->handle);
                            int *layer_content_below = get_layer_content(prev_layer);
                            int gid_below = remove_gid_flip_bits(layer_content_below[(index_height * map->handle->width) + index_width]);
                            if (is_gid_valid(gid_below, map->handle))
                            {
                                int tmp_x_below, tmp_y_below;
                                get_tile_position(gid_below, &tmp_x_below, &tmp_y_below, map->handle);
                                map->animated_tile[map->animated_tile_index].canvas_src_x = tmp_x_below;
                                map->animated_tile[map->animated_tile_index].canvas_src_y = tmp_y_below;
                            }
                        }

                        map->animated_tile_index += 1;
                    }
                }

                object = object->next;
            }

            const char *layer_name = get_layer_name(layer);
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

    return true;
}

int get_tile_index(int pos_x, int pos_y, map_t *map)
{
    int index;

    index = pos_x / get_tile_width(map->handle);
    index += (pos_y / get_tile_height(map->handle)) * map->handle->width;

    if (index > (map->tile_desc_count - 1))
    {
        index = map->tile_desc_count - 1;
    }

    return index;
}
