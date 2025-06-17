/** @file map.h
 *
 *  A minimalist, cross-platform puzzle-platformer, designed
 *  especially for the Nokia N-Gage.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#ifndef MAP_H
#define MAP_H

#include <SDL3/SDL.h>

#include "cute_tiled.h"

typedef struct tile_desc
{
    bool is_solid;
    bool is_wall;
    int offset_top;

} tile_desc_t;

typedef struct anim
{
    Uint64 time_since_last_anim_frame;
    int current_frame;
    int first_frame;
    int fps;
    int length;
    int offset_y;

} anim_t;

typedef struct entity
{
    cute_tiled_object_t *handle;
    int pos_x;
    int pos_y;
    int uid;
    int id;
    int index;
    bool is_gone;

} entity_t;

typedef struct anim_tile
{
    int dst_x;
    int dst_y;
    int canvas_src_x;
    int canvas_src_y;
    int anim_length;
    int current_frame;
    int gid;
    int id;
    int object_id;

} anim_tile_t;

typedef struct map
{
    cute_tiled_map_t *handle;

    int width;
    int height;
    int layer_count;
    int spawn_x;
    int spawn_y;

    SDL_Texture *render_target;
    SDL_Surface *render_canvas;
    SDL_Surface *tileset_surface;

    bool static_tiles_rendered;

    Uint64 hash_id_objectgroup;
    Uint64 hash_id_tilelayer;

    bool boolean_property;
    float decimal_property;
    int integer_property;
    const char *string_property;

    anim_tile_t *animated_tile;
    int animated_tile_index;

    Uint8 bg_r;
    Uint8 bg_g;
    Uint8 bg_b;

    Uint64 time_a;
    Uint64 time_b;
    Uint64 delta_time;
    Uint64 time_since_last_frame;

    entity_t *entity;
    int entity_count;

    tile_desc_t *tile_desc;
    int tile_desc_count;

} map_t;

typedef enum
{
    TILE_LAYER = 0,
    OBJECT_GROUP

} layer_type;

void destroy_map(map_t *map);
bool load_map(const char *file_name, map_t **map, SDL_Renderer *renderer);
bool render_map(map_t *map, SDL_Renderer *renderer);
int get_tile_index(int pos_x, int pos_y, map_t *map);

#endif // MAP_H
