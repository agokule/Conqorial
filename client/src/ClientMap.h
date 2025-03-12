#pragma once

#include "Map.h"
#include "SDL3/SDL.h"

SDL_Color get_tile_color(MapTileType type);

SDL_Texture *init_map_texture(const Map &map, SDL_Renderer *renderer);

void draw_map_texture(SDL_Texture *texture, SDL_Renderer *renderer, SDL_FRect src_rect);


