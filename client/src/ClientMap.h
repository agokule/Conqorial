#pragma once

#include "Map.h"
#include "AppState.h"
#include "SDL3/SDL.h"

SDL_Color get_tile_color(MapTileType type);

SDL_Texture *init_map_texture(const Map &map, SDL_Renderer *renderer);

void draw_map_texture(SDL_Texture *texture, SDL_Renderer *renderer, SDL_FRect src_rect);

void zoom_map(float zoom_factor, float center_x, float center_y, AppState &state);

