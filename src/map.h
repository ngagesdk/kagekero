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

#include "aabb.h"
#include "cute_tiled.h"

#define H_COIN 0x000000017c953f2e
#define H_DOOR 0x000000017c95cc59

typedef struct tile_desc
{
    bool is_deadly;
    bool is_solid;
    bool is_wall;
    int offset_top;

} tile_desc_t;

typedef struct obj
{
    int x;
    int y;
    int canvas_src_x;
    int canvas_src_y;
    int anim_length;
    int start_frame;
    int current_frame;
    int gid;
    int id;
    int object_id;

    Uint64 hash;

    bool is_hidden;

} obj_t;

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

    obj_t *obj;
    int obj_count;
    int prev_coins;
    int coins_left;
    int coin_max;

    Uint8 bg_r;
    Uint8 bg_g;
    Uint8 bg_b;

    Uint64 time_a;
    Uint64 time_b;
    Uint64 delta_time;
    Uint64 time_since_last_frame;

    Uint64 tileset_hash;
    Uint64 prev_tileset_hash;

    tile_desc_t *tile_desc;
    int tile_desc_count;

    bool use_lgbtq_flag;

} map_t;

typedef enum
{
    TILE_LAYER = 0,
    OBJECT_GROUP

} layer_type;

void destroy_map(map_t *map);
bool load_map(const char *file_name, map_t **map, SDL_Renderer *renderer);
bool render_map(map_t *map, SDL_Renderer *renderer, bool *has_updated);
int get_tile_index(int pos_x, int pos_y, map_t *map);
bool object_intersects(aabb_t bb, map_t *map, int *index_ptr);

#endif // MAP_H
